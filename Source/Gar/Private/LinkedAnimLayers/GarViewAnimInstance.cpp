#include "LinkedAnimLayers/GarViewAnimInstance.h"
#include "GarAnimationInstance.h"
#include "GarCharacter.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarViewAnimInstance)

//void UGarViewAnimInstance::NativeUpdateAnimation(const float DeltaTime)
void UGarViewAnimInstance::RefreshOnGameThread(const float DeltaTime)
{
	//DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarViewAnimInstance::NativeUpdateAnimation()"),
	//                            STAT_UGarViewAnimInstance_NativeUpdateAnimation, STATGROUP_Gar)

	//Super::NativeUpdateAnimation(DeltaTime);

	if (!Parent.IsValid() || !Character.IsValid())
	{
		return;
	}

	const auto& View{Character->GetViewState()};

	Rotation = View.LookRotation;
	YawSpeed = View.YawSpeed;
}

//void UGarViewAnimInstance::NativeThreadSafeUpdateAnimation(const float DeltaTime)
void UGarViewAnimInstance::Refresh(const float DeltaTime)
{
	//DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarViewAnimInstance::NativeThreadSafeUpdateAnimation()"),
	//                            STAT_UGarViewAnimInstance_NativeThreadSafeUpdateAnimation, STATGROUP_Gar)

	//Super::NativeThreadSafeUpdateAnimation(DeltaTime);

	if (!Parent.IsValid() || !Character.IsValid())
	{
		return;
	}

	if (!Parent->bIsActionRunning)
	{
		YawAngle = FMath::UnwindDegrees(UE_REAL_TO_FLOAT(Rotation.Yaw - Parent->LocomotionState.Rotation.Yaw));
		PitchAngle = FMath::UnwindDegrees(UE_REAL_TO_FLOAT(Rotation.Pitch - Parent->LocomotionState.Rotation.Pitch));
		PitchAmount = 0.5f - PitchAngle / 180.0f;
	}

	const auto ViewAmount{1.0f - Parent->GetCurveValueClamped01(UGarConstants::BlockViewCurveName())};
	const auto AimingAmount{Parent->GetCurveValueClamped01(UGarConstants::AllowAimingCurveName())};

	LookAmount = ViewAmount * (1.0f - AimingAmount);

	// Refresh Spine Rotation

	if (SpineRotation.bSpineRotationAllowed != Parent->IsSpineRotationAllowed())
	{
		SpineRotation.bSpineRotationAllowed = ~SpineRotation.bSpineRotationAllowed;
		SpineRotation.InitialYawAngle = SpineRotation.CurrentYawAngle;
	}

	if (SpineRotation.bSpineRotationAllowed)
	{
		static constexpr auto InterpolationSpeed{20.0f};

		SpineRotation.SpineAmount = Parent->bPendingUpdate
			                        ? 1.0f
			                        : UGarMath::ExponentialDecay(SpineRotation.SpineAmount, 1.0f, DeltaTime, InterpolationSpeed);

		SpineRotation.TargetYawAngle = YawAngle;
	}
	else
	{
		static constexpr auto InterpolationSpeed{10.0f};

		SpineRotation.SpineAmount = Parent->bPendingUpdate
			                        ? 0.0f
			                        : UGarMath::ExponentialDecay(SpineRotation.SpineAmount, 0.0f, DeltaTime, InterpolationSpeed);
	}

	SpineRotation.CurrentYawAngle = UGarMath::LerpAngle(SpineRotation.InitialYawAngle,
	                                                    SpineRotation.TargetYawAngle,
	                                                    SpineRotation.SpineAmount);

	SpineRotation.YawAngle = UGarMath::LerpAngle(0.0f, SpineRotation.CurrentYawAngle, ViewAmount * AimingAmount);
}

void UGarViewAnimInstance::ReinitializeLook()
{
	Look.bReinitializationRequired = true;
}

void UGarViewAnimInstance::RefreshLook()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarViewAnimInstance::RefreshLook()"), STAT_UGarViewAnimInstance_RefreshLook, STATGROUP_Gar)

	if (!Parent.IsValid())
	{
		return;
	}

	Look.bReinitializationRequired |= Parent->bPendingUpdate;

	const auto CharacterYawAngle{UE_REAL_TO_FLOAT(Parent->LocomotionState.Rotation.Yaw)};

	if (Parent->MovementBase.bHasRelativeRotation)
	{
		// Offset the angle to keep it relative to the movement base.

		Look.WorldYawAngle = FMath::UnwindDegrees(Look.WorldYawAngle + Parent->MovementBase.DeltaRotation.Yaw);
	}

	float TargetYawAngle;
	float TargetPitchAngle;
	float InterpolationSpeed;

	if (Parent->FaceRotationMode == GarRotationModeTags::VelocityDirection)
	{
		// Look towards input direction.

		TargetYawAngle = FMath::UnwindDegrees(
			(Parent->LocomotionState.bHasInput ? Parent->LocomotionState.InputYawAngle : Parent->LocomotionState.TargetYawAngle)
			- CharacterYawAngle);
		TargetPitchAngle = 0.0f;
		InterpolationSpeed = LookTowardsInputYawAngleInterpolationSpeed;
	}
	else
	{
		// Look towards view direction.

		TargetYawAngle = YawAngle;
		TargetPitchAngle = PitchAngle;
		InterpolationSpeed = LookTowardsCameraRotationInterpolationSpeed;
	}

	if (Look.bReinitializationRequired || InterpolationSpeed <= 0.0f)
	{
		Look.YawAngle = TargetYawAngle;
		Look.PitchAngle = TargetPitchAngle;
	}
	else
	{
		const auto CurrentYawAngle{FMath::UnwindDegrees(Look.WorldYawAngle - CharacterYawAngle)};
		auto DeltaYawAngle{FMath::UnwindDegrees(TargetYawAngle - CurrentYawAngle)};

		if (FMath::Abs(Parent->LocomotionState.YawSpeed) > UE_SMALL_NUMBER && FMath::Abs(TargetYawAngle) > 90.0f)
		{
			// When interpolating yaw angle, favor the character rotation direction, over the shortest rotation
			// direction, so that the rotation of the head remains synchronized with the rotation of the body.

			DeltaYawAngle = Parent->LocomotionState.YawSpeed > 0.0f ? FMath::Abs(DeltaYawAngle) : -FMath::Abs(DeltaYawAngle);
		}

		const auto InterpolationAmount{UGarMath::ExponentialDecay(GetDeltaSeconds(), InterpolationSpeed)};

		Look.YawAngle = FMath::UnwindDegrees(CurrentYawAngle + DeltaYawAngle * InterpolationAmount);
		Look.PitchAngle = UGarMath::LerpAngle(Look.PitchAngle, TargetPitchAngle, InterpolationAmount);
	}

	Look.WorldYawAngle = FMath::UnwindDegrees(CharacterYawAngle + Look.YawAngle);

	// Separate the yaw angle into 3 separate values. These 3 values are used to improve the
	// blending of the view when rotating completely around the character. This allows to
	// keep the view responsive but still smoothly blend from left to right or right to left.

	Look.YawForwardAmount = Look.YawAngle / 360.0f + 0.5f;
	Look.YawLeftAmount = 0.5f - FMath::Abs(Look.YawForwardAmount - 0.5f);
	Look.YawRightAmount = 0.5f + FMath::Abs(Look.YawForwardAmount - 0.5f);

	Look.bReinitializationRequired = false;
}
