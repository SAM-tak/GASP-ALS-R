#pragma once

#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "State/GarControlRigInput.h"
#include "State/GarFeetState.h"
#include "State/GarCharacterMovementState.h"
#include "State/GarMovementBaseState.h"
#include "State/GarPoseState.h"
#include "GarGameplayTags.h"
#include "GarAnimationInstance.generated.h"

struct FGarFootLimitsSettings;
class UGarAnimationInstanceSettings;
class UGarLinkedAnimationInstance;
class UGarLayeringAnimInstance;
class UGarCharacterMovementState;
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
	FTransform CharacterTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTag FaceRotationMode{GarRotationModeTags::ViewDirection};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarMovementBaseState MovementBase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarPoseState PoseState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarCharacterMovementState CharacterMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarFeetState FeetState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (DeprecatedProperty, DeprecationMessage = "remove later"))
	uint8 bCharacterMoving : 1{false}; // TODO : remove later

public:
	//This is in the process of organizing such that things that are only accessed within each LinkedAnimLayer are defined within the LinkedAnimLayer,
	//and only those that are referenced across multiple LinkedAnimLayers are held in the GarAnimationInstance.

	// Former ViewState
	FRotator ViewRotation; // in NativeUpdateAnimation ex ViewState.Rotation
	float ViewYawAngle; // in NativeThreadSafeUpdateAnimation ex ViewState.YawAngle
	float ViewYawSpeed; // in NativeThreadSafeUpdateAnimation ex ViewState.YawSpeed

	// Former LocomotionAnimationState
	FRotator CharacterRotation{ForceInit};
	float CharacterZScale;
	float InputYawAngle{0.0f};
	float TargetYawAngle{0.0f};
	float YawSpeed{0.0f};
	uint8 bHasInput : 1{false};
	uint8 bMovingSmooth : 1{false};

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

	// Character Movement

private:
	void RefreshCharacterMovementOnGameThread(float DeltaTime);

	// Feet

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
