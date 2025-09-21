// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/Actions/GarGameplayAbility_Traversal.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "ChooserFunctionLibrary.h"
#include "AnimationWarpingLibrary.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystemGlobals.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarAnimationInstance.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Abilities/Tasks/GarAbilityTask_Tick.h"
#include "Utility/GarMath.h"
#include "Utility/GarUtility.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Traversal)

TMap<FGameplayAbilitySpecHandle, FGarTraversalParameters> UGarGameplayAbility_Traversal::ParameterMap;

UGarGameplayAbility_Traversal::UGarGameplayAbility_Traversal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarLocomotionActionTags::Traversal));
	ActivationOwnedTags.AddTag(GarLocomotionActionTags::Traversal);
	ActivationOwnedTags.AddTag(GarStateFlagTags::BlockUpdateCapsuleSize);
	ActivationOwnedTags.AddTag(GarStateFlagTags::RotationLocked);
	CancelAbilitiesWithTag.AddTag(GarLocomotionActionTags::Root);
	BlockAbilitiesWithTag.AddTag(GarLocomotionActionTags::Traversal);
	BlockAbilitiesWithTag.AddTag(GarLocomotionActionTags::Rolling);
	ActivationBlockedTags.AddTag(GarLocomotionActionTags::Unconsious);
	ActivationBlockedTags.AddTag(GarLocomotionActionTags::Dying);

	TraversalTraceResponses.WorldStatic = ECR_Block;
	TraversalTraceResponses.WorldDynamic = ECR_Block;
	TraversalTraceResponses.Destructible = ECR_Block;
}

bool UGarGameplayAbility_Traversal::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
													  const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
													  OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	FGarTraversalParameters Params;
	if (CanTraversal(Handle, *ActorInfo, Params))
	{
		// The idea of putting the mutable member variable into the ActivateAbility method does not work well.
		// The memory area is cleared, or if it is a different instance from when CanActivateAbility is called,
		// anyway the contents of the parameter are lost.
		// Therefore, I am passing it with a static variable map.
		CommitParameters(Handle, Params);
		return true;
	}
	return false;
}

bool UGarGameplayAbility_Traversal::CanTraversal(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo,
	FGarTraversalParameters& Params) const
{
	auto* Character{Cast<AGarCharacter>(ActorInfo.OwnerActor)};
	if (!IsValid(Character))
	{
		return false;
	}

	FGarTraversalTraceResult TraceResult;
	if (!TraceEnvironment(Character, TraceResult))
	{
		return false;
	}

	Params.TargetPrimitive = TraceResult.TargetPrimitive;
	Params.FrontLedgeLocation = TraceResult.FrontLedgeLocation;
	Params.UpperLedgeNormal = TraceResult.UpperLedgeNormal;
	Params.BackLedgeLocation = TraceResult.BackLedgeLocation;
	Params.BackFloorLocation = TraceResult.BackFloorLocation;

	FGarTraversalChooserInputs ChooserInputs;
	ChooserInputs.bHasFrontLedge = true;
	ChooserInputs.bHasBackLedge = true;
	ChooserInputs.bHasBackFloor = TraceResult.bHasBackFloor;
	ChooserInputs.ObstacleHeight = UE_REAL_TO_FLOAT(
		Params.FrontLedgeLocation.Z - Character->GetActorLocation().Z + Character->GetCapsule()->GetScaledCapsuleHalfHeight()
	);
	ChooserInputs.ObstacleDepth = UE_REAL_TO_FLOAT(FVector::DistXY(Params.FrontLedgeLocation, Params.BackLedgeLocation));
	ChooserInputs.BackFloorFall = Params.BackLedgeLocation.Z - Params.BackFloorLocation.Z;
	ChooserInputs.Speed = Character->GetVelocity().Size2D();
	ChooserInputs.CurrentGameplayTags.Reset();
	Character->GetOwnedGameplayTags(ChooserInputs.CurrentGameplayTags);

	// Determine the mantling type by checking the movement mode and mantling height.

	TArray<UAnimMontage*> Candidates;

	ChooseCandidate(ChooserInputs, Params.ChooserOutput, Candidates);

	if (Candidates.IsEmpty())
	{
		return false;
	}

	Params.Result = MotionMatch(TArray<UObject*>(Candidates));

	if (!IsValid(Params.Result.SelectedAnim))
	{
		return false;
	}

	ensure(Cast<UAnimMontage>(Params.Result.SelectedAnim));

	return true;
}

bool UGarGameplayAbility_Traversal::TraceEnvironment_Implementation(AGarCharacter* Character, FGarTraversalTraceResult& OutResult) const
{
	const auto ActorLocation{Character->GetActorLocation()};
	const auto ActorYawAngle{UE_REAL_TO_FLOAT(FMath::UnwindDegrees(Character->GetActorRotation().Yaw))};
	const auto Velocity{Character->GetVelocity()};
	float ForwardTraceAngle;
	if (Velocity.Size2D() > StaticSpeedThreshold)
	{
		auto VelocityYawAngle{UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(Velocity))};
		ForwardTraceAngle = Character->HasInput()
			                ? VelocityYawAngle +
			                    FMath::ClampAngle(UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(Character->GetInputDirection())) - VelocityYawAngle,
			                                    -MaxReachAngle, MaxReachAngle)
			                : VelocityYawAngle;
	}
	else
	{
		ForwardTraceAngle = Character->HasInput() ? UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(Character->GetInputDirection())) : ActorYawAngle;
	}

	const auto ForwardTraceDeltaAngle{FMath::UnwindDegrees(ForwardTraceAngle - ActorYawAngle)};
	if (FMath::Abs(ForwardTraceDeltaAngle) > TraceAngleThreshold)
	{
		return false;
	}

	const auto ForwardTraceDirection{UGarMath::AngleToDirectionXY(ActorYawAngle + FMath::ClampAngle(ForwardTraceDeltaAngle, -MaxReachAngle, MaxReachAngle))};

#if ENABLE_DRAW_DEBUG
	bool bDisplayDebug{UGarUtility::ShouldDisplayDebugForActor(Character, UGarConstants::TraversalDebugDisplayName())};
#endif

	const auto* Capsule{Character->GetCapsule()};

	const auto CapsuleScale{Capsule->GetComponentScale().Z};
	const auto CapsuleRadius{Capsule->GetScaledCapsuleRadius()};
	const auto CapsuleHalfHeight{Capsule->GetScaledCapsuleHalfHeight()};

	const FVector CapsuleBottomLocation{ActorLocation.X, ActorLocation.Y, ActorLocation.Z - CapsuleHalfHeight};

	const auto TraceCapsuleRadius{CapsuleRadius - 1.0f};

	bool bInAir{Character->HasMatchingGameplayTag(GarLocomotionModeTags::InAir)};

	const FGarTraversalTraceSettings& TraceSettings{bInAir ? InAirTrace : GroundedTrace};

	const auto LedgeHeightDelta{UE_REAL_TO_FLOAT((TraceSettings.LedgeHeight.GetMax() - TraceSettings.LedgeHeight.GetMin()) * CapsuleScale)};

	// Trace forward to find an object the character cannot walk on.

	static const FName ForwardTraceTag{FString::Printf(TEXT("%hs (Forward Trace)"), __FUNCTION__)};

	auto ForwardTraceStart{CapsuleBottomLocation - ForwardTraceDirection * CapsuleRadius};
	ForwardTraceStart.Z += (TraceSettings.LedgeHeight.X + TraceSettings.LedgeHeight.Y) *
							0.5f * CapsuleScale - UGarCharacterMoverComponent::MAX_FLOOR_DIST;

	auto ForwardTraceEnd{ForwardTraceStart + ForwardTraceDirection * (CapsuleRadius + (TraceSettings.ReachDistance + 1.0f) * CapsuleScale)};

	const auto ForwardTraceCapsuleHalfHeight{LedgeHeightDelta * 0.5f};

	auto* World{Character->GetWorld()};

	FHitResult ForwardTraceHit;
	World->SweepSingleByChannel(ForwardTraceHit, ForwardTraceStart, ForwardTraceEnd,
	                            FQuat::Identity, TraversalTraceChannel,
	                            FCollisionShape::MakeCapsule(TraceCapsuleRadius, ForwardTraceCapsuleHalfHeight),
	                            {ForwardTraceTag, false, Character}, TraversalTraceResponses);

	if (!ForwardTraceHit.IsValidBlockingHit())
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UGarUtility::DrawDebugSweepSingleCapsuleAlternative(World, ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, false, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);
		}
#endif

		return false;
	}

	// Determine front edge position.
	// Trace downward from the first trace's impact point and determine if the hit location is walkable.

	const auto TargetDirection{-ForwardTraceHit.ImpactNormal.GetSafeNormal2D()};

	static const FName FrontLedgeTraceTag{FString::Printf(TEXT("%hs (FrontEdge Trace)"), __FUNCTION__)};

	const FVector2D TargetLocationOffset{TargetDirection * (TraceSettings.TargetLocationOffset * CapsuleScale)};

	const FVector FrontLedgeTraceStart{
		ForwardTraceHit.ImpactPoint.X + TargetLocationOffset.X,
		ForwardTraceHit.ImpactPoint.Y + TargetLocationOffset.Y,
		CapsuleBottomLocation.Z + LedgeHeightDelta + 2.5f * TraceCapsuleRadius + UGarCharacterMoverComponent::MIN_FLOOR_DIST
	};

	const FVector FrontLedgeTraceEnd{
		FrontLedgeTraceStart.X,
		FrontLedgeTraceStart.Y,
		CapsuleBottomLocation.Z +
		TraceSettings.LedgeHeight.GetMin() * CapsuleScale + TraceCapsuleRadius - UGarCharacterMoverComponent::MAX_FLOOR_DIST
	};

	FHitResult FrontLedgeTraceHit;
	World->SweepSingleByChannel(FrontLedgeTraceHit, FrontLedgeTraceStart, FrontLedgeTraceEnd, FQuat::Identity,
	                            TraversalTraceChannel, FCollisionShape::MakeSphere(TraceCapsuleRadius),
	                            {FrontLedgeTraceTag, false, Character}, TraversalTraceResponses);

	auto TargetPrimitive{FrontLedgeTraceHit.GetComponent()};

	if (!FrontLedgeTraceHit.IsValidBlockingHit() ||
		HasNonTraversalTag(FrontLedgeTraceHit) ||
	    !IsValid(TargetPrimitive) ||
		TargetPrimitive->GetComponentVelocity().SizeSquared() > FMath::Square(TargetPrimitiveSpeedThreshold) ||
	    !TargetPrimitive->CanCharacterStepUp(Character) ||
		Character->GetMover()->IsWalkable(ForwardTraceHit))
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UGarUtility::DrawDebugSweepSingleCapsuleAlternative(World, ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, false, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);
		}
#endif

		return false;
	}

	OutResult.TargetPrimitive = TargetPrimitive;
	OutResult.FrontLedgeLocation = FrontLedgeTraceHit.ImpactPoint;
	OutResult.UpperLedgeNormal = FrontLedgeTraceHit.ImpactNormal;

	const auto SlopeAngleCos{UE_REAL_TO_FLOAT(FrontLedgeTraceHit.ImpactNormal.Z)};

	// The approximate slope angle is used in situations where the normal slope angle cannot convey
	// the true nature of the surface slope, for example, for a 45 degree staircase the slope
	// angle will always be 90 degrees, while the approximate slope angle will be ~45 degrees.

	auto ApproximateSlopeNormal{FrontLedgeTraceHit.Location - FrontLedgeTraceHit.ImpactPoint};
	ApproximateSlopeNormal.Normalize();

	const auto ApproximateSlopeAngleCos{UE_REAL_TO_FLOAT(ApproximateSlopeNormal.Z)};

	if (SlopeAngleCos < SlopeAngleThresholdCos ||
	    ApproximateSlopeAngleCos < SlopeAngleThresholdCos ||
	    !Character->GetMover()->IsWalkable(FrontLedgeTraceHit))
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UGarUtility::DrawDebugSweepSingleCapsuleAlternative(World, ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, true, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);

			UGarUtility::DrawDebugSweepSingleSphere(World, FrontLedgeTraceStart, FrontLedgeTraceEnd, TraceCapsuleRadius,
			                                        false, FrontLedgeTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
			                                        TraceSettings.bDrawFailedTraces ? 7.5f : 0.0f);
		}
#endif

		return false;
	}

	// Check that there is enough free space for the capsule at the target location.

	static const FName TargetLocationTraceTag{FString::Printf(TEXT("%hs (Target Location Overlap)"), __FUNCTION__)};

	const FVector TargetLocation{FrontLedgeTraceHit.ImpactPoint + FVector{0.0f, 0.0f, UGarCharacterMoverComponent::MIN_FLOOR_DIST}};

	const FVector TargetCapsuleLocation{TargetLocation.X, TargetLocation.Y, TargetLocation.Z + CapsuleHalfHeight};

	if (World->OverlapBlockingTestByChannel(TargetCapsuleLocation, FQuat::Identity, TraversalTraceChannel,
	                                        FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
	                                        {TargetLocationTraceTag, false, Character}, TraversalTraceResponses))
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UGarUtility::DrawDebugSweepSingleCapsuleAlternative(World, ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, true, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);

			UGarUtility::DrawDebugSweepSingleSphere(World, FrontLedgeTraceStart, FrontLedgeTraceEnd, TraceCapsuleRadius,
			                                        false, FrontLedgeTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
			                                        TraceSettings.bDrawFailedTraces ? 7.5f : 0.0f);

			DrawDebugCapsule(World, TargetCapsuleLocation, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity,
			                 FColor::Red, false, TraceSettings.bDrawFailedTraces ? 10.0f : 0.0f);
		}
#endif

		return false;
	}

	// Perform additional overlap at the approximate start location to
	// ensure there are no vertical obstacles on the path, such as a ceiling.

	static const FName StartLocationTraceTag{FString::Printf(TEXT("%hs (Start Location Overlap)"), __FUNCTION__)};

	const FVector2D StartLocationOffset{TargetDirection * (TraceSettings.StartLocationOffset * CapsuleScale)};

	const FVector StartLocation{
		TargetLocation.X - StartLocationOffset.X,
		TargetLocation.Y - StartLocationOffset.Y,
		(FrontLedgeTraceHit.Location.Z + FrontLedgeTraceEnd.Z) * 0.5f
	};

	const auto StartLocationTraceCapsuleHalfHeight{(FrontLedgeTraceHit.Location.Z - FrontLedgeTraceEnd.Z) * 0.5f + TraceCapsuleRadius};

	if (World->OverlapBlockingTestByChannel(StartLocation, FQuat::Identity, TraversalTraceChannel,
	                                        FCollisionShape::MakeCapsule(TraceCapsuleRadius, StartLocationTraceCapsuleHalfHeight),
	                                        {StartLocationTraceTag, false, Character}, TraversalTraceResponses))
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UGarUtility::DrawDebugSweepSingleCapsuleAlternative(World, ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, true, ForwardTraceHit,
			                                                    {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);

			UGarUtility::DrawDebugSweepSingleSphere(World, FrontLedgeTraceStart, FrontLedgeTraceEnd, TraceCapsuleRadius,
			                                        false, FrontLedgeTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
			                                        TraceSettings.bDrawFailedTraces ? 7.5f : 0.0f);

			DrawDebugCapsule(World, StartLocation, StartLocationTraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity,
			                 FLinearColor{1.0f, 0.5f, 0.0f}.ToFColor(true), false, TraceSettings.bDrawFailedTraces ? 10.0f : 0.0f);
		}
#endif

		return false;
	}

#if ENABLE_DRAW_DEBUG
	if (bDisplayDebug)
	{
		UGarUtility::DrawDebugSweepSingleCapsuleAlternative(World, ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
		                                                    ForwardTraceCapsuleHalfHeight, true, ForwardTraceHit,
		                                                    {0.0f, 0.25f, 1.0f}, {0.0f, 0.75f, 1.0f}, 5.0f);

		UGarUtility::DrawDebugSweepSingleSphere(World, FrontLedgeTraceStart, FrontLedgeTraceEnd,
		                                        TraceCapsuleRadius, true, FrontLedgeTraceHit,
		                                        {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f}, 7.5f);
	}
#endif

	OutResult.BackLedgeLocation = TargetLocation;

	static const FName EndLocationTraceTag{FString::Printf(TEXT("%hs (End Location Overlap)"), __FUNCTION__)};

	const FVector EndLocationTraceStart{TargetLocation + TargetDirection * (MinimumDepth + CapsuleRadius + Character->GetVelocity().Size2D() * RisingTime)
		+ FVector{0.f, 0.f, TraceCapsuleRadius + UGarCharacterMoverComponent::MIN_FLOOR_DIST}};

	const FVector EndLocationTraceEnd{
		EndLocationTraceStart.X,
		EndLocationTraceStart.Y,
		//Character->GetMover()->GetActorFeetLocation().Z
		0.0f //Character->GetActorFeetLocation().Z // TODO
	};

	FHitResult EndLocationTraceHit;
	World->SweepSingleByChannel(EndLocationTraceHit, EndLocationTraceStart, EndLocationTraceEnd, FQuat::Identity,
		TraversalTraceChannel, FCollisionShape::MakeSphere(TraceCapsuleRadius),
		{EndLocationTraceTag, false, Character}, TraversalTraceResponses);

	#if ENABLE_DRAW_DEBUG
	if (bDisplayDebug)
	{
		UGarUtility::DrawDebugSweepSingleSphere(World, EndLocationTraceStart, EndLocationTraceEnd, TraceCapsuleRadius,
			EndLocationTraceHit.IsValidBlockingHit(), EndLocationTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
			EndLocationTraceHit.IsValidBlockingHit() || TraceSettings.bDrawFailedTraces ? 7.5f : 0.0f);
	}
	#endif

	if (EndLocationTraceHit.IsValidBlockingHit())
	{
		OutResult.bHasBackFloor = true;
		OutResult.BackFloorLocation = EndLocationTraceHit.ImpactPoint;
		if(FVector::DistXY(EndLocationTraceHit.ImpactPoint, EndLocationTraceStart) >= 0.001f)
		{
			OutResult.BackLedgeLocation = EndLocationTraceHit.ImpactPoint;
		}
	}
	else
	{
		OutResult.bHasBackFloor = false;
		OutResult.BackFloorLocation = EndLocationTraceEnd;
	}

	return true;
}

void UGarGameplayAbility_Traversal::CommitParameters(const FGameplayAbilitySpecHandle Handle, const FGarTraversalParameters& Parameters) const
{
	ParameterMap.Add(Handle, Parameters);
}

bool UGarGameplayAbility_Traversal::HasNonTraversalTag(const FHitResult& HitResult) const
{
	auto* Actor{HitResult.GetActor()};
	if (IsValid(Actor) && Actor->ActorHasTag(ExcludeTargetTag))
	{
		return true;
	}
	auto* Component{HitResult.GetComponent()};
	if (IsValid(Component) && Component->ComponentHasTag(ExcludeTargetTag))
	{
		return true;
	}
	return false;
}

void UGarGameplayAbility_Traversal::UpdateWarpTarget() // TODO: bFollowComponentがうまくいっていることを確認したら廃止
{
	auto* Character{GetGarCharacterFromActorInfo()};
	const auto& PrimitiveTransform{CurrentTargetPrimitive->GetComponentTransform()};
	auto BackLedgeLocation{PrimitiveTransform.TransformPosition(BackLedgeOffset)};
	auto BackFloorLocation{PrimitiveTransform.GetLocation() + BackFloorOffset};

	// Perform Traversal Action Event - This event is triggered at the end of the TryTraversalAction function, and uses the result to play a traversal montage.
	// This needs to be an event rather than a function, so that we can use the convenient Play Montage Event node,
	// which triggers latent events when the montage is completed, blends out, or is interrupted.
	// We use the Flying movement mode while doing the traversal action to allow Root Motion to move the character in the Z axis.
	// We also disable collision with the traversed obstacle during the action to ensure the character aligns properly.
	// This is a relatively simple event that can be replaced with a more robust traversal system, and is here for demonstration purposes only.

	// In order for the actor to move to the exact points on the obstacle,
	// we use a Motion Warping component which warps the montage’s root motion using notify states on the montage.
	// This function updates the warp targets in the component using the ledge locations.

	float AnimatedDistanceFromFrontLedgeToBackLedge{0.0f};
	float AnimatedDistanceFromFrontLedgeToBackFloor{0.0f};

	// Update the FrontLedge warp target using the front ledge's location and rotation.
	MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(FName(TEXTVIEW("FrontLedge")), CurrentTargetPrimitive.Get(),
		NAME_None, true, EWarpTargetLocationOffsetDirection::TargetsForwardVector,
		FrontLedgeOffset + FVector(0.f, 0.f, 0.5f), FrontLedgeRotationOffset);

	// If the action type was a hurdle or a vault, we need to also update the BackLedge target. If it is not a hurdle or vault, remove it.
	if (ActionTags.HasTag(GarTraversalActionTags::Hurdle) || ActionTags.HasTag(GarTraversalActionTags::Vault))
	{
		// Because the traversal animations move at different distances (no fixed metrics),
		// we need to know how far the animation moves in order to warp it properly.
		// Here we cache a curve value at the end of the Back Ledge warp window to determine
		// how far the animation is from the front ledge once the character reaches the back ledge location in the animation.
		TArray<FMotionWarpingWindowData> Windows;
		UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(ChosenMontage, FName(TEXTVIEW("BackLedge")), Windows);
		if (!Windows.IsEmpty())
		{
			UAnimationWarpingLibrary::GetCurveValueFromAnimation(ChosenMontage, FName(TEXTVIEW("Distance_From_Ledge")), Windows[0].StartTime,
				AnimatedDistanceFromFrontLedgeToBackLedge);
			// Update the BackLedge warp target.
			MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(FName(TEXTVIEW("BackLedge")), CurrentTargetPrimitive.Get(),
				NAME_None, true, EWarpTargetLocationOffsetDirection::TargetsForwardVector,
				BackLedgeOffset, FRotator::ZeroRotator);
		}
		else
		{
			MotionWarpingComponent->RemoveWarpTarget(FName(TEXTVIEW("BackLedge")));
		}
	}
	else
	{
		MotionWarpingComponent->RemoveWarpTarget(FName(TEXTVIEW("BackLedge")));
	}

	// If the action type was a hurdle, we need to also update the BackFloor target. If it is not a hurdle, remove it.
	if (ActionTags.HasTag(GarTraversalActionTags::Hurdle))
	{
		// Caches a curve value at the end of the Back Floor warp window to determine
		// how far the animation is from the front ledge once the character touches the ground.
		TArray<FMotionWarpingWindowData> Windows;
		UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(ChosenMontage, FName(TEXTVIEW("BackFloor")), Windows);
		if (!Windows.IsEmpty())
		{
			UAnimationWarpingLibrary::GetCurveValueFromAnimation(ChosenMontage, FName(TEXTVIEW("Distance_From_Ledge")), Windows[0].EndTime,
				AnimatedDistanceFromFrontLedgeToBackFloor);
			// Since the animations may land on the floor at different distances (a run hurdle may travel further than a walk or stand hurdle),
			// use the total animated distance away from the back ledge as the X and Y values of the BackFloor warp point.
			// This could technically cause some collision issues if the floor is not flat, or there is an bostacle in the way,
			// therefore having fixed metrics for all traversal animations would be an improvement.
			auto HLoc = BackLedgeLocation + ((BackFloorLocation - BackLedgeLocation).GetSafeNormal2D()
				* FMath::Abs(AnimatedDistanceFromFrontLedgeToBackFloor - AnimatedDistanceFromFrontLedgeToBackLedge));
			MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(FName(TEXTVIEW("BackFloor")), CurrentTargetPrimitive.Get(),
				NAME_None, true, EWarpTargetLocationOffsetDirection::WorldSpace,
				FVector(HLoc.X, HLoc.Y, BackFloorLocation.Z) - PrimitiveTransform.GetLocation(), FRotator::ZeroRotator);
		}
		else
		{
			MotionWarpingComponent->RemoveWarpTarget(FName(TEXTVIEW("BackFloor")));
		}
	}
	else
	{
		MotionWarpingComponent->RemoveWarpTarget(FName(TEXTVIEW("BackFloor")));
	}
}

void UGarGameplayAbility_Traversal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
													const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const auto& Parameters = ParameterMap[Handle];

	ON_SCOPE_EXIT
	{
		ParameterMap.Remove(Handle);
	};

	auto* Character{GetGarCharacterFromActorInfo()};
	auto* AbilitySystem{GetGarAbilitySystemComponentFromActorInfo()};

	ActionTags = Parameters.ChooserOutput.Tags;

	if (ActorInfo->IsNetAuthority())
	{
		auto* Montage{Cast<UAnimMontage>(Parameters.Result.SelectedAnim)};

		if(!PlayMontage(ActivationInfo, Montage, Parameters.Result.WantedPlayRate, NAME_None, Parameters.Result.SelectedTime, Handle, ActorInfo))
		{
			return;
		}

		if (CurrentMotangeDuration <= 0.0f)
		{
			return;
		}

		Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

		if (!IsActive())
		{
			return;
		}

		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
		
		if (!Parameters.TargetPrimitive.IsValid())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		MotionWarpingComponent = Character->GetMotionWarping();

		if (!ensure(MotionWarpingComponent.IsValid()))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		if (!Parameters.TargetPrimitive.IsValid())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		ChosenMontage = Montage;
		CurrentTargetPrimitive = Parameters.TargetPrimitive;

		const auto& PrimitiveTransform{CurrentTargetPrimitive->GetComponentTransform()};
		FrontLedgeOffset = PrimitiveTransform.InverseTransformPosition(Parameters.FrontLedgeLocation);
		FrontLedgeRotationOffset = PrimitiveTransform.InverseTransformRotation(FRotationMatrix::MakeFromXZ(
			(Parameters.FrontLedgeLocation - Character->GetActorLocation()).GetSafeNormal2D(),
			Parameters.UpperLedgeNormal).ToQuat()).Rotator();
		UpperLedgeNormalLS = PrimitiveTransform.InverseTransformVectorNoScale(Parameters.UpperLedgeNormal);
		BackLedgeOffset = PrimitiveTransform.InverseTransformPosition(Parameters.BackLedgeLocation);
		BackFloorOffset = Parameters.BackFloorLocation - PrimitiveTransform.GetLocation();

		UpdateWarpTarget();
	}
	else
	{
		Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	}

	AbilitySystem->AddLooseGameplayTags(ActionTags);

	// Reset network smoothing.

	//Character->GetMover()->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;

	//Character->GetMesh()->SetRelativeLocationAndRotation(Character->GetBaseTranslationOffset(),
	//	Character->GetMesh()->IsUsingAbsoluteRotation()
	//	? Character->GetActorQuat() * Character->GetBaseRotationOffset()
	//	: Character->GetBaseRotationOffset(), false, nullptr, ETeleportType::TeleportPhysics);

	// Clear the character movement mode and set the locomotion action to traverse.

	//Character->GetMover()->FlushServerMoves();
	//Character->GetMover()->SetMovementMode(MOVE_Custom);
	//Character->GetMover()->SetMovementModeLocked(true);
	//Character->GetMover()->SetBase(CurrentTargetPrimitive.Get());
	//Character->GetMover()->SetVelocity(FVector::ZeroVector);
	Character->SetActorRotation((Parameters.FrontLedgeLocation - Character->GetActorLocation()).GetSafeNormal2D().ToOrientationRotator());

	auto CapsuleComponent{Character->GetCapsule()};
	OriginalUnscaledCapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	OriginalUnscaledCapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
	CapsuleComponent->SetCapsuleHalfHeight(CapsuleHalfHeightWhileInAction);
	CapsuleComponent->SetCapsuleRadius(CapsuleRadiusWhileInAction);

	if (ActorInfo->IsNetAuthority())
	{
		TickTask = UGarAbilityTask_Tick::New(this, FName(TEXT("UGarGameplayAbility_Traversal")));
		if (TickTask.IsValid())
		{
			TickTask->OnTick.AddDynamic(this, &ThisClass::Tick);
			TickTask->ReadyForActivation();
		}
	}
}

void UGarGameplayAbility_Traversal::Tick_Implementation(const float DeltaTime)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	auto* CharacterMovement{Character->GetMover()};

	//if (Mover->MovementMode != MOVE_Custom)
	//{
	//	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
	//	return;
	//}

#if ENABLE_DRAW_DEBUG
	if (UGarUtility::ShouldDisplayDebugForActor(Character, UGarConstants::TraversalDebugDisplayName()))
	{
		DebugDrawWarpTarget();
	}
#endif

	if (!CurrentTargetPrimitive.IsValid() || !CurrentTargetPrimitive->IsVisible())
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);

		if (bStartRagdollingOnTargetPrimitiveDestruction)
		{
			GetGarAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesBySingleTag(TryActiveOnPrimitiveDestruction);
		}
	}
}

void UGarGameplayAbility_Traversal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
											   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	auto* Character{GetGarCharacterFromActorInfo()};
	auto* AnimInstance{Character->GetGarAnimationInstace()};
	auto Mover{Character->GetMover()};
	auto* AbilitySystem{GetGarAbilitySystemComponentFromActorInfo()};

	AbilitySystem->RemoveLooseGameplayTags(ActionTags);

	auto CapsuleComponent{Character->GetCapsule()};
	CapsuleComponent->SetCapsuleHalfHeight(OriginalUnscaledCapsuleHalfHeight);
	CapsuleComponent->SetCapsuleRadius(OriginalUnscaledCapsuleRadius);

	//Mover->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;

	//Mover->SetMovementModeLocked(false);
	//if (Mover->MovementMode == MOVE_Custom)
	//{
	//	Mover->SetMovementMode(MOVE_Walking);
	//}

	Character->ForceNetUpdate();
}

#if WITH_EDITOR
void UGarGameplayAbility_Traversal::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, SlopeAngleThreshold))
	{
		SlopeAngleThresholdCos = FMath::Cos(FMath::DegreesToRadians(SlopeAngleThreshold));
	}
	else if (PropertyChangedEvent.GetPropertyName() != GET_MEMBER_NAME_CHECKED(ThisClass, TraversalTraceResponseChannels))
	{
		return;
	}

	TraversalTraceResponses.SetAllChannels(ECR_Ignore);

	for (const auto& CollisionChannel : TraversalTraceResponseChannels)
	{
		TraversalTraceResponses.SetResponse(CollisionChannel, ECR_Block);
	}
}
#endif

#if ENABLE_DRAW_DEBUG
void UGarGameplayAbility_Traversal::DebugDrawWarpTarget()
{
	if (const auto Target = MotionWarpingComponent->FindWarpTarget(FName("FrontLedge")))
	{
		const FVector Location{Target->GetLocation()};
		const FRotator Rotation{Target->GetRotation().Rotator()};

		DrawDebugSphere(GetWorld(), Location, 10.f, 12, FColor::Green, false, 2.0f);
		DrawDebugString(GetWorld(), Location, "FrontLedge", nullptr, FColor::Green, 2.0f);

		FVector Forward = Rotation.Vector();
		DrawDebugLine(GetWorld(), Location, Location + Forward * 100.f, FColor::Blue, false, 2.f, 0, 2.f);
	}
	if (const auto Target = MotionWarpingComponent->FindWarpTarget(FName("BackLedge")))
	{
		const FVector Location{Target->GetLocation()};
		const FRotator Rotation{Target->GetRotation().Rotator()};

		DrawDebugSphere(GetWorld(), Location, 15.f, 12, FColor::Green, false, 2.0f);
		DrawDebugString(GetWorld(), Location, "BackLedge", nullptr, FColor::Green, 2.0f);

		FVector Forward = Rotation.Vector();
		DrawDebugLine(GetWorld(), Location, Location + Forward * 100.f, FColor::Blue, false, 2.f, 0, 2.f);
	}
	if (const auto Target = MotionWarpingComponent->FindWarpTarget(FName("BackFloor")))
	{
		const FVector Location{Target->GetLocation()};
		const FRotator Rotation{Target->GetRotation().Rotator()};

		DrawDebugSphere(GetWorld(), Location, 20.f, 12, FColor::Green, false, 2.0f);
		DrawDebugString(GetWorld(), Location, "BackFloor", nullptr, FColor::Green, 2.0f);

		FVector Forward = Rotation.Vector();
		DrawDebugLine(GetWorld(), Location, Location + Forward * 100.f, FColor::Blue, false, 2.f, 0, 2.f);
	}
}
#endif
