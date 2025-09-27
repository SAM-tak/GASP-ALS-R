#include "GarAnimationInstance.h"

#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "MotionWarpingComponent.h"
#include "MoverPoseSearchTrajectoryPredictor.h"
#include "GarAnimationInstanceProxy.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarConstants.h"
#include "LinkedAnimLayers/GarLayeringAnimInstance.h"
#include "LinkedAnimLayers/GarRagdollingAnimInstance.h"
#include "Settings/GarBoneNameTable.h"
#include "Abilities/Actions/GarGameplayAbility_Ragdolling.h"
#include "Utility/GarMath.h"
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
	LayeringAnimInstance = Cast<UGarLayeringAnimInstance>(GetLinkedAnimGraphInstanceByTag(FName{TEXTVIEW("Layering")}));
	RagdollingAnimInstance = Cast<UGarRagdollingAnimInstance>(GetLinkedAnimGraphInstanceByTag(FName{TEXTVIEW("Ragdolling")}));

	if (!ensure(IsValid(BoneNameTable))) return;
	if (!ensure(Character.IsValid())) return;
	if (!ensure(LayeringAnimInstance.IsValid())) return;
	if (!ensure(RagdollingAnimInstance.IsValid())) return;

	Super::NativeBeginPlay();
}

void UGarAnimationInstance::NativeUpdateAnimation(const float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarAnimationInstance::NativeUpdateAnimation()"),
								STAT_UGarAnimationInstance_NativeUpdateAnimation, STATGROUP_Gar)

	Super::NativeUpdateAnimation(DeltaTime);

	if (!IsValid(BoneNameTable) || !Character.IsValid() || !Character->GetMotionWarping())
	{
		return;
	}

	PreviousGameplayTags = CurrentGameplayTags;
	Character->GetOwnedGameplayTags(CurrentGameplayTags);

	auto* Mesh{GetSkelMeshComponent()};
	CharacterTransform = Mesh->GetComponentTransform();

	const auto WarpTarget {Character->GetMotionWarping()->FindWarpTarget(FName(TEXTVIEW("FrontLedge")))};
	if (WarpTarget)
	{
		InteractionTransform = WarpTarget->GetTargetTrasform();
	}

	ViewRotationMode = Character->GetRotationMode();
	if (ViewRotationMode != GarRotationModeTags::Aiming)
	{
		ViewRotationMode = Character->GetDesiredRotationMode();
	}
	bIsActionRunning = Character->GetLocomotionAction().IsValid();

	RefreshCharacterMovementOnGameThread(DeltaTime);
}

void UGarAnimationInstance::NativeThreadSafeUpdateAnimation(const float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarAnimationInstance::NativeThreadSafeUpdateAnimation()"),
								STAT_UGarAnimationInstance_NativeThreadSafeUpdateAnimation, STATGROUP_Gar)

	Super::NativeThreadSafeUpdateAnimation(DeltaTime);

	if (!IsValid(BoneNameTable) || !Character.IsValid())
	{
		return;
	}

	if (LayeringAnimInstance.IsValid())
	{
		LayeringAnimInstance->Refresh();
	}

	RefreshPose();
}

void UGarAnimationInstance::NativePostUpdateAnimation()
{
	// Can't use UAnimationInstance::NativePostEvaluateAnimation() instead this function, as it will not be called if
	// USkinnedMeshComponent::VisibilityBasedAnimTickOption is set to EVisibilityBasedAnimTickOption::AlwaysTickPose.

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarAnimationInstance::NativePostUpdateAnimation()"),
								STAT_UGarAnimationInstance_NativePostUpdateAnimation, STATGROUP_Gar)

	if (!IsValid(BoneNameTable) || !Character.IsValid())
	{
		return;
	}
}

FAnimInstanceProxy* UGarAnimationInstance::CreateAnimInstanceProxy()
{
	return new FGarAnimationInstanceProxy{this};
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
	PoseState.LyingAmount = GetCurveValue(Curves, UGarConstants::PoseLyingCurveName());

	PoseState.MovingAmount = GetCurveValue(Curves, UGarConstants::PoseMovingCurveName());

	PoseState.GaitAmount = FMath::Clamp(GetCurveValue(Curves, UGarConstants::PoseGaitCurveName()), 0.0f, 3.0f);
	PoseState.GaitWalkingAmount = UGarMath::Clamp01(PoseState.MovingAmount);
	PoseState.GaitRunningAmount = UGarMath::Clamp01(PoseState.GaitAmount);
	PoseState.GaitSprintingAmount = UGarMath::Clamp01(PoseState.GaitAmount - 1.0f);

	PoseState.AimingAmount = GetCurveValue(Curves, UGarConstants::PoseAimingCurveName());
}

void UGarAnimationInstance::RefreshCharacterMovementOnGameThread(float DeltaTime)
{
	check(IsInGameThread())

	const auto* Mover{Character->GetMover()};

	CharacterMovement.VelocityAcceleration = (CharacterMovement.Velocity - Mover->GetVelocity()) / FMath::Max(DeltaTime, 0.001f);
	CharacterMovement.Velocity = Mover->GetVelocity();
	CharacterMovement.CurrentMaxSpeed = Mover->CurrentMaxSpeed;
	CharacterMovement.CurrentAcceleration = Mover->CurrentAcceleration;
	CharacterMovement.CurrentDeceleration = Mover->CurrentDeceleration;
	CharacterMovement.bIsGrounded = Mover->GetLocomotionMode() == GarLocomotionModeTags::Grounded;
	CharacterMovement.GravityAcceleration = Mover->GetGravityAcceleration();
	CharacterMovement.ViewRotation = Character->GetViewRotation();
	CharacterMovement.TrajectoryPredictor = Mover->GetTrajectoryPredictor();

	if (CharacterMovement.GravityAcceleration.SquaredLength() > 0.001)
	{
		CharacterMovement.UpVector = -CharacterMovement.GravityAcceleration.GetUnsafeNormal();
	}
	if (Mover->GetLocomotionMode() == GarLocomotionModeTags::InAir)
	{
		CharacterMovement.LatestVelocityInAir = CharacterMovement.Velocity;
	}
	if (CharacterMovement.Velocity.Size2D() > 5.0f)
	{
		CharacterMovement.LastNonZeroVelocity = CharacterMovement.Velocity;
	}
}

float UGarAnimationInstance::GetCurveValueClamped01(const FName& CurveName) const
{
	return UGarMath::Clamp01(GetCurveValue(CurveName));
}
