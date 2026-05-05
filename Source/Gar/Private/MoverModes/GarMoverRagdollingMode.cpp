// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModes/GarMoverRagdollingMode.h"

#include "Components/SkeletalMeshComponent.h"
#include "MoverComponent.h"
#include "MoveLibrary/MovementUtils.h"
#include "MoveLibrary/BasedMovementUtils.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "MoveLibrary/GroundMovementUtils.h"
#include "MoveLibrary/AirMovementUtils.h"
#include "Settings/GarMovementSettings.h"
#include "State/GarCharacterMoverInputs.h"
#include "GarPhysicalAnimationComponent.h"
#include "GarCharacterMoverComponent.h"
#include "GarCharacter.h"
#include "GarConstants.h"
#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarMoverRagdollingMode)

UGarMoverRagdollingMode::UGarMoverRagdollingMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SharedSettingsClasses.Add(UGarMovementSettings::StaticClass());

	GameplayTags.AddTag(GarLocomotionModeTags::Grounded);
}

void UGarMoverRagdollingMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const UMoverComponent* MoverComp = GetMoverComponent();
	const FMoverDefaultSyncState* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	check(StartingSyncState);

	const AGarCharacter* Character{Cast<AGarCharacter>(MoverComp->GetOwner())};
	// GetBoneTransform は ComponentSpaceTransforms に依存するが、DedicatedServer では
	// VisibilityBasedAnimTickOption により物理シミュレーション結果が骨トランスフォームに
	// コピーされないことがある。GetBodyInstance の物理ボディを直接参照することで
	// サーバーでも正確なラグドール骨位置を取得する。
	FTransform TargetTransform(StartingSyncState->GetOrientation_WorldSpace(), StartingSyncState->GetLocation_WorldSpace());
	if (FBodyInstance* TopBoneBodyForTransform = Character->GetMesh()->GetBodyInstance(TopBoneName))
	{
		TargetTransform = TopBoneBodyForTransform->GetUnrealWorldTransform();
	}
	auto TargetLocation{TargetTransform.GetLocation()};

#if ENABLE_DRAW_DEBUG
	if (UGarUtility::ShouldDisplayDebugForActor(Character, UGarConstants::PADebugDisplayName()))
	{
		DrawDebugCoordinateSystem(Character->GetWorld(), TargetLocation, TargetTransform.Rotator(), 150.0f);
	}
#endif

	if (Character->GetPhysicalAnimation()->GetRagdollingState().bFreezing)
	{
		return;
	}

	const float DeltaSeconds = TimeStep.StepMs * 0.001f;

	const FVector CurrentLocation = Character->GetActorLocation();

	auto TopBoneBody{Character->GetMesh()->GetBodyInstance(TopBoneName)};

	float TopBoneSpeed2D = 0;
	float TopBoneSpeed3D = 0;
	
	if (TopBoneBody)
	{
		const FVector TopBoneVelocity = TopBoneBody->GetUnrealWorldVelocity();
		TopBoneSpeed2D = TopBoneVelocity.Size2D();
		TopBoneSpeed3D = TopBoneVelocity.Size();
	}

	FVector Velocity{ForceInit};
	auto Diff{TargetLocation - CurrentLocation};
	if (GameplayTags.HasTag(GarLocomotionModeTags::Grounded))
	{
		auto Len = Diff.Size2D();
		if (DeltaSeconds > 0 && Len > 0.1f)
		{
			auto Dir{Diff.GetSafeNormal2D()};
			Velocity = (Dir * (Len / DeltaSeconds)).GetClampedToMaxSize(FMath::Clamp(TopBoneSpeed2D, MinSpeed, MaxSpeed));
		}
	}
	else
	{
		auto Len = Diff.Size();
		if (DeltaSeconds > 0 && Len > 0.1f)
		{
			Velocity = ((TargetLocation - CurrentLocation) / DeltaSeconds).GetClampedToMaxSize(FMath::Clamp(TopBoneSpeed3D, MinSpeed, MaxSpeed));
		}
	}

	auto TargetDirection{TargetTransform.GetRotation().RotateVector(FVector::RightVector)};
	auto DotY{FVector::UpVector.Dot(TargetDirection)};
	if (FMath::Abs(DotY) > 0.7)
	{
		if (DotY > 0)
		{
			TargetDirection = TargetTransform.GetRotation().RotateVector(FVector::BackwardVector);
		}
		else
		{
			TargetDirection = TargetTransform.GetRotation().RotateVector(FVector::ForwardVector);
		}
	}
	const FRotator CurrentRotation = Character->GetActorRotation();
	const FRotator RDiff{(TargetDirection.GetSafeNormal2D().Rotation() - CurrentRotation).GetNormalized()};
	FRotator AngularVelocity{ForceInit};
	if (DeltaSeconds > 0 && !RDiff.IsNearlyZero(1.0))
	{
		AngularVelocity.Yaw = FMath::Clamp((RDiff * (1.0f / DeltaSeconds)).Yaw, 1, 360*2);
	}

	OutProposedMove.LinearVelocity = Velocity;
	//OutProposedMove.AngularVelocity = AngularVelocity;
}

void UGarMoverRagdollingMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	UGarCharacterMoverComponent* MoverComp = Cast<UGarCharacterMoverComponent>(GetMoverComponent());
	const FMoverTickStartData& StartState = Params.StartState;
	USceneComponent* UpdatedComponent = Params.MovingComps.UpdatedComponent.Get();
	FProposedMove ProposedMove = Params.ProposedMove;

	if (!UpdatedComponent)
	{
		return;
	}

	const FMoverDefaultSyncState* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	check(StartingSyncState);

	FMoverDefaultSyncState& OutputSyncState = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FMoverDefaultSyncState>();

	const float DeltaSeconds = Params.TimeStep.StepMs * 0.001f;

	FMovementRecord MoveRecord;
	MoveRecord.SetDeltaSeconds(DeltaSeconds);

	if (GameplayTags.HasTag(GarLocomotionModeTags::InAir))
	{
		UMoverBlackboard* SimBlackboard = MoverComp->GetSimBlackboard_Mutable();

		SimBlackboard->Invalidate(CommonBlackboard::LastFloorResult);	// flying = no valid floor
		SimBlackboard->Invalidate(CommonBlackboard::LastFoundDynamicMovementBase);
	}

	OutputSyncState.MoveDirectionIntent = (ProposedMove.bHasDirIntent ? ProposedMove.DirectionIntent : FVector::ZeroVector);


	// Use the orientation intent directly. If no intent is provided, use last frame's orientation. Note that we are assuming rotation changes can't fail. 
	const FRotator StartingOrient = StartingSyncState->GetOrientation_WorldSpace();

	const FRotator TargetOrient = UMovementUtils::ApplyAngularVelocityToRotator(StartingOrient, ProposedMove.AngularVelocityDegrees, DeltaSeconds);
	const bool bIsOrientationChanging = !StartingOrient.Equals(TargetOrient);
	
	FVector MoveDelta = ProposedMove.LinearVelocity * DeltaSeconds;

	FQuat TargetOrientQuat = TargetOrient.Quaternion();
	if (Settings->bShouldRemainVertical)
	{
		TargetOrientQuat = FRotationMatrix::MakeFromZX(MoverComp->GetUpDirection(), TargetOrientQuat.GetForwardVector()).ToQuat();
	}

	FHitResult Hit(1.f);

	if (!MoveDelta.IsNearlyZero() || bIsOrientationChanging)
	{
		UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, MoveDelta, TargetOrientQuat, true, Hit, ETeleportType::None, MoveRecord);
	}

	if (Hit.IsValidBlockingHit())
	{
		FMoverOnImpactParams ImpactParams(DefaultModeNames::Flying, Hit, MoveDelta);
		MoverComp->HandleImpact(ImpactParams);

		// Try to slide the remaining distance along the surface.
		UMovementUtils::TryMoveToSlideAlongSurface(FMovingComponentSet(MoverComp), MoveDelta, 1.f - Hit.Time, TargetOrientQuat, Hit.Normal, Hit, true, MoveRecord);
	}

	// If we are very close to a walkable surface, make sure we maintain a small gap over it
	FFloorCheckResult FloorUnderActor;
	UFloorQueryUtils::FindFloor(Params.MovingComps, Settings->FloorSweepDistance, Settings->MaxWalkSlopeCosine, Settings->bUseFlatBaseForFloorChecks, UpdatedComponent->GetComponentLocation(), OUT FloorUnderActor);

	if (FloorUnderActor.IsWalkableFloor())
	{
		UGroundMovementUtils::TryMoveToAdjustHeightAboveFloor(MoverComp, FloorUnderActor, Settings->MaxWalkSlopeCosine, MoveRecord);
		if (HasGameplayTag(GarLocomotionModeTags::InAir, true))
		{
			OutputState.MovementEndState.NextModeName = TEXT("Ragdolling");
		}
	}
	else
	{
		if (HasGameplayTag(GarLocomotionModeTags::Grounded, true))
		{
			OutputState.MovementEndState.NextModeName = TEXT("Ragdolling In Air");
		}
	}

	CaptureFinalState(UpdatedComponent, MoveRecord, *StartingSyncState, ProposedMove.AngularVelocityDegrees, OutputSyncState, DeltaSeconds);
}

// TODO: replace this function with simply looking at/collapsing the MovementRecord
void UGarMoverRagdollingMode::CaptureFinalState(USceneComponent* UpdatedComponent, FMovementRecord& Record, const FMoverDefaultSyncState& StartSyncState, const FVector& AngularVelocityDegrees, FMoverDefaultSyncState& OutputSyncState, const float DeltaSeconds) const
{
	const FVector FinalLocation = UpdatedComponent->GetComponentLocation();
	const FVector FinalVelocity = Record.GetRelevantVelocity();
	
	// TODO: Update Main/large movement record with substeps from our local record

	OutputSyncState.SetTransforms_WorldSpace(FinalLocation,
											  UpdatedComponent->GetComponentRotation(),
											  FinalVelocity,
											  AngularVelocityDegrees,
											  nullptr); // no movement base

	UpdatedComponent->ComponentVelocity = FinalVelocity;
}

void UGarMoverRagdollingMode::OnRegistered(const FName ModeName)
{
	Super::OnRegistered(ModeName);

	Settings = GetMoverComponent()->FindSharedSettings<UGarMovementSettings>();
	ensureMsgf(Settings, TEXT("Failed to find instance of GarMovementSettings on %s. Movement may not function properly."), *GetPathNameSafe(this));
}

void UGarMoverRagdollingMode::OnUnregistered()
{
	Settings = nullptr;

	Super::OnUnregistered();
}
