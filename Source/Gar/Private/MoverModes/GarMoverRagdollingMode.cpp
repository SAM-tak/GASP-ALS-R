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
#include "GarCharacterMoverComponent.h"
#include "GarCharacter.h"
#include "GarConstants.h"
#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarMoverRagdollingMode)

UGarMoverRagdollingMode::UGarMoverRagdollingMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SharedSettingsClasses.Add(UGarMovementSettings::StaticClass());
}

void UGarMoverRagdollingMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const UMoverComponent* MoverComp = GetMoverComponent();
	const FMoverDefaultSyncState* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	check(StartingSyncState);

	const AGarCharacter* Character{Cast<AGarCharacter>(MoverComp->GetOwner())};
	auto TargetLocation{Character->GetMesh()->GetBoneLocation(TopBoneName)};

#if ENABLE_DRAW_DEBUG
	if (UGarUtility::ShouldDisplayDebugForActor(Character, UGarConstants::PADebugDisplayName()))
	{
		DrawDebugCrosshairs(Character->GetWorld(), TargetLocation, FRotator::ZeroRotator, 100.0f, FColor::Green, false, 1.0f);
	}
#endif

	const float DeltaSeconds = TimeStep.StepMs * 0.001f;

	const FVector CurrentLocation = Character->GetActorLocation();

	FVector Velocity = ((TargetLocation - CurrentLocation) / DeltaSeconds).GetClampedToMaxSize(MaxSpeed);

	OutProposedMove.LinearVelocity = Velocity;
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

	UMoverBlackboard* SimBlackboard = MoverComp->GetSimBlackboard_Mutable();

	SimBlackboard->Invalidate(CommonBlackboard::LastFloorResult);	// flying = no valid floor
	SimBlackboard->Invalidate(CommonBlackboard::LastFoundDynamicMovementBase);

	OutputSyncState.MoveDirectionIntent = (ProposedMove.bHasDirIntent ? ProposedMove.DirectionIntent : FVector::ZeroVector);

	// Use the orientation intent directly. If no intent is provided, use last frame's orientation. Note that we are assuming rotation changes can't fail. 
	const FRotator StartingOrient = StartingSyncState->GetOrientation_WorldSpace();
	FRotator TargetOrient = StartingOrient;

	bool bIsOrientationChanging = false;

	// Apply orientation changes (if any)
	if (!UMovementUtils::IsAngularVelocityZero(ProposedMove.AngularVelocity))
	{
		TargetOrient += (ProposedMove.AngularVelocity * DeltaSeconds);
		bIsOrientationChanging = (TargetOrient != StartingOrient);
	}
	
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
	UFloorQueryUtils::FindFloor(Params.MovingComps, Settings->FloorSweepDistance, Settings->MaxWalkSlopeCosine, UpdatedComponent->GetComponentLocation(), OUT FloorUnderActor);

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

	CaptureFinalState(UpdatedComponent, MoveRecord, *StartingSyncState, OutputSyncState, DeltaSeconds);
}

// TODO: replace this function with simply looking at/collapsing the MovementRecord
void UGarMoverRagdollingMode::CaptureFinalState(USceneComponent* UpdatedComponent, FMovementRecord& Record, const FMoverDefaultSyncState& StartSyncState,
	FMoverDefaultSyncState& OutputSyncState, const float DeltaSeconds) const
{
	const FVector FinalLocation = UpdatedComponent->GetComponentLocation();
	const FVector FinalVelocity = Record.GetRelevantVelocity();
	
	// TODO: Update Main/large movement record with substeps from our local record

	OutputSyncState.SetTransforms_WorldSpace(FinalLocation,
											  UpdatedComponent->GetComponentRotation(),
											  FinalVelocity,
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
