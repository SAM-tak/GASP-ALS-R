// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/Actions/GarGameplayAbility_Traversal.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "ChooserFunctionLibrary.h"
#include "AnimationWarpingLibrary.h"
#include "MotionWarpingComponent.h"
#include "GarCharacter.h"
#include "GarCharacterMovementComponent.h"
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

	const auto* Capsule{Character->GetCapsuleComponent()};

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
		0.5f * CapsuleScale - UCharacterMovementComponent::MAX_FLOOR_DIST;

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

	Params.FrontLedgeLocation = ForwardTraceHit.ImpactPoint;
	Params.FrontLedgeNormal = ForwardTraceHit.ImpactNormal;

	if (FMath::IsNearlyEqual(ForwardTraceHit.ImpactPoint.Z, ForwardTraceStart.Z, 0.001f))
	{
		static const FName FrontLedgeTraceTag{ FString::Printf(TEXT("%hs (Front Ledge Trace)"), __FUNCTION__) };

		const FVector Projected{
			(FVector::UpVector - FVector::DotProduct(FVector::UpVector, ForwardTraceHit.ImpactNormal) * ForwardTraceHit.ImpactNormal).GetSafeNormal()
		};

		const FVector FrontLedgeTraceStart{
			ForwardTraceHit.ImpactPoint + Projected * CapsuleHalfHeight
			+ FVector{0.f, 0.f, (CapsuleBottomLocation.Z + LedgeHeightDelta + 2.5f * CapsuleRadius + UCharacterMovementComponent::MIN_FLOOR_DIST)}
		};

		const FVector FrontLedgeTraceEnd{
			FrontLedgeTraceStart
			- FVector{0.f, 0.f, (TraceSettings.LedgeHeight.GetMin() * CapsuleScale + CapsuleRadius + UCharacterMovementComponent::MAX_FLOOR_DIST)}
		};

		FHitResult FrontLedgeTraceHit;
		World->SweepSingleByChannel(FrontLedgeTraceHit, FrontLedgeTraceStart, FrontLedgeTraceEnd, FQuat::Identity,
			TraversalTraceChannel, FCollisionShape::MakeSphere(CapsuleRadius),
			{ FrontLedgeTraceTag, false, Character }, TraversalTraceResponses);
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UGarUtility::DrawDebugSweepSingleCapsuleAlternative(World, FrontLedgeTraceStart, FrontLedgeTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, false, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);
		}
#endif
		if(!FrontLedgeTraceHit.IsValidBlockingHit())
		{
			Params.FrontLedgeLocation = FrontLedgeTraceHit.ImpactPoint;
		}
	}

	// Trace downward from the first trace's impact point and determine if the hit location is walkable.

	const auto TargetDirection{-ForwardTraceHit.ImpactNormal.GetSafeNormal2D()};

	static const FName DownwardTraceTag{FString::Printf(TEXT("%hs (Downward Trace)"), __FUNCTION__)};

	const FVector2D TargetLocationOffset{TargetDirection * (TraceSettings.TargetLocationOffset * CapsuleScale)};

	const FVector DownwardTraceStart{
		Params.FrontLedgeLocation.X + TargetLocationOffset.X,
		Params.FrontLedgeLocation.Y + TargetLocationOffset.Y,
		CapsuleBottomLocation.Z + LedgeHeightDelta + 2.5f * TraceCapsuleRadius + UCharacterMovementComponent::MIN_FLOOR_DIST
	};

	const FVector DownwardTraceEnd{
		DownwardTraceStart.X,
		DownwardTraceStart.Y,
		CapsuleBottomLocation.Z +
		TraceSettings.LedgeHeight.GetMin() * CapsuleScale + TraceCapsuleRadius - UCharacterMovementComponent::MAX_FLOOR_DIST
	};

	FHitResult DownwardTraceHit;
	World->SweepSingleByChannel(DownwardTraceHit, DownwardTraceStart, DownwardTraceEnd, FQuat::Identity,
	                            TraversalTraceChannel, FCollisionShape::MakeSphere(TraceCapsuleRadius),
	                            {DownwardTraceTag, false, Character}, TraversalTraceResponses);

	auto TargetPrimitive{DownwardTraceHit.GetComponent()};

	if (!DownwardTraceHit.IsValidBlockingHit() ||
		HasNonTraversalTag(DownwardTraceHit) ||
	    !IsValid(TargetPrimitive) ||
		TargetPrimitive->GetComponentVelocity().SizeSquared() > FMath::Square(TargetPrimitiveSpeedThreshold) ||
	    !TargetPrimitive->CanCharacterStepUp(Character) ||
		Character->GetCharacterMovement()->IsWalkable(ForwardTraceHit))
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

	Params.TargetPrimitive = TargetPrimitive;

	const auto SlopeAngleCos{UE_REAL_TO_FLOAT(DownwardTraceHit.ImpactNormal.Z)};

	// The approximate slope angle is used in situations where the normal slope angle cannot convey
	// the true nature of the surface slope, for example, for a 45 degree staircase the slope
	// angle will always be 90 degrees, while the approximate slope angle will be ~45 degrees.

	auto ApproximateSlopeNormal{DownwardTraceHit.Location - DownwardTraceHit.ImpactPoint};
	ApproximateSlopeNormal.Normalize();

	const auto ApproximateSlopeAngleCos{UE_REAL_TO_FLOAT(ApproximateSlopeNormal.Z)};

	if (SlopeAngleCos < SlopeAngleThresholdCos ||
	    ApproximateSlopeAngleCos < SlopeAngleThresholdCos ||
	    !Character->GetCharacterMovement()->IsWalkable(DownwardTraceHit))
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UGarUtility::DrawDebugSweepSingleCapsuleAlternative(World, ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, true, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);

			UGarUtility::DrawDebugSweepSingleSphere(World, DownwardTraceStart, DownwardTraceEnd, TraceCapsuleRadius,
			                                        false, DownwardTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
			                                        TraceSettings.bDrawFailedTraces ? 7.5f : 0.0f);
		}
#endif

		return false;
	}

	// Check that there is enough free space for the capsule at the target location.

	static const FName TargetLocationTraceTag{FString::Printf(TEXT("%hs (Target Location Overlap)"), __FUNCTION__)};

	const FVector TargetLocation{DownwardTraceHit.ImpactPoint + FVector{0.0f, 0.0f, UCharacterMovementComponent::MIN_FLOOR_DIST}};

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

			UGarUtility::DrawDebugSweepSingleSphere(World, DownwardTraceStart, DownwardTraceEnd, TraceCapsuleRadius,
			                                        false, DownwardTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
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
		(DownwardTraceHit.Location.Z + DownwardTraceEnd.Z) * 0.5f
	};

	const auto StartLocationTraceCapsuleHalfHeight{(DownwardTraceHit.Location.Z - DownwardTraceEnd.Z) * 0.5f + TraceCapsuleRadius};

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

			UGarUtility::DrawDebugSweepSingleSphere(World, DownwardTraceStart, DownwardTraceEnd, TraceCapsuleRadius,
			                                        false, DownwardTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
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

		UGarUtility::DrawDebugSweepSingleSphere(World, DownwardTraceStart, DownwardTraceEnd,
		                                        TraceCapsuleRadius, true, DownwardTraceHit,
		                                        {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f}, 7.5f);
	}
#endif

	FGarTraversalChooserInputs ChooserInputs;
	ChooserInputs.bHasFrontLedge = true;
	ChooserInputs.bHasBackLedge = true;
	ChooserInputs.ObstacleHeight = UE_REAL_TO_FLOAT(TargetLocation.Z - CapsuleBottomLocation.Z);
	ChooserInputs.Speed = Character->GetVelocity().Size2D();

	Params.BackLedgeLocation = TargetLocation;

	static const FName EndLocationTraceTag{FString::Printf(TEXT("%hs (End Location Overlap)"), __FUNCTION__)};

	const FVector EndLocationTraceStart{TargetLocation + TargetDirection * (MinimumDepth + ChooserInputs.Speed * RisingTime)
		+ FVector{0.f, 0.f, TraceCapsuleRadius + UCharacterMovementComponent::MIN_FLOOR_DIST}};

	const FVector EndLocationTraceEnd{
		EndLocationTraceStart.X,
		EndLocationTraceStart.Y,
		Character->GetCharacterMovement()->GetActorFeetLocation().Z
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
		ChooserInputs.bHasBackFloor = true;
		Params.BackFloorLocation = EndLocationTraceHit.ImpactPoint;
		if(FVector::DistXY(EndLocationTraceHit.ImpactPoint, EndLocationTraceStart) >= 0.001f)
		{
			Params.BackLedgeLocation = EndLocationTraceHit.ImpactPoint;
		}
	}
	else
	{
		Params.BackFloorLocation = EndLocationTraceEnd;
	}

	ChooserInputs.ObstacleDepth = UE_REAL_TO_FLOAT(FVector::DistXY(Params.FrontLedgeLocation, Params.BackLedgeLocation));
	ChooserInputs.BackFloorFall = TargetLocation.Z - Params.BackFloorLocation.Z;
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

		MotionWarpingComponent = Character->GetMotionWarping();

		if (!ensure(MotionWarpingComponent.IsValid()))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

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
		MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(FName(TEXTVIEW("FrontLedge")), Parameters.TargetPrimitive.Get(),
			NAME_None, true, EWarpTargetLocationOffsetDirection::WorldSpace,
			Parameters.FrontLedgeLocation + FVector(0.f, 0.f, 0.5f),
			FRotationMatrix::MakeFromX(-Parameters.FrontLedgeNormal).Rotator());

		// If the action type was a hurdle or a vault, we need to also update the BackLedge target. If it is not a hurdle or vault, remove it.
		if (Parameters.ChooserOutput.Tags.HasTag(GarTraversalActionTags::Hurdle) || Parameters.ChooserOutput.Tags.HasTag(GarTraversalActionTags::Vault))
		{
			// Because the traversal animations move at different distances (no fixed metrics),
			// we need to know how far the animation moves in order to warp it properly.
			// Here we cache a curve value at the end of the Back Ledge warp window to determine
			// how far the animation is from the front ledge once the character reaches the back ledge location in the animation.
			TArray<FMotionWarpingWindowData> Windows;
			UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(Montage, FName(TEXTVIEW("BackLedge")), Windows);
			if (!Windows.IsEmpty())
			{
				UAnimationWarpingLibrary::GetCurveValueFromAnimation(Montage, FName(TEXTVIEW("Distance_From_Ledge")), Windows[0].StartTime,
					AnimatedDistanceFromFrontLedgeToBackLedge);
				// Update the BackLedge warp target.
				MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(FName(TEXTVIEW("BackLedge")), Parameters.TargetPrimitive.Get(),
					NAME_None, true, EWarpTargetLocationOffsetDirection::WorldSpace,
					Parameters.BackLedgeLocation, FRotator::ZeroRotator);
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
		if (Parameters.ChooserOutput.Tags.HasTag(GarTraversalActionTags::Hurdle))
		{
			// Caches a curve value at the end of the Back Floor warp window to determine
			// how far the animation is from the front ledge once the character touches the ground.
			TArray<FMotionWarpingWindowData> Windows;
			UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(Montage, FName(TEXTVIEW("BackFloor")), Windows);
			if (!Windows.IsEmpty())
			{
				UAnimationWarpingLibrary::GetCurveValueFromAnimation(Montage, FName(TEXTVIEW("Distance_From_Ledge")), Windows[0].StartTime,
					AnimatedDistanceFromFrontLedgeToBackFloor);
				// Since the animations may land on the floor at different distances (a run hurdle may travel further than a walk or stand hurdle),
				// use the total animated distance away from the back ledge as the X and Y values of the BackFloor warp point.
				// This could technically cause some collision issues if the floor is not flat, or there is an bostacle in the way,
				// therefore having fixed metrics for all traversal animations would be an improvement.
				auto HLoc = Parameters.BackLedgeLocation + (Parameters.FrontLedgeNormal
					* FMath::Abs(AnimatedDistanceFromFrontLedgeToBackFloor - AnimatedDistanceFromFrontLedgeToBackLedge));
				MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(FName(TEXTVIEW("BackFloor")), Parameters.TargetPrimitive.Get(),
					NAME_None, true, EWarpTargetLocationOffsetDirection::WorldSpace,
					FVector(HLoc.X, HLoc.Y, Parameters.BackFloorLocation.Z), FRotator::ZeroRotator);
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

		CurrentTargetPrimitive = Parameters.TargetPrimitive;
	}
	else
	{
		Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	}

	AbilitySystem->AddLooseGameplayTags(Parameters.ChooserOutput.Tags);
	ActionTags = Parameters.ChooserOutput.Tags;

	// Disable collision during the traversal action to ensure the character aligns properly with the obstacle.

	SavedCollisionEnabled = Character->GetCapsuleComponent()->GetCollisionEnabled();
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Reset network smoothing.

	Character->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;

	Character->GetMesh()->SetRelativeLocationAndRotation(Character->GetBaseTranslationOffset(),
		Character->GetMesh()->IsUsingAbsoluteRotation()
		? Character->GetActorQuat() * Character->GetBaseRotationOffset()
		: Character->GetBaseRotationOffset(), false, nullptr, ETeleportType::TeleportPhysics);

	// Clear the character movement mode and set the locomotion action to traverse.

	Character->GetCharacterMovement()->FlushServerMoves();
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Custom);
	Character->GetGarCharacterMovement()->SetMovementModeLocked(true);
	Character->GetCharacterMovement()->SetBase(CurrentTargetPrimitive.Get());
	Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;

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
	auto* CharacterMovement{Character->GetGarCharacterMovement()};

	if (CharacterMovement->MovementMode != MOVE_Custom)
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

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
	auto CharacterMovement{Character->GetGarCharacterMovement()};
	auto* AbilitySystem{GetGarAbilitySystemComponentFromActorInfo()};

	AbilitySystem->RemoveLooseGameplayTags(ActionTags);

	CharacterMovement->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;

	CharacterMovement->SetMovementModeLocked(false);
	if (CharacterMovement->MovementMode == MOVE_Custom)
	{
		CharacterMovement->SetMovementMode(MOVE_Walking);
	}

	Character->ForceNetUpdate();
	Character->GetCapsuleComponent()->SetCollisionEnabled(SavedCollisionEnabled);
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
