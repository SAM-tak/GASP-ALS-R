#include "GarAnimationInstance.h"

#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GarAnimationInstanceProxy.h"
#include "GarCharacter.h"
#include "GarConstants.h"
#include "LinkedAnimLayers/GarLayeringAnimInstance.h"
#include "LinkedAnimLayers/GarViewAnimInstance.h"
#include "LinkedAnimLayers/GarRagdollingAnimInstance.h"
#include "Settings/GarAnimationInstanceSettings.h"
#include "Abilities/Actions/GarGameplayAbility_Ragdolling.h"
#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimationInstance)

void UGarAnimationInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AGarCharacter>(GetOwningActor());

#if WITH_EDITOR
	if (!GetWorld()->IsGameWorld() && !Character.IsValid())
	{
		// Use default objects for editor preview.

		Character = GetMutableDefault<AGarCharacter>();
	}
#endif
}

void UGarAnimationInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	LayeringAnimInstance = Cast<UGarLayeringAnimInstance>(GetLinkedAnimGraphInstanceByTag(FName{TEXTVIEW("Layering")}));
	ViewAnimInstance = Cast<UGarViewAnimInstance>(GetLinkedAnimGraphInstanceByTag(FName{TEXTVIEW("View")}));
	RagdollingAnimInstance = Cast<UGarRagdollingAnimInstance>(GetLinkedAnimGraphInstanceByTag(FName{TEXTVIEW("Ragdolling")}));

	ensure(IsValid(Settings));
	ensure(IsValid(Settings->BoneNameTable));
	ensure(Character.IsValid());
	ensure(LayeringAnimInstance.IsValid());
	ensure(ViewAnimInstance.IsValid());
	ensure(RagdollingAnimInstance.IsValid());
}

void UGarAnimationInstance::NativeUpdateAnimation(const float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarAnimationInstance::NativeUpdateAnimation()"),
								STAT_UGarAnimationInstance_NativeUpdateAnimation, STATGROUP_Gar)

	Super::NativeUpdateAnimation(DeltaTime);

	if (!IsValid(Settings) || !IsValid(Settings->BoneNameTable) || !Character.IsValid())
	{
		return;
	}

	auto* Mesh{GetSkelMeshComponent()};

	if (Mesh->IsUsingAbsoluteRotation() && IsValid(Mesh->GetAttachParent()))
	{
		const auto& ParentTransform{Mesh->GetAttachParent()->GetComponentTransform()};

		// Manually synchronize mesh rotation with character rotation.

		Mesh->MoveComponent(FVector::ZeroVector, ParentTransform.GetRotation() * Character->GetBaseRotationOffset(), false);

		// Re-cache proxy transforms to match the modified mesh transform.

		const auto& Proxy{GetProxyOnGameThread<FAnimInstanceProxy>()};

		const_cast<FTransform&>(Proxy.GetComponentTransform()) = Mesh->GetComponentTransform();
		const_cast<FTransform&>(Proxy.GetComponentRelativeTransform()) = Mesh->GetRelativeTransform();
		const_cast<FTransform&>(Proxy.GetActorTransform()) = Character->GetActorTransform();
	}

#if WITH_EDITORONLY_DATA && ENABLE_DRAW_DEBUG
	bDisplayDebugTraces = UGarUtility::ShouldDisplayDebugForActor(Character.Get(), UGarConstants::TracesDebugDisplayName());
#endif

	PreviousGameplayTags = CurrentGameplayTags;
	Character->GetOwnedGameplayTags(CurrentGameplayTags);

	CharacterTransform = Mesh->GetComponentTransform();

	FaceRotationMode = Character->GetRotationMode();
	if (FaceRotationMode != GarRotationModeTags::Aiming)
	{
		FaceRotationMode = Character->GetDesiredRotationMode();
	}
	bIsActionRunning = Character->GetLocomotionAction().IsValid();

	CharacterZScale = UE_REAL_TO_FLOAT(GetSkelMeshComponent()->GetComponentScale().Z);

	LocomotionState = Character->GetLocomotionState();
	bMovingSmooth = (LocomotionState.bHasInput && LocomotionState.bHasSpeed)
				  || LocomotionState.Speed > Settings->MovingSmoothSpeedThreshold;

	RefreshMovementBaseOnGameThread();

	if (ViewAnimInstance.IsValid())
	{
		ViewAnimInstance->RefreshOnGameThread(DeltaTime);
	}
	const auto& View{Character->GetViewState()};
	ViewRotation = View.LookRotation; // same as ViewAnimInstance->Rotation
	ViewYawSpeed = View.YawSpeed;

	RefreshCharacterMovementOnGameThread(DeltaTime);
	RefreshFeetOnGameThread();
}

void UGarAnimationInstance::NativeThreadSafeUpdateAnimation(const float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarAnimationInstance::NativeThreadSafeUpdateAnimation()"),
								STAT_UGarAnimationInstance_NativeThreadSafeUpdateAnimation, STATGROUP_Gar)

	Super::NativeThreadSafeUpdateAnimation(DeltaTime);

	if (!IsValid(Settings) || !IsValid(Settings->BoneNameTable) || !Character.IsValid())
	{
		return;
	}

	if (LayeringAnimInstance.IsValid())
	{
		LayeringAnimInstance->Refresh();
	}

	RefreshPose();

	if (ViewAnimInstance.IsValid())
	{
		ViewAnimInstance->Refresh(DeltaTime);
		ViewYawAngle = ViewAnimInstance->YawAngle;
	}
	else
	{
		ViewYawAngle = 0.0f;
	}

	RefreshFeet(DeltaTime);
}

void UGarAnimationInstance::NativePostUpdateAnimation()
{
	// Can't use UAnimationInstance::NativePostEvaluateAnimation() instead this function, as it will not be called if
	// USkinnedMeshComponent::VisibilityBasedAnimTickOption is set to EVisibilityBasedAnimTickOption::AlwaysTickPose.

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarAnimationInstance::NativePostUpdateAnimation()"),
								STAT_UGarAnimationInstance_NativePostUpdateAnimation, STATGROUP_Gar)

	if (!IsValid(Settings) || !IsValid(Settings->BoneNameTable) || !Character.IsValid())
	{
		return;
	}

	for (const auto& RequestFunction : RequestQueue)
	{
		RequestFunction();
	}

	RequestQueue.Reset();

	bPendingUpdate = false;
}

FAnimInstanceProxy* UGarAnimationInstance::CreateAnimInstanceProxy()
{
	return new FGarAnimationInstanceProxy{this};
}

FGarControlRigInput UGarAnimationInstance::GetControlRigInput() const
{
	return {
		ViewAnimInstance.IsValid() ? ViewAnimInstance->SpineRotation.YawAngle : 0.0f,
		FeetState.Left.IkRotation,
		FeetState.Left.IkLocation,
		FeetState.Left.IkAmount,
		FeetState.Right.IkRotation,
		FeetState.Right.IkLocation,
		FeetState.Right.IkAmount,
		FeetState.MinMaxPelvisOffsetZ,
	};
}

void UGarAnimationInstance::RefreshMovementBaseOnGameThread()
{
	const auto& BasedMovement{Character->GetBasedMovement()};

	if (BasedMovement.MovementBase != MovementBase.Primitive || BasedMovement.BoneName != MovementBase.BoneName)
	{
		MovementBase.Primitive = BasedMovement.MovementBase;
		MovementBase.BoneName = BasedMovement.BoneName;
		MovementBase.bBaseChanged = true;
	}
	else
	{
		MovementBase.bBaseChanged = false;
	}

	MovementBase.bHasRelativeLocation = BasedMovement.HasRelativeLocation();
	MovementBase.bHasRelativeRotation = MovementBase.bHasRelativeLocation & BasedMovement.bRelativeRotation;

	const auto PreviousRotation{MovementBase.Rotation};

	MovementBaseUtility::GetMovementBaseTransform(BasedMovement.MovementBase, BasedMovement.BoneName,
												  MovementBase.Location, MovementBase.Rotation);

	MovementBase.DeltaRotation = MovementBase.bHasRelativeLocation && !MovementBase.bBaseChanged
									 ? (MovementBase.Rotation * PreviousRotation.Inverse()).Rotator()
									 : FRotator::ZeroRotator;
}

void UGarAnimationInstance::RefreshPose()
{
	const auto& Curves{GetProxyOnAnyThread<FGarAnimationInstanceProxy>().GetAnimationCurves(EAnimCurveType::AttributeCurve)};

	static const auto GetCurveValue{
		[](const TMap<FName, float>& Curves, const FName& CurveName) -> float
		{
			const auto* Value{Curves.Find(CurveName)};

			return Value != nullptr ? *Value : 0.0f;
		}
	};

	PoseState.GroundedAmount = GetCurveValue(Curves, UGarConstants::PoseGroundedCurveName());
	PoseState.InAirAmount = GetCurveValue(Curves, UGarConstants::PoseInAirCurveName());

	PoseState.StandingAmount = GetCurveValue(Curves, UGarConstants::PoseStandingCurveName());
	PoseState.CrouchingAmount = GetCurveValue(Curves, UGarConstants::PoseCrouchingCurveName());

	PoseState.MovingAmount = GetCurveValue(Curves, UGarConstants::PoseMovingCurveName());

	PoseState.GaitAmount = FMath::Clamp(GetCurveValue(Curves, UGarConstants::PoseGaitCurveName()), 0.0f, 3.0f);
	PoseState.GaitWalkingAmount = UGarMath::Clamp01(PoseState.GaitAmount);
	PoseState.GaitRunningAmount = UGarMath::Clamp01(PoseState.GaitAmount - 1.0f);
	PoseState.GaitSprintingAmount = UGarMath::Clamp01(PoseState.GaitAmount - 2.0f);

	// Use the grounded pose curve value to "unweight" the gait pose curve. This is used to
	// instantly get the full gait value from the very beginning of transitions to grounded states.

	PoseState.UnweightedGaitAmount = PoseState.GroundedAmount > 0.0f
										 ? PoseState.GaitAmount / PoseState.GroundedAmount
										 : PoseState.GaitAmount;

	PoseState.UnweightedGaitWalkingAmount = UGarMath::Clamp01(PoseState.UnweightedGaitAmount);
	PoseState.UnweightedGaitRunningAmount = UGarMath::Clamp01(PoseState.UnweightedGaitAmount - 1.0f);
	PoseState.UnweightedGaitSprintingAmount = UGarMath::Clamp01(PoseState.UnweightedGaitAmount - 2.0f);
}

bool UGarAnimationInstance::IsSpineRotationAllowed()
{
	return !CurrentGameplayTags.HasTag(GarRotationModeTags::VelocityDirection);
}

void UGarAnimationInstance::RefreshCharacterMovementOnGameThread(float DeltaTime)
{
	check(IsInGameThread())

	const auto* Movement{Character->GetCharacterMovement()};

	CharacterMovement.Acceleration = Movement->GetCurrentAcceleration();
	CharacterMovement.MaxAcceleration = Movement->GetMaxAcceleration();
	CharacterMovement.MaxBrakingDeceleration = Movement->GetMaxBrakingDeceleration();
	CharacterMovement.VelocityLastFrame = CharacterMovement.Velocity;
	CharacterMovement.Velocity = Movement->Velocity;
	CharacterMovement.VelocityAcceleration = (CharacterMovement.Velocity - CharacterMovement.VelocityLastFrame) / FMath::Max(DeltaTime, 0.001f);

	if (FVector2D(CharacterMovement.Velocity).Length() > 5.0f)
	{
		CharacterMovement.LastNonZeroVelocity = CharacterMovement.Velocity;
	}

	CharacterMovement.WalkableFloorZ = Movement->GetWalkableFloorZ();
}

void UGarAnimationInstance::RefreshFeetOnGameThread()
{
	check(IsInGameThread())

	const auto* Mesh{GetSkelMeshComponent()};

	const auto FootLeftTargetTransform{Mesh->GetSocketTransform(Settings->BoneNameTable->FootLeftVirtualBoneName)};

	FeetState.Left.TargetLocation = FootLeftTargetTransform.GetLocation();
	FeetState.Left.TargetRotation = FootLeftTargetTransform.GetRotation();

	const auto FootRightTargetTransform{Mesh->GetSocketTransform(Settings->BoneNameTable->FootRightVirtualBoneName)};

	FeetState.Right.TargetLocation = FootRightTargetTransform.GetLocation();
	FeetState.Right.TargetRotation = FootRightTargetTransform.GetRotation();
}

void UGarAnimationInstance::RefreshFeet(const float DeltaTime)
{
	FeetState.FootPlantedAmount = FMath::Clamp(GetCurveValue(UGarConstants::FootPlantedCurveName()), -1.0f, 1.0f);
	FeetState.FeetCrossingAmount = GetCurveValueClamped01(UGarConstants::FeetCrossingCurveName());

	FeetState.MinMaxPelvisOffsetZ = FVector2f::ZeroVector;

	const auto ComponentTransformInverse{GetProxyOnAnyThread<FAnimInstanceProxy>().GetComponentTransform().Inverse()};

	RefreshFoot(FeetState.Left, UGarConstants::FootLeftIkCurveName(), Settings->Feet.LeftFootLimits, ComponentTransformInverse, DeltaTime);

	RefreshFoot(FeetState.Right, UGarConstants::FootRightIkCurveName(), Settings->Feet.RightFootLimits, ComponentTransformInverse, DeltaTime);

	FeetState.MinMaxPelvisOffsetZ.X = UE_REAL_TO_FLOAT(
		FMath::Min(FeetState.Left.OffsetTargetLocationZ, FeetState.Right.OffsetTargetLocationZ) / CharacterZScale);

	FeetState.MinMaxPelvisOffsetZ.Y = UE_REAL_TO_FLOAT(
		FMath::Max(FeetState.Left.OffsetTargetLocationZ, FeetState.Right.OffsetTargetLocationZ) / CharacterZScale);
}

void UGarAnimationInstance::RefreshFoot(FGarFootState& FootState, const FName& FootIkCurveName, const FGarFootLimitsSettings& LimitsSettings,
										const FTransform& ComponentTransformInverse, const float DeltaTime) const
{
	FootState.IkAmount = GetCurveValueClamped01(FootIkCurveName);

	auto FinalLocation{FootState.TargetLocation};
	auto FinalRotation{FootState.TargetRotation};

	const auto PreviousFinalRotation{FinalRotation};
	RefreshFootOffset(FootState, DeltaTime, FinalLocation, FinalRotation);

	// Prevent the foot from assuming an unnatural pose when on a highly
	// sloped surface by limiting its rotation after applying a foot offset.

	LimitFootRotation(LimitsSettings, PreviousFinalRotation, FinalRotation);

	FootState.IkLocation = ComponentTransformInverse.TransformPosition(FinalLocation);
	FootState.IkRotation = ComponentTransformInverse.TransformRotation(FinalRotation);
}

void UGarAnimationInstance::RefreshFootOffset(FGarFootState& FootState, const float DeltaTime,
											  FVector& FinalLocation, FQuat& FinalRotation) const
{
	if (!FAnimWeight::IsRelevant(FootState.IkAmount))
	{
		FootState.OffsetTargetLocationZ = 0.0f;
		FootState.OffsetTargetRotation = FQuat::Identity;
		FootState.OffsetSpringState.Reset();
		FootState.Hit.Init();
		return;
	}

	if (CurrentGameplayTags.HasTag(GarLocomotionModeTags::InAir))
	{
		FootState.OffsetTargetLocationZ = 0.0f;
		FootState.OffsetTargetRotation = FQuat::Identity;
		FootState.OffsetSpringState.Reset();

		if (bPendingUpdate)
		{
			FootState.OffsetLocationZ = 0.0f;
			FootState.OffsetRotation = FQuat::Identity;
		}
		else
		{
			static constexpr auto InterpolationSpeed{15.0f};

			FootState.OffsetLocationZ = FMath::FInterpTo(FootState.OffsetLocationZ, 0.0f, DeltaTime, InterpolationSpeed);
			FootState.OffsetRotation = FMath::QInterpTo(FootState.OffsetRotation, FQuat::Identity, DeltaTime, InterpolationSpeed);

			FinalLocation.Z += FootState.OffsetLocationZ;
			FinalRotation = FootState.OffsetRotation * FinalRotation;
		}
		FootState.Hit.Init();
		return;
	}

	// Trace downward from the foot location to find the geometry. If the surface is walkable, save the impact location and normal.

	const FVector TraceLocation{
		FinalLocation.X, FinalLocation.Y, GetProxyOnAnyThread<FAnimInstanceProxy>().GetComponentTransform().GetLocation().Z
	};
	
	bool bGroundValid{FootState.Hit.IsValidBlockingHit() && FootState.Hit.ImpactNormal.Z >= CharacterMovement.WalkableFloorZ};

	FTraceDelegate TraceDelegate = FTraceDelegate::CreateWeakLambda(this, [&FootState](const FTraceHandle& Handle, FTraceDatum& Data) mutable
	{
		if (Data.OutHits.Num() > 0)
		{
			FootState.Hit = Data.OutHits[0];
		}
		else
		{
			FootState.Hit.Init();
		}
	});
	if (IsInGameThread())
	{
		GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single,
											TraceLocation + FVector{
												0.0f, 0.0f, Settings->Feet.IkTraceDistanceUpward * CharacterZScale
											},
											TraceLocation - FVector{
												0.0f, 0.0f, Settings->Feet.IkTraceDistanceDownward * CharacterZScale
											},
											Settings->Feet.IkTraceChannel, {__FUNCTION__, true, Character.Get()},
											FCollisionResponseParams::DefaultResponseParam, &TraceDelegate);

#if WITH_EDITORONLY_DATA && ENABLE_DRAW_DEBUG
		if (bDisplayDebugTraces)
		{
			UGarUtility::DrawDebugLineTraceSingle(GetWorld(), FootState.Hit.TraceStart, FootState.Hit.TraceEnd, bGroundValid, FootState.Hit,
												  {0.0f, 0.25f, 1.0f}, {0.0f, 0.75f, 1.0f});
		}
#endif
	}
	else
	{
		RequestQueue.Emplace([this, TraceDelegate, TraceLocation
#if WITH_EDITORONLY_DATA && ENABLE_DRAW_DEBUG
							, &FootState, bGroundValid
#endif
		]
		{
			GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single,
												TraceLocation + FVector{
													0.0f, 0.0f, Settings->Feet.IkTraceDistanceUpward * CharacterZScale
												},
												TraceLocation - FVector{
													0.0f, 0.0f, Settings->Feet.IkTraceDistanceDownward * CharacterZScale
												},
												Settings->Feet.IkTraceChannel, {__FUNCTION__, true, Character.Get()},
												FCollisionResponseParams::DefaultResponseParam, &TraceDelegate);
#if WITH_EDITORONLY_DATA && ENABLE_DRAW_DEBUG
			if (bDisplayDebugTraces)
			{
				UGarUtility::DrawDebugLineTraceSingle(GetWorld(), FootState.Hit.TraceStart, FootState.Hit.TraceEnd, bGroundValid, FootState.Hit,
													  {0.0f, 0.25f, 1.0f}, {0.0f, 0.75f, 1.0f});
			}
#endif
		});
	}

	if (bGroundValid)
	{
		const auto SlopeAngleCos{UE_REAL_TO_FLOAT(FootState.Hit.ImpactNormal.Z)};

		const auto FootHeight{Settings->Feet.FootHeight * CharacterZScale};
		const auto FootHeightOffset{SlopeAngleCos > UE_SMALL_NUMBER ? FootHeight / SlopeAngleCos - FootHeight : 0.0f};

		// Find the difference between the impact location and the expected (flat) floor location.
		// These values are offset by the foot height to get better behavior on sloped surfaces.

		FootState.OffsetTargetLocationZ = FootState.Hit.ImpactPoint.Z - TraceLocation.Z + FootHeightOffset;

		// Calculate the rotation offset.

		FootState.OffsetTargetRotation = FQuat::FindBetweenNormals(FVector::UpVector, FootState.Hit.ImpactNormal);
	}

	// Interpolate current offsets to the new target values.

	if (bPendingUpdate)
	{
		FootState.OffsetSpringState.Reset();

		FootState.OffsetLocationZ = FootState.OffsetTargetLocationZ;
		FootState.OffsetRotation = FootState.OffsetTargetRotation;
	}
	else
	{
		static constexpr auto LocationInterpolationFrequency{0.4f};
		static constexpr auto LocationInterpolationDampingRatio{4.0f};
		static constexpr auto LocationInterpolationTargetVelocityAmount{1.0f};

		FootState.OffsetLocationZ = UGarMath::SpringDampFloat(FootState.OffsetLocationZ, FootState.OffsetTargetLocationZ,
															  FootState.OffsetSpringState, DeltaTime, LocationInterpolationFrequency,
															  LocationInterpolationDampingRatio, LocationInterpolationTargetVelocityAmount);

		static constexpr auto RotationInterpolationSpeed{30.0f};

		FootState.OffsetRotation = FMath::QInterpTo(FootState.OffsetRotation, FootState.OffsetTargetRotation,
													DeltaTime, RotationInterpolationSpeed);
	}

	FinalLocation.Z += FootState.OffsetLocationZ;
	FinalRotation = FootState.OffsetRotation * FinalRotation;
}

void UGarAnimationInstance::LimitFootRotation(const FGarFootLimitsSettings& LimitsSettings,
											  const FQuat& ParentRotation, FQuat& Rotation) const
{
	const auto RelativeRotation{ParentRotation.Inverse() * Rotation};

	FQuat Swing;
	FQuat Twist;
	RelativeRotation.ToSwingTwist(FVector{LimitsSettings.TwistAxis}, Swing, Twist);

	// Limit swing.

	const auto SwingLimitOffset{FQuat{LimitsSettings.SwingLimitOffsetQuaternion}};

	Swing = SwingLimitOffset * Swing;

	// Clamp a point with Swing.Y and Swing.Z coordinates to an ellipse with LimitsSettings.Swing2Limit
	// and LimitsSettings.Swing1Limit dimensions. A simplified and not very accurate algorithm is used here,
	// but it is enough for our needs. To get a more accurate result, you can use an algorithm similar
	// to the one used in Chaos::NearPointOnEllipse() or FRigUnit_SphericalPoseReader::DistanceToEllipse().

	FVector2D SwingLimit{Swing.Y, Swing.Z};
	SwingLimit.Normalize();

	SwingLimit.X = FMath::Abs(SwingLimit.X * LimitsSettings.Swing2Limit);
	SwingLimit.Y = FMath::Abs(SwingLimit.Y * LimitsSettings.Swing1Limit);

	const auto NewSwingY{FMath::Sign(Swing.Y) * FMath::Min(FMath::Abs(Swing.Y), SwingLimit.X)};
	const auto NewSwingZ{FMath::Sign(Swing.Z) * FMath::Min(FMath::Abs(Swing.Z), SwingLimit.Y)};

	FQuat NewSwing{
		0.0f, NewSwingY, NewSwingZ, FMath::Sqrt(FMath::Max(0.0f, 1.0f - NewSwingY * NewSwingY - NewSwingZ * NewSwingZ))
	};

	NewSwing = SwingLimitOffset.Inverse() * NewSwing;

	// Limit twist.

	const auto NewTwistX{FMath::Sign(Twist.X) * FMath::Min(FMath::Abs(Twist.X), LimitsSettings.TwistLimit)};

	const FQuat NewTwist(NewTwistX, 0.0f, 0.0f, FMath::Sqrt(FMath::Max(0.0f, 1.0f - NewTwistX * NewTwistX)));

	Rotation = ParentRotation * (NewSwing * NewTwist);
}

float UGarAnimationInstance::GetCurveValueClamped01(const FName& CurveName) const
{
	return UGarMath::Clamp01(GetCurveValue(CurveName));
}
