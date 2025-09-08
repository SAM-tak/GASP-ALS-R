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

	LayeringAnimInstance = Cast<UGarLayeringAnimInstance>(GetLinkedAnimGraphInstanceByTag(FName{TEXT("Layering")}));
	ViewAnimInstance = Cast<UGarViewAnimInstance>(GetLinkedAnimGraphInstanceByTag(FName{TEXT("View")}));
	RagdollingAnimInstance = Cast<UGarRagdollingAnimInstance>(GetLinkedAnimGraphInstanceByTag(FName{TEXT("Ragdolling")}));

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
	FaceRotationMode = Character->GetRotationMode();
	if (FaceRotationMode != GarRotationModeTags::Aiming)
	{
		FaceRotationMode = Character->GetDesiredRotationMode();
	}
	bIsActionRunning = Character->GetLocomotionAction().IsValid();

	RefreshMovementBaseOnGameThread();

	if (ViewAnimInstance.IsValid())
	{
		ViewAnimInstance->RefreshOnGameThread(DeltaTime);
	}
	const auto& View{Character->GetViewState()};
	ViewRotation = View.LookRotation; // same as ViewAnimInstance->Rotation
	ViewYawSpeed = View.YawSpeed;

	RefreshLocomotionOnGameThread();
	RefreshGroundedOnGameThread();
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

	RefreshGrounded(DeltaTime);
	RefreshFeet(DeltaTime);
	RefreshTransitions();
	RefreshRotateInPlace(DeltaTime);
	RefreshTurnInPlace(DeltaTime);
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

	PlayQueuedTransitionAnimation();
	PlayQueuedTurnInPlaceAnimation();
	StopQueuedTransitionAndTurnInPlaceAnimations();

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
		GroundedState.VelocityBlend.ForwardAmount,
		GroundedState.VelocityBlend.BackwardAmount,
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

void UGarAnimationInstance::RefreshLocomotionOnGameThread()
{
	check(IsInGameThread())

	const auto& Locomotion{Character->GetLocomotionState()};

	PrevLocomotionState = LocomotionState;

	LocomotionState.bHasInput = Locomotion.bHasInput;
	LocomotionState.InputYawAngle = Locomotion.InputYawAngle;

	LocomotionState.Speed = Locomotion.Speed;
	LocomotionState.Velocity = Locomotion.Velocity;
	LocomotionState.VelocityYawAngle = Locomotion.VelocityYawAngle;
	LocomotionState.Acceleration = Locomotion.Acceleration;

	const auto* Movement{Character->GetCharacterMovement()};

	LocomotionState.MaxAcceleration = Movement->GetMaxAcceleration();
	LocomotionState.MaxBrakingDeceleration = Movement->GetMaxBrakingDeceleration();
	LocomotionState.WalkableFloorZ = Movement->GetWalkableFloorZ();

	LocomotionState.bMoving = Locomotion.bMoving;

	LocomotionState.bMovingSmooth = (Locomotion.bHasInput && Locomotion.bHasSpeed) ||
									Locomotion.Speed > Settings->MovingSmoothSpeedThreshold;

	LocomotionState.TargetYawAngle = Locomotion.TargetYawAngle;
	LocomotionState.Location = Locomotion.Location;
	LocomotionState.Rotation = Locomotion.Rotation;
	LocomotionState.RotationQuaternion = Locomotion.RotationQuaternion;
	LocomotionState.YawSpeed = Locomotion.YawSpeed;

	LocomotionState.Scale = UE_REAL_TO_FLOAT(GetSkelMeshComponent()->GetComponentScale().Z);

	const auto* Capsule{Character->GetCapsuleComponent()};

	LocomotionState.CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	LocomotionState.CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
}

void UGarAnimationInstance::RefreshGroundedOnGameThread()
{
	check(IsInGameThread())

	GroundedState.bPivotActive = GroundedState.bPivotActivationRequested && !bPendingUpdate &&
								 LocomotionState.Speed < Settings->Grounded.PivotActivationSpeedThreshold;

	GroundedState.bPivotActivationRequested = false;
}

void UGarAnimationInstance::RefreshGrounded(const float DeltaTime)
{
	// Always sample sprint block curve, otherwise issues with inertial blending may occur.

	GroundedState.SprintBlockAmount = GetCurveValueClamped01(UGarConstants::SprintBlockCurveName());
	GroundedState.HipsDirectionLockAmount = FMath::Clamp(GetCurveValue(UGarConstants::HipsDirectionLockCurveName()), -1.0f, 1.0f);

	if (!CurrentGameplayTags.HasTag(GarLocomotionModeTags::Grounded))
	{
		GroundedState.VelocityBlend.bReinitializationRequired = true;
		GroundedState.SprintTime = 0.0f;
		return;
	}

	if (!LocomotionState.bMoving)
	{
		return;
	}

	// Calculate the relative acceleration amount. This value represents the current amount of acceleration / deceleration
	// relative to the character rotation. It is normalized to a range of -1 to 1 so that -1 equals the
	// max braking deceleration and 1 equals the max acceleration of the character movement component.

	FVector3f RelativeAccelerationAmount;

	if ((LocomotionState.Acceleration | LocomotionState.Velocity) >= 0.0f)
	{
		RelativeAccelerationAmount = UGarMath::ClampMagnitude01(
			FVector3f{LocomotionState.RotationQuaternion.UnrotateVector(LocomotionState.Acceleration)} /
			LocomotionState.MaxAcceleration);
	}
	else
	{
		RelativeAccelerationAmount = UGarMath::ClampMagnitude01(
			FVector3f{LocomotionState.RotationQuaternion.UnrotateVector(LocomotionState.Acceleration)} /
			LocomotionState.MaxBrakingDeceleration);
	}

	RefreshMovementDirection();
	RefreshVelocityBlend(DeltaTime);
	RefreshRotationYawOffsets();

	RefreshSprint(RelativeAccelerationAmount, DeltaTime);

	RefreshStrideBlendAmount();
	RefreshWalkRunBlendAmount();

	RefreshStandingPlayRate();
	RefreshCrouchingPlayRate();
}

void UGarAnimationInstance::RefreshMovementDirection()
{
	// Calculate the movement direction. This value represents the direction the character is moving relative
	// to the camera and is used in the cycle blending to blend to the appropriate directional states.

	if (CurrentGameplayTags.HasTag(GarGaitTags::Sprinting))
	{
		GroundedState.MovementDirection = EGarMovementDirection::Forward;
		return;
	}

	static constexpr auto ForwardHalfAngle{70.0f};

	GroundedState.MovementDirection = UGarMath::CalculateMovementDirection(
		FMath::UnwindDegrees(UE_REAL_TO_FLOAT(LocomotionState.VelocityYawAngle - ViewRotation.Yaw)),
		ForwardHalfAngle, 5.0f);
}

void UGarAnimationInstance::RefreshVelocityBlend(const float DeltaTime)
{
	GroundedState.VelocityBlend.bReinitializationRequired |= bPendingUpdate;

	// Calculate and interpolate the velocity blend amounts. This value represents the velocity amount of
	// the character in each direction (normalized so that diagonals equal 0.5 for each direction) and is
	// used in a blend multi node to produce better directional blending than a standard blend space.

	const auto RelativeVelocityDirection{
		FVector3f{LocomotionState.RotationQuaternion.UnrotateVector(LocomotionState.Velocity)}.GetSafeNormal()
	};

	const auto RelativeDirection{
		RelativeVelocityDirection /
		(FMath::Abs(RelativeVelocityDirection.X) + FMath::Abs(RelativeVelocityDirection.Y) + FMath::Abs(RelativeVelocityDirection.Z))
	};

	if (GroundedState.VelocityBlend.bReinitializationRequired)
	{
		GroundedState.VelocityBlend.bReinitializationRequired = false;

		GroundedState.VelocityBlend.ForwardAmount = UGarMath::Clamp01(RelativeDirection.X);
		GroundedState.VelocityBlend.BackwardAmount = FMath::Abs(FMath::Clamp(RelativeDirection.X, -1.0f, 0.0f));
		GroundedState.VelocityBlend.LeftAmount = FMath::Abs(FMath::Clamp(RelativeDirection.Y, -1.0f, 0.0f));
		GroundedState.VelocityBlend.RightAmount = UGarMath::Clamp01(RelativeDirection.Y);
	}
	else
	{
		GroundedState.VelocityBlend.ForwardAmount = FMath::FInterpTo(GroundedState.VelocityBlend.ForwardAmount,
																	 UGarMath::Clamp01(RelativeDirection.X), DeltaTime,
																	 Settings->Grounded.VelocityBlendInterpolationSpeed);

		GroundedState.VelocityBlend.BackwardAmount = FMath::FInterpTo(GroundedState.VelocityBlend.BackwardAmount,
																	  FMath::Abs(FMath::Clamp(RelativeDirection.X, -1.0f, 0.0f)), DeltaTime,
																	  Settings->Grounded.VelocityBlendInterpolationSpeed);

		GroundedState.VelocityBlend.LeftAmount = FMath::FInterpTo(GroundedState.VelocityBlend.LeftAmount,
																  FMath::Abs(FMath::Clamp(RelativeDirection.Y, -1.0f, 0.0f)), DeltaTime,
																  Settings->Grounded.VelocityBlendInterpolationSpeed);

		GroundedState.VelocityBlend.RightAmount = FMath::FInterpTo(GroundedState.VelocityBlend.RightAmount,
																   UGarMath::Clamp01(RelativeDirection.Y), DeltaTime,
																   Settings->Grounded.VelocityBlendInterpolationSpeed);
	}
}

void UGarAnimationInstance::RefreshRotationYawOffsets()
{
	// Set the rotation yaw offsets. These values influence the rotation yaw offset curve in the
	// animation graph and are used to offset the character's rotation for more natural movement.
	// The curves allow for fine control over how the offset behaves for each movement direction.

	const auto RotationYawOffset{FMath::UnwindDegrees(UE_REAL_TO_FLOAT(LocomotionState.VelocityYawAngle - ViewRotation.Yaw))};

	GroundedState.RotationYawOffsets.ForwardAngle = Settings->Grounded.RotationYawOffsetForwardCurve->GetFloatValue(RotationYawOffset);
	GroundedState.RotationYawOffsets.BackwardAngle = Settings->Grounded.RotationYawOffsetBackwardCurve->GetFloatValue(RotationYawOffset);
	GroundedState.RotationYawOffsets.LeftAngle = Settings->Grounded.RotationYawOffsetLeftCurve->GetFloatValue(RotationYawOffset);
	GroundedState.RotationYawOffsets.RightAngle = Settings->Grounded.RotationYawOffsetRightCurve->GetFloatValue(RotationYawOffset);
}

void UGarAnimationInstance::RefreshSprint(const FVector3f& RelativeAccelerationAmount, const float DeltaTime)
{
	if (!CurrentGameplayTags.HasTag(GarGaitTags::Sprinting))
	{
		GroundedState.SprintTime = 0.0f;
		GroundedState.SprintAccelerationAmount = 0.0f;
		return;
	}

	// Use the relative acceleration as the sprint relative acceleration if less than 0.5 seconds has
	// elapsed since the start of the sprint, otherwise set the sprint relative acceleration to zero.
	// This is necessary to apply the acceleration animation only at the beginning of the sprint.

	static constexpr auto TimeThreshold{0.5f};

	GroundedState.SprintTime = bPendingUpdate
								   ? TimeThreshold
								   : GroundedState.SprintTime + DeltaTime;

	GroundedState.SprintAccelerationAmount = GroundedState.SprintTime >= TimeThreshold
												 ? 0.0f
												 : RelativeAccelerationAmount.X;
}

void UGarAnimationInstance::RefreshStrideBlendAmount()
{
	// Calculate the stride blend amount. This value is used within the blend spaces to scale the stride (distance feet travel)
	// so that the character can walk or run at different movement speeds. It also allows the walk or run gait animations to
	// blend independently while still matching the animation speed to the movement speed, preventing the character from needing
	// to play a half walk + half run blend. The curves are used to map the stride amount to the speed for maximum control.

	const auto Speed{LocomotionState.Speed / LocomotionState.Scale};

	const auto StandingStrideBlend{
		FMath::Lerp(Settings->Grounded.StrideBlendAmountWalkCurve->GetFloatValue(Speed),
					Settings->Grounded.StrideBlendAmountRunCurve->GetFloatValue(Speed),
					PoseState.UnweightedGaitRunningAmount)
	};

	// Crouching stride blend amount.

	GroundedState.StrideBlendAmount = FMath::Lerp(StandingStrideBlend,
												  Settings->Grounded.StrideBlendAmountWalkCurve->GetFloatValue(Speed),
												  PoseState.CrouchingAmount);
}

void UGarAnimationInstance::RefreshWalkRunBlendAmount()
{
	// Calculate the walk run blend amount. This value is used within the blend spaces to blend between walking and running.

	GroundedState.WalkRunBlendAmount = CurrentGameplayTags.HasTag(GarGaitTags::Walking) ? 0.0f : 1.0f;
}

void UGarAnimationInstance::RefreshStandingPlayRate()
{
	// Calculate the standing play rate by dividing the character's speed by the animated speed for each gait.
	// The interpolation is determined by the gait amount curve that exists on every locomotion cycle so that
	// the play rate is always in sync with the currently blended animation. The value is also divided by the
	// stride blend and the capsule scale so that the play rate increases as the stride or scale gets smaller.

	const auto WalkRunSpeedAmount{
		FMath::Lerp(LocomotionState.Speed / Settings->Grounded.AnimatedWalkSpeed,
					LocomotionState.Speed / Settings->Grounded.AnimatedRunSpeed,
					PoseState.UnweightedGaitRunningAmount)
	};

	const auto WalkRunSprintSpeedAmount{
		FMath::Lerp(WalkRunSpeedAmount,
					LocomotionState.Speed / Settings->Grounded.AnimatedSprintSpeed,
					PoseState.UnweightedGaitSprintingAmount)
	};

	GroundedState.StandingPlayRate = FMath::Clamp(
		WalkRunSprintSpeedAmount / (GroundedState.StrideBlendAmount * LocomotionState.Scale), 0.0f, 3.0f);
}

void UGarAnimationInstance::RefreshCrouchingPlayRate()
{
	// Calculate the crouching play rate by dividing the character's speed by the animated speed. This value needs
	// to be separate from the standing play rate to improve the blend from crouching to standing while in motion.

	GroundedState.CrouchingPlayRate = FMath::Clamp(
		LocomotionState.Speed / (Settings->Grounded.AnimatedCrouchSpeed * GroundedState.StrideBlendAmount * LocomotionState.Scale),
		0.0f, 2.0f);
}

bool UGarAnimationInstance::IsFootLockInhibited() const
{
	return RotateInPlaceState.bFootLockInhibited || TurnInPlaceState.bFootLockInhibited;
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

	RefreshFoot(FeetState.Left, UGarConstants::FootLeftIkCurveName(), UGarConstants::FootLeftLockCurveName(),
				Settings->Feet.LeftFootLimits, ComponentTransformInverse, DeltaTime);

	RefreshFoot(FeetState.Right, UGarConstants::FootRightIkCurveName(), UGarConstants::FootRightLockCurveName(),
				Settings->Feet.RightFootLimits, ComponentTransformInverse, DeltaTime);

	FeetState.MinMaxPelvisOffsetZ.X = UE_REAL_TO_FLOAT(
		FMath::Min(FeetState.Left.OffsetTargetLocationZ, FeetState.Right.OffsetTargetLocationZ) / LocomotionState.Scale);

	FeetState.MinMaxPelvisOffsetZ.Y = UE_REAL_TO_FLOAT(
		FMath::Max(FeetState.Left.OffsetTargetLocationZ, FeetState.Right.OffsetTargetLocationZ) / LocomotionState.Scale);
}

void UGarAnimationInstance::RefreshFoot(FGarFootState& FootState, const FName& FootIkCurveName,
										const FName& FootLockCurveName, const FGarFootLimitsSettings& LimitsSettings,
										const FTransform& ComponentTransformInverse, const float DeltaTime) const
{
	FootState.IkAmount = GetCurveValueClamped01(FootIkCurveName);

	ProcessFootLockTeleport(FootState);

	ProcessFootLockBaseChange(FootState, ComponentTransformInverse);

	auto FinalLocation{FootState.TargetLocation};
	auto FinalRotation{FootState.TargetRotation};

	RefreshFootLock(FootState, FootLockCurveName, ComponentTransformInverse, DeltaTime, FinalLocation, FinalRotation);

	const auto PreviousFinalRotation{FinalRotation};
	RefreshFootOffset(FootState, DeltaTime, FinalLocation, FinalRotation);

	// Prevent the foot from assuming an unnatural pose when on a highly
	// sloped surface by limiting its rotation after applying a foot offset.

	LimitFootRotation(LimitsSettings, PreviousFinalRotation, FinalRotation);

	FootState.IkLocation = ComponentTransformInverse.TransformPosition(FinalLocation);
	FootState.IkRotation = ComponentTransformInverse.TransformRotation(FinalRotation);
}

void UGarAnimationInstance::ProcessFootLockTeleport(FGarFootState& FootState) const
{
	// Due to network smoothing, we assume that teleportation occurs over a short period of time, not
	// in one frame, since after accepting the teleportation event, the character can still be moved for
	// some indefinite time, and this must be taken into account in order to avoid foot locking glitches.

	if (bPendingUpdate || GetWorld()->TimeSince(TeleportedTime) > 0.2f ||
		!FAnimWeight::IsRelevant(FootState.IkAmount * FootState.LockAmount))
	{
		return;
	}

	const auto& ComponentTransform{GetProxyOnAnyThread<FAnimInstanceProxy>().GetComponentTransform()};

	FootState.LockLocation = ComponentTransform.TransformPosition(FootState.LockComponentRelativeLocation);
	FootState.LockRotation = ComponentTransform.TransformRotation(FootState.LockComponentRelativeRotation);

	if (MovementBase.bHasRelativeLocation)
	{
		const auto BaseRotationInverse{MovementBase.Rotation.Inverse()};

		FootState.LockMovementBaseRelativeLocation = BaseRotationInverse.RotateVector(FootState.LockLocation - MovementBase.Location);
		FootState.LockMovementBaseRelativeRotation = BaseRotationInverse * FootState.LockRotation;
	}
}

void UGarAnimationInstance::ProcessFootLockBaseChange(FGarFootState& FootState, const FTransform& ComponentTransformInverse) const
{
	if ((!bPendingUpdate && !MovementBase.bBaseChanged) || !FAnimWeight::IsRelevant(FootState.IkAmount * FootState.LockAmount))
	{
		return;
	}

	if (bPendingUpdate)
	{
		FootState.LockLocation = FootState.TargetLocation;
		FootState.LockRotation = FootState.TargetRotation;
	}

	FootState.LockComponentRelativeLocation = ComponentTransformInverse.TransformPosition(FootState.LockLocation);
	FootState.LockComponentRelativeRotation = ComponentTransformInverse.TransformRotation(FootState.LockRotation);

	if (MovementBase.bHasRelativeLocation)
	{
		const auto BaseRotationInverse{MovementBase.Rotation.Inverse()};

		FootState.LockMovementBaseRelativeLocation = BaseRotationInverse.RotateVector(FootState.LockLocation - MovementBase.Location);
		FootState.LockMovementBaseRelativeRotation = BaseRotationInverse * FootState.LockRotation;
	}
	else
	{
		FootState.LockMovementBaseRelativeLocation = FVector::ZeroVector;
		FootState.LockMovementBaseRelativeRotation = FQuat::Identity;
	}
}

void UGarAnimationInstance::RefreshFootLock(FGarFootState& FootState, const FName& FootLockCurveName,
											const FTransform& ComponentTransformInverse, const float DeltaTime,
											FVector& FinalLocation, FQuat& FinalRotation) const
{
	auto NewFootLockAmount{GetCurveValueClamped01(FootLockCurveName)};

	if (LocomotionState.bMovingSmooth || !CurrentGameplayTags.HasTag(GarLocomotionModeTags::Grounded))
	{
		// Smoothly disable foot locking if the character is moving or in the air,
		// instead of relying on the curve value from the animation blueprint.

		static constexpr auto MovingDecreaseSpeed{5.0f};
		static constexpr auto NotGroundedDecreaseSpeed{0.6f};

		NewFootLockAmount = bPendingUpdate
								? 0.0f
								: FMath::Max(0.0f, FMath::Min(NewFootLockAmount,
															  FootState.LockAmount - DeltaTime *
															  (LocomotionState.bMovingSmooth
																   ? MovingDecreaseSpeed
																   : NotGroundedDecreaseSpeed)));
	}

	if (Settings->Feet.bDisableFootLock || !FAnimWeight::IsRelevant(FootState.IkAmount * NewFootLockAmount))
	{
		if (FootState.LockAmount > 0.0f)
		{
			FootState.LockAmount = 0.0f;

			FootState.LockLocation = FVector::ZeroVector;
			FootState.LockRotation = FQuat::Identity;

			FootState.LockComponentRelativeLocation = FVector::ZeroVector;
			FootState.LockComponentRelativeRotation = FQuat::Identity;

			FootState.LockMovementBaseRelativeLocation = FVector::ZeroVector;
			FootState.LockMovementBaseRelativeRotation = FQuat::Identity;
		}

		return;
	}

	bool bNewAmountEqualOne{FAnimWeight::IsFullWeight(NewFootLockAmount)};
	bool bNewAmountGreaterThanPrevious{NewFootLockAmount > FootState.LockAmount};

	// Update the foot lock amount only if the new amount is less than the current amount or equal to 1. This
	// allows the foot to blend out from a locked location or lock to a new location, but never blend in.

	if (bNewAmountEqualOne)
	{
		if (bNewAmountGreaterThanPrevious)
		{
			// If the new foot lock amount is 1 and the previous amount is less than 1, then save the new foot lock location and rotation.

			if (FootState.LockAmount <= 0.9f)
			{
				// Keep the same lock location and rotation when the previous lock
				// amount is close to 1 to get rid of the foot "teleportation" issue.

				FootState.LockLocation = FinalLocation;
				FootState.LockRotation = FinalRotation;

				FootState.LockComponentRelativeLocation = ComponentTransformInverse.TransformPosition(FootState.LockLocation);
				FootState.LockComponentRelativeRotation = ComponentTransformInverse.TransformRotation(FootState.LockRotation);
			}

			if (MovementBase.bHasRelativeLocation)
			{
				const auto BaseRotationInverse{MovementBase.Rotation.Inverse()};

				FootState.LockMovementBaseRelativeLocation = BaseRotationInverse.RotateVector(FinalLocation - MovementBase.Location);
				FootState.LockMovementBaseRelativeRotation = BaseRotationInverse * FinalRotation;
			}
			else
			{
				FootState.LockMovementBaseRelativeLocation = FVector::ZeroVector;
				FootState.LockMovementBaseRelativeRotation = FQuat::Identity;
			}
		}

		FootState.LockAmount = 1.0f;
	}
	else if (!bNewAmountGreaterThanPrevious)
	{
		FootState.LockAmount = NewFootLockAmount;
	}

	if (IsFootLockInhibited())
	{
		// Inhibition is implemented by temporarily performing all calculations in component space rather
		// than in world space. So, the feet will still remain locked, but this time relative to the character.

		const auto& ComponentTransform{GetProxyOnAnyThread<FAnimInstanceProxy>().GetComponentTransform()};

		FootState.LockLocation = ComponentTransform.TransformPosition(FootState.LockComponentRelativeLocation);
		FootState.LockRotation = ComponentTransform.TransformRotation(FootState.LockComponentRelativeRotation);

		if (MovementBase.bHasRelativeLocation)
		{
			const auto BaseRotationInverse{MovementBase.Rotation.Inverse()};

			FootState.LockMovementBaseRelativeLocation = BaseRotationInverse.RotateVector(FootState.LockLocation - MovementBase.Location);
			FootState.LockMovementBaseRelativeRotation = BaseRotationInverse * FootState.LockRotation;
		}
	}
	else
	{
		if (MovementBase.bHasRelativeLocation)
		{
			FootState.LockLocation = MovementBase.Location + MovementBase.Rotation.RotateVector(FootState.LockMovementBaseRelativeLocation);
			FootState.LockRotation = MovementBase.Rotation * FootState.LockMovementBaseRelativeRotation;
		}

		FootState.LockComponentRelativeLocation = ComponentTransformInverse.TransformPosition(FootState.LockLocation);
		FootState.LockComponentRelativeRotation = ComponentTransformInverse.TransformRotation(FootState.LockRotation);
	}

	FinalLocation = FMath::Lerp(FinalLocation, FootState.LockLocation, FootState.LockAmount);
	FinalRotation = FQuat::Slerp(FinalRotation, FootState.LockRotation, FootState.LockAmount);
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
	
	bool bGroundValid{FootState.Hit.IsValidBlockingHit() && FootState.Hit.ImpactNormal.Z >= LocomotionState.WalkableFloorZ};

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
												0.0f, 0.0f, Settings->Feet.IkTraceDistanceUpward * LocomotionState.Scale
											},
											TraceLocation - FVector{
												0.0f, 0.0f, Settings->Feet.IkTraceDistanceDownward * LocomotionState.Scale
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
													0.0f, 0.0f, Settings->Feet.IkTraceDistanceUpward * LocomotionState.Scale
												},
												TraceLocation - FVector{
													0.0f, 0.0f, Settings->Feet.IkTraceDistanceDownward * LocomotionState.Scale
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

		const auto FootHeight{Settings->Feet.FootHeight * LocomotionState.Scale};
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

void UGarAnimationInstance::PlayQuickStopAnimation()
{
	if (!IsValid(Settings))
	{
		return;
	}

	if (!CurrentGameplayTags.HasTag(GarRotationModeTags::VelocityDirection))
	{
		PlayTransitionLeftAnimation(Settings->Transitions.QuickStopBlendInDuration, Settings->Transitions.QuickStopBlendOutDuration,
									Settings->Transitions.QuickStopPlayRate.X, Settings->Transitions.QuickStopStartTime);
		return;
	}

	auto RotationYawAngle{
		FMath::UnwindDegrees(UE_REAL_TO_FLOAT(
			(LocomotionState.bHasInput ? LocomotionState.InputYawAngle : LocomotionState.TargetYawAngle) - LocomotionState.Rotation.Yaw))
	};

	RotationYawAngle = UGarMath::RemapAngleForCounterClockwiseRotation(RotationYawAngle);

	// Scale quick stop animation play rate based on how far the character
	// is going to rotate. At 180 degrees, the play rate will be maximal.

	if (RotationYawAngle <= 0.0f)
	{
		PlayTransitionLeftAnimation(Settings->Transitions.QuickStopBlendInDuration, Settings->Transitions.QuickStopBlendOutDuration,
									FMath::Lerp(Settings->Transitions.QuickStopPlayRate.X, Settings->Transitions.QuickStopPlayRate.Y,
												FMath::Abs(RotationYawAngle) / 180.0f), Settings->Transitions.QuickStopStartTime);
	}
	else
	{
		PlayTransitionRightAnimation(Settings->Transitions.QuickStopBlendInDuration, Settings->Transitions.QuickStopBlendOutDuration,
									 FMath::Lerp(Settings->Transitions.QuickStopPlayRate.X, Settings->Transitions.QuickStopPlayRate.Y,
												 FMath::Abs(RotationYawAngle) / 180.0f), Settings->Transitions.QuickStopStartTime);
	}
}

void UGarAnimationInstance::PlayTransitionAnimation(UAnimSequenceBase* Animation, const float BlendInDuration, const float BlendOutDuration,
													const float PlayRate, const float StartTime, const bool bFromStandingIdleOnly)
{
	if (bFromStandingIdleOnly && (LocomotionState.bMoving || !CurrentGameplayTags.HasTag(GarStanceTags::Standing)))
	{
		return;
	}

	// Animation montages can't be played in the worker thread, so queue them up to play later in the game thread.

	TransitionsState.QueuedTransitionAnimation = Animation;
	TransitionsState.QueuedTransitionBlendInDuration = BlendInDuration;
	TransitionsState.QueuedTransitionBlendOutDuration = BlendOutDuration;
	TransitionsState.QueuedTransitionPlayRate = PlayRate;
	TransitionsState.QueuedTransitionStartTime = StartTime;

	if (IsInGameThread())
	{
		PlayQueuedTransitionAnimation();
	}
}

void UGarAnimationInstance::PlayTransitionLeftAnimation(const float BlendInDuration, const float BlendOutDuration, const float PlayRate,
														const float StartTime, const bool bFromStandingIdleOnly)
{
	if (!IsValid(Settings))
	{
		return;
	}

	PlayTransitionAnimation(CurrentGameplayTags.HasTag(GarStanceTags::Crouching)
								? Settings->Transitions.CrouchingTransitionLeftAnimation
								: Settings->Transitions.StandingTransitionLeftAnimation,
							BlendInDuration, BlendOutDuration, PlayRate, StartTime, bFromStandingIdleOnly);
}

void UGarAnimationInstance::PlayTransitionRightAnimation(const float BlendInDuration, const float BlendOutDuration, const float PlayRate,
														 const float StartTime, const bool bFromStandingIdleOnly)
{
	if (!IsValid(Settings))
	{
		return;
	}

	PlayTransitionAnimation(CurrentGameplayTags.HasTag(GarStanceTags::Crouching)
								? Settings->Transitions.CrouchingTransitionRightAnimation
								: Settings->Transitions.StandingTransitionRightAnimation,
							BlendInDuration, BlendOutDuration, PlayRate, StartTime, bFromStandingIdleOnly);
}

void UGarAnimationInstance::StopTransitionAndTurnInPlaceAnimations(const float BlendOutDuration)
{
	TransitionsState.bStopTransitionsQueued = true;
	TransitionsState.QueuedStopTransitionsBlendOutDuration = BlendOutDuration;

	if (IsInGameThread())
	{
		StopQueuedTransitionAndTurnInPlaceAnimations();
	}
}

void UGarAnimationInstance::RefreshTransitions()
{
	// The allow transitions curve is modified within certain states, so that transitions allowed will be true while in those states.

	TransitionsState.bTransitionsAllowed = FAnimWeight::IsFullWeight(GetCurveValue(UGarConstants::AllowTransitionsCurveName()));

	RefreshDynamicTransition();
}

void UGarAnimationInstance::RefreshDynamicTransition()
{
	if (TransitionsState.DynamicTransitionsFrameDelay > 0)
	{
		TransitionsState.DynamicTransitionsFrameDelay -= 1;
		return;
	}

	if (!TransitionsState.bTransitionsAllowed || LocomotionState.bMoving || !CurrentGameplayTags.HasTag(GarLocomotionModeTags::Grounded))
	{
		return;
	}

	// Check each foot to see if the location difference between the foot look and its desired / target location
	// exceeds a threshold. If it does, play an additive transition animation on that foot. The currently set
	// transition plays the second half of a 2 foot transition animation, so that only a single foot moves.

	const auto FootLockDistanceThresholdSquared{
		FMath::Square(Settings->Transitions.DynamicTransitionFootLockDistanceThreshold * LocomotionState.Scale)
	};

	const auto FootLockLeftDistanceSquared{FVector::DistSquared(FeetState.Left.TargetLocation, FeetState.Left.LockLocation)};
	const auto FootLockRightDistanceSquared{FVector::DistSquared(FeetState.Right.TargetLocation, FeetState.Right.LockLocation)};

	const auto bTransitionLeftAllowed{
		FAnimWeight::IsRelevant(FeetState.Left.LockAmount) && FootLockLeftDistanceSquared > FootLockDistanceThresholdSquared
	};

	const auto bTransitionRightAllowed{
		FAnimWeight::IsRelevant(FeetState.Right.LockAmount) && FootLockRightDistanceSquared > FootLockDistanceThresholdSquared
	};

	if (!bTransitionLeftAllowed && !bTransitionRightAllowed)
	{
		return;
	}

	TObjectPtr<UAnimSequenceBase> DynamicTransitionAnimation;

	// If both transitions are allowed, choose the one with a greater lock distance.

	if (!bTransitionLeftAllowed)
	{
		DynamicTransitionAnimation = CurrentGameplayTags.HasTag(GarStanceTags::Crouching)
										 ? Settings->Transitions.CrouchingDynamicTransitionRightAnimation
										 : Settings->Transitions.StandingDynamicTransitionRightAnimation;
	}
	else if (!bTransitionRightAllowed)
	{
		DynamicTransitionAnimation = CurrentGameplayTags.HasTag(GarStanceTags::Crouching)
										 ? Settings->Transitions.CrouchingDynamicTransitionLeftAnimation
										 : Settings->Transitions.StandingDynamicTransitionLeftAnimation;
	}
	else if (FootLockLeftDistanceSquared >= FootLockRightDistanceSquared)
	{
		DynamicTransitionAnimation = CurrentGameplayTags.HasTag(GarStanceTags::Crouching)
										 ? Settings->Transitions.CrouchingDynamicTransitionLeftAnimation
										 : Settings->Transitions.StandingDynamicTransitionLeftAnimation;
	}
	else
	{
		DynamicTransitionAnimation = CurrentGameplayTags.HasTag(GarStanceTags::Crouching)
										 ? Settings->Transitions.CrouchingDynamicTransitionRightAnimation
										 : Settings->Transitions.StandingDynamicTransitionRightAnimation;
	}

	if (IsValid(DynamicTransitionAnimation))
	{
		// Block next dynamic transitions for about 2 frames to give the animation blueprint some time to properly react to the animation.

		TransitionsState.DynamicTransitionsFrameDelay = 2;

		// Animation montages can't be played in the worker thread, so queue them up to play later in the game thread.

		TransitionsState.QueuedTransitionAnimation = DynamicTransitionAnimation;
		TransitionsState.QueuedTransitionBlendInDuration = Settings->Transitions.DynamicTransitionBlendDuration;
		TransitionsState.QueuedTransitionBlendOutDuration = Settings->Transitions.DynamicTransitionBlendDuration;
		TransitionsState.QueuedTransitionPlayRate = Settings->Transitions.DynamicTransitionPlayRate;
		TransitionsState.QueuedTransitionStartTime = 0.0f;

		if (IsInGameThread())
		{
			PlayQueuedTransitionAnimation();
		}
	}
}

void UGarAnimationInstance::PlayQueuedTransitionAnimation()
{
	check(IsInGameThread())

	if (TransitionsState.bStopTransitionsQueued || !IsValid(TransitionsState.QueuedTransitionAnimation))
	{
		return;
	}

	PlaySlotAnimationAsDynamicMontage(TransitionsState.QueuedTransitionAnimation, UGarConstants::TransitionSlotName(),
									  TransitionsState.QueuedTransitionBlendInDuration, TransitionsState.QueuedTransitionBlendOutDuration,
									  TransitionsState.QueuedTransitionPlayRate, 1, 0.0f, TransitionsState.QueuedTransitionStartTime);

	TransitionsState.QueuedTransitionAnimation = nullptr;
	TransitionsState.QueuedTransitionBlendInDuration = 0.0f;
	TransitionsState.QueuedTransitionBlendOutDuration = 0.0f;
	TransitionsState.QueuedTransitionPlayRate = 1.0f;
	TransitionsState.QueuedTransitionStartTime = 0.0f;
}

void UGarAnimationInstance::StopQueuedTransitionAndTurnInPlaceAnimations()
{
	check(IsInGameThread())

	if (!TransitionsState.bStopTransitionsQueued)
	{
		return;
	}

	StopSlotAnimation(TransitionsState.QueuedStopTransitionsBlendOutDuration, UGarConstants::TransitionSlotName());
	StopSlotAnimation(TransitionsState.QueuedStopTransitionsBlendOutDuration, UGarConstants::TurnInPlaceStandingSlotName());
	StopSlotAnimation(TransitionsState.QueuedStopTransitionsBlendOutDuration, UGarConstants::TurnInPlaceCrouchingSlotName());

	TransitionsState.bStopTransitionsQueued = false;
	TransitionsState.QueuedStopTransitionsBlendOutDuration = 0.0f;
}

bool UGarAnimationInstance::IsRotateInPlaceAllowed()
{
	return CurrentGameplayTags.HasTag(GarRotationModeTags::Aiming) || CurrentGameplayTags.HasTag(GarViewModeTags::FirstPerson);
}

void UGarAnimationInstance::RefreshRotateInPlace(const float DeltaTime)
{
	static constexpr auto PlayRateInterpolationSpeed{5.0f};

	// Rotate in place is allowed only if the character is standing still and aiming or in first-person view mode.

	if (LocomotionState.bMoving || !CurrentGameplayTags.HasTag(GarLocomotionModeTags::Grounded) || !IsRotateInPlaceAllowed())
	{
		RotateInPlaceState.bRotatingLeft = false;
		RotateInPlaceState.bRotatingRight = false;

		RotateInPlaceState.PlayRate = bPendingUpdate
										  ? Settings->RotateInPlace.PlayRate.X
										  : FMath::FInterpTo(RotateInPlaceState.PlayRate, Settings->RotateInPlace.PlayRate.X,
															 DeltaTime, PlayRateInterpolationSpeed);

		RotateInPlaceState.bFootLockInhibited = false;
		return;
	}

	// Check if the character should rotate left or right by checking if the view yaw angle exceeds the threshold.

	RotateInPlaceState.bRotatingLeft = ViewYawAngle < -Settings->RotateInPlace.ViewYawAngleThreshold;
	RotateInPlaceState.bRotatingRight = ViewYawAngle > Settings->RotateInPlace.ViewYawAngleThreshold;

	if (!RotateInPlaceState.bRotatingLeft && !RotateInPlaceState.bRotatingRight)
	{
		RotateInPlaceState.PlayRate = bPendingUpdate
										  ? Settings->RotateInPlace.PlayRate.X
										  : FMath::FInterpTo(RotateInPlaceState.PlayRate, Settings->RotateInPlace.PlayRate.X,
															 DeltaTime, PlayRateInterpolationSpeed);

		RotateInPlaceState.bFootLockInhibited = false;
		return;
	}

	// If the character should rotate, set the play rate to scale with the view yaw
	// speed. This makes the character rotate faster when moving the camera faster.

	const auto PlayRate{
		FMath::GetMappedRangeValueClamped(Settings->RotateInPlace.ReferenceViewYawSpeed,
										  Settings->RotateInPlace.PlayRate, ViewYawAngle)
	};

	RotateInPlaceState.PlayRate = bPendingUpdate
									  ? PlayRate
									  : FMath::FInterpTo(RotateInPlaceState.PlayRate, PlayRate,
														 DeltaTime, PlayRateInterpolationSpeed);

	// Inhibit foot locking when rotating at a large angle or rotating too fast, otherwise the legs may twist into a spiral.

	RotateInPlaceState.bFootLockInhibited =
		Settings->RotateInPlace.bDisableFootLock ||
		FMath::Abs(ViewYawAngle) > Settings->RotateInPlace.FootLockInhibitionViewYawAngleThreshold ||
		ViewYawAngle > Settings->RotateInPlace.FootLockInhibitionViewYawSpeedThreshold;
}

bool UGarAnimationInstance::IsTurnInPlaceAllowed()
{
	return CurrentGameplayTags.HasTag(GarRotationModeTags::ViewDirection) && !CurrentGameplayTags.HasTag(GarViewModeTags::FirstPerson);
}

void UGarAnimationInstance::RefreshTurnInPlace(const float DeltaTime)
{
	// Turn in place is allowed only if transitions are allowed, the character
	// standing still and looking at the camera and not in first-person mode.

	if (LocomotionState.bMoving || !CurrentGameplayTags.HasTag(GarLocomotionModeTags::Grounded) || !IsTurnInPlaceAllowed())
	{
		TurnInPlaceState.ActivationDelay = 0.0f;
		TurnInPlaceState.bFootLockInhibited = false;
		return;
	}

	if (!TransitionsState.bTransitionsAllowed)
	{
		TurnInPlaceState.ActivationDelay = 0.0f;
		return;
	}

	// Check if the view yaw speed is below the threshold and if the view yaw angle is outside the
	// threshold. If so, begin counting the activation delay time. If not, reset the activation delay
	// time. This ensures the conditions remain true for a sustained time before turning in place.

	if (ViewYawSpeed >= Settings->TurnInPlace.ViewYawSpeedThreshold ||
		FMath::Abs(ViewYawAngle) <= Settings->TurnInPlace.ViewYawAngleThreshold)
	{
		TurnInPlaceState.ActivationDelay = 0.0f;
		TurnInPlaceState.bFootLockInhibited = false;
		return;
	}

	TurnInPlaceState.ActivationDelay = bPendingUpdate
										   ? 0.0f
										   : TurnInPlaceState.ActivationDelay + DeltaTime;

	const auto ActivationDelay{
		FMath::GetMappedRangeValueClamped({Settings->TurnInPlace.ViewYawAngleThreshold, 180.0f},
										  Settings->TurnInPlace.ViewYawAngleToActivationDelay,
										  FMath::Abs(ViewYawAngle))
	};

	// Check if the activation delay time exceeds the set delay (mapped to the view yaw angle). If so, start a turn in place.

	if (TurnInPlaceState.ActivationDelay <= ActivationDelay)
	{
		return;
	}

	// Select settings based on turn angle and stance.

	UGarTurnInPlaceSettings* TurnInPlaceSettings{nullptr};
	FName TurnInPlaceSlotName;

	if (CurrentGameplayTags.HasTag(GarStanceTags::Standing))
	{
		TurnInPlaceSlotName = UGarConstants::TurnInPlaceStandingSlotName();

		if (FMath::Abs(ViewYawAngle) < Settings->TurnInPlace.Turn180AngleThreshold)
		{
			TurnInPlaceSettings = ViewYawAngle < 0.0f ? Settings->TurnInPlace.StandingTurn90Left : Settings->TurnInPlace.StandingTurn90Right;
		}
		else
		{
			TurnInPlaceSettings = ViewYawAngle < 0.0f ? Settings->TurnInPlace.StandingTurn180Left : Settings->TurnInPlace.StandingTurn180Right;
		}
	}
	else if (CurrentGameplayTags.HasTag(GarStanceTags::Crouching))
	{
		TurnInPlaceSlotName = UGarConstants::TurnInPlaceCrouchingSlotName();

		if (FMath::Abs(ViewYawAngle) < Settings->TurnInPlace.Turn180AngleThreshold)
		{
			TurnInPlaceSettings = ViewYawAngle < 0.0f ? Settings->TurnInPlace.CrouchingTurn90Left : Settings->TurnInPlace.CrouchingTurn90Right;
		}
		else
		{
			TurnInPlaceSettings = ViewYawAngle < 0.0f ? Settings->TurnInPlace.CrouchingTurn180Left : Settings->TurnInPlace.CrouchingTurn180Right;
		}
	}

	if (TurnInPlaceSettings && IsValid(TurnInPlaceSettings) && ensure(IsValid(TurnInPlaceSettings->Animation)))
	{
		// Animation montages can't be played in the worker thread, so queue them up to play later in the game thread.

		TurnInPlaceState.QueuedSettings = TurnInPlaceSettings;
		TurnInPlaceState.QueuedSlotName = TurnInPlaceSlotName;
		TurnInPlaceState.QueuedTurnYawAngle = ViewYawAngle;

		if (IsInGameThread())
		{
			PlayQueuedTurnInPlaceAnimation();
		}
	}
}

void UGarAnimationInstance::PlayQueuedTurnInPlaceAnimation()
{
	check(IsInGameThread())

	if (TransitionsState.bStopTransitionsQueued || !IsValid(TurnInPlaceState.QueuedSettings))
	{
		return;
	}

	const auto* TurnInPlaceSettings{TurnInPlaceState.QueuedSettings.Get()};

	PlaySlotAnimationAsDynamicMontage(TurnInPlaceSettings->Animation, TurnInPlaceState.QueuedSlotName,
									  Settings->TurnInPlace.BlendDuration, Settings->TurnInPlace.BlendDuration,
									  TurnInPlaceSettings->PlayRate, 1, 0.0f);

	// Scale the rotation yaw delta (gets scaled in animation graph) to compensate for play rate and turn angle (if allowed).

	TurnInPlaceState.PlayRate = TurnInPlaceSettings->bScalePlayRateByAnimatedTurnAngle
									? TurnInPlaceSettings->PlayRate *
									  FMath::Abs(TurnInPlaceState.QueuedTurnYawAngle / TurnInPlaceSettings->AnimatedTurnAngle)
									: TurnInPlaceSettings->PlayRate;

	TurnInPlaceState.bFootLockInhibited = Settings->TurnInPlace.bDisableFootLock;

	TurnInPlaceState.QueuedSettings = nullptr;
	TurnInPlaceState.QueuedSlotName = NAME_None;
	TurnInPlaceState.QueuedTurnYawAngle = 0.0f;
}

float UGarAnimationInstance::GetCurveValueClamped01(const FName& CurveName) const
{
	return UGarMath::Clamp01(GetCurveValue(CurveName));
}
