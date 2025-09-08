#pragma once

#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "State/GarControlRigInput.h"
#include "State/GarFeetState.h"
#include "State/GarGroundedState.h"
#include "State/GarLocomotionAnimationState.h"
#include "State/GarMovementBaseState.h"
#include "State/GarPoseState.h"
#include "State/GarRotateInPlaceState.h"
#include "State/GarTransitionsState.h"
#include "State/GarTurnInPlaceState.h"
#include "GarGameplayTags.h"
#include "GarAnimationInstance.generated.h"

struct FGarFootLimitsSettings;
class UGarAnimationInstanceSettings;
class UGarLinkedAnimationInstance;
class UGarLayeringAnimInstance;
class UGarViewAnimInstance;
class UGarRagdollingAnimInstance;
class AGarCharacter;

UCLASS()
class GAR_API UGarAnimationInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend UGarLinkedAnimationInstance;
	friend UGarViewAnimInstance;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGarAnimationInstanceSettings> Settings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<AGarCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarLayeringAnimInstance> LayeringAnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarViewAnimInstance> ViewAnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarRagdollingAnimInstance> RagdollingAnimInstance;

	// Time of the last teleportation event.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (ClampMin = 0))
	float TeleportedTime{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "State", Transient, Meta = (ClampMin = 0, ClampMax = 1))
	float IdleAdditiveAmount{1.0f};

	// Used to indicate that the animation instance has not been updated for a long time
	// and its current state may not be correct (such as foot location used in foot locking).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bPendingUpdate : 1{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bIsActionRunning : 1{false};

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bDisplayDebugTraces : 1{false};
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTagContainer CurrentGameplayTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTagContainer PreviousGameplayTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTag FaceRotationMode{GarRotationModeTags::ViewDirection};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarMovementBaseState MovementBase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarPoseState PoseState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarLocomotionAnimationState LocomotionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarLocomotionAnimationState PrevLocomotionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarGroundedState GroundedState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarFeetState FeetState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarTransitionsState TransitionsState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarRotateInPlaceState RotateInPlaceState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarTurnInPlaceState TurnInPlaceState;

public:
	//This is in the process of organizing such that things that are only accessed within each LinkedAnimLayer are defined within the LinkedAnimLayer,
	//and only those that are referenced across multiple LinkedAnimLayers are held in the GarAnimationInstance.

	// Former ViewState
	FRotator ViewRotation; // in NativeUpdateAnimation ex ViewState.Rotation
	float ViewYawAngle; // in NativeThreadSafeUpdateAnimation ex ViewState.YawAngle
	float ViewYawSpeed; // in NativeThreadSafeUpdateAnimation ex ViewState.YawSpeed

	const FGameplayTagContainer& GetCurrentGameplayTags() const;

public:
	virtual void NativeInitializeAnimation() override;

	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaTime) override;

	virtual void NativeThreadSafeUpdateAnimation(float DeltaTime) override;

	virtual void NativePostUpdateAnimation();

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	// Core

protected:
	UFUNCTION(BlueprintPure, Category = "GAR|Animation Instance",
		Meta = (BlueprintProtected, BlueprintThreadSafe, ReturnDisplayName = "Rig Input"))
	FGarControlRigInput GetControlRigInput() const;

	mutable TArray<TFunction<void()>> RequestQueue;

public:
	void MarkPendingUpdate();

	void MarkTeleported();

private:
	void RefreshMovementBaseOnGameThread();

	void RefreshPose();

	// View

public:
	virtual bool IsSpineRotationAllowed();

	UGarViewAnimInstance* GetViewAnimInstance() const;

	// Locomotion

private:
	void RefreshLocomotionOnGameThread();

	// Grounded

public:
	const FGarGroundedState& GetGroundedState() const;

protected:
	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe))
	void SetHipsDirection(EGarHipsDirection NewHipsDirection);

	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe))
	void ActivatePivot();

private:
	void RefreshGroundedOnGameThread();

	void RefreshGrounded(float DeltaTime);

	void RefreshMovementDirection();

	void RefreshVelocityBlend(float DeltaTime);

	void RefreshRotationYawOffsets();

	void RefreshSprint(const FVector3f& RelativeAccelerationAmount, float DeltaTime);

	void RefreshStrideBlendAmount();

	void RefreshWalkRunBlendAmount();

	void RefreshStandingPlayRate();

	void RefreshCrouchingPlayRate();

	// Feet

public:
	// If true, the foot locking will be temporarily "paused". This is not the same as a
	// complete shutdown because the internal state of the foot locking will continue to update.
	virtual bool IsFootLockInhibited() const;

private:
	void RefreshFeetOnGameThread();

	void RefreshFeet(float DeltaTime);

	void RefreshFoot(FGarFootState& FootState, const FName& FootIkCurveName, const FName& FootLockCurveName,
	                 const FGarFootLimitsSettings& LimitsSettings, const FTransform& ComponentTransformInverse, float DeltaTime) const;

	void ProcessFootLockTeleport(FGarFootState& FootState) const;

	void ProcessFootLockBaseChange(FGarFootState& FootState, const FTransform& ComponentTransformInverse) const;

	void RefreshFootLock(FGarFootState& FootState, const FName& FootLockCurveName, const FTransform& ComponentTransformInverse,
	                     float DeltaTime, FVector& FinalLocation, FQuat& FinalRotation) const;

	void RefreshFootOffset(FGarFootState& FootState, float DeltaTime, FVector& FinalLocation, FQuat& FinalRotation) const;

	void LimitFootRotation(const FGarFootLimitsSettings& LimitsSettings, const FQuat& ParentRotation, FQuat& Rotation) const;

	// Transitions

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintThreadSafe))
	void PlayQuickStopAnimation();

	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintThreadSafe))
	void PlayTransitionAnimation(UAnimSequenceBase* Animation, float BlendInDuration = 0.2f, float BlendOutDuration = 0.2f,
	                             float PlayRate = 1.0f, float StartTime = 0.0f, bool bFromStandingIdleOnly = false);

	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintThreadSafe))
	void PlayTransitionLeftAnimation(float BlendInDuration = 0.2f, float BlendOutDuration = 0.2f, float PlayRate = 1.0f,
	                                 float StartTime = 0.0f, bool bFromStandingIdleOnly = false);

	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintThreadSafe))
	void PlayTransitionRightAnimation(float BlendInDuration = 0.2f, float BlendOutDuration = 0.2f, float PlayRate = 1.0f,
	                                  float StartTime = 0.0f, bool bFromStandingIdleOnly = false);

	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintThreadSafe))
	void StopTransitionAndTurnInPlaceAnimations(float BlendOutDuration = 0.2f);

private:
	void RefreshTransitions();

	void RefreshDynamicTransition();

	void PlayQueuedTransitionAnimation();

	void StopQueuedTransitionAndTurnInPlaceAnimations();

	// Rotate In Place

public:
	virtual bool IsRotateInPlaceAllowed();

private:
	void RefreshRotateInPlace(float DeltaTime);

	// Turn In Place

public:
	virtual bool IsTurnInPlaceAllowed();

private:
	void RefreshTurnInPlace(float DeltaTime);

	void PlayQueuedTurnInPlaceAnimation();

	// Ragdolling

public:
	UGarRagdollingAnimInstance* GetRagdollingAnimInstance() const;

	// Utility

public:
	float GetCurveValueClamped01(const FName& CurveName) const;
};

inline const FGameplayTagContainer& UGarAnimationInstance::GetCurrentGameplayTags() const
{
	return CurrentGameplayTags;
}

inline UGarViewAnimInstance* UGarAnimationInstance::GetViewAnimInstance() const
{
	return ViewAnimInstance.Get();
}

inline UGarRagdollingAnimInstance* UGarAnimationInstance::GetRagdollingAnimInstance() const
{
	return RagdollingAnimInstance.Get();
}

inline void UGarAnimationInstance::MarkPendingUpdate()
{
	bPendingUpdate |= true;
}

inline void UGarAnimationInstance::MarkTeleported()
{
	TeleportedTime = GetWorld()->GetTimeSeconds();
}

inline const FGarGroundedState& UGarAnimationInstance::GetGroundedState() const
{
	return GroundedState;
}

inline void UGarAnimationInstance::SetHipsDirection(const EGarHipsDirection NewHipsDirection)
{
	GroundedState.HipsDirection = NewHipsDirection;
}

inline void UGarAnimationInstance::ActivatePivot()
{
	GroundedState.bPivotActivationRequested = true;
}
