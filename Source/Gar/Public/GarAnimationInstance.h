#pragma once

#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "State/GarPoseState.h"
#include "State/GarCharacterMovementState.h"
#include "GarGameplayTags.h"
#include "GarAnimationInstance.generated.h"

class UGarAnimationInstanceSettings;
class UGarLayeringAnimInstance;
class UGarCharacterMovementState;
class UGarRagdollingAnimInstance;
class UGarBoneNameTable;
class AGarCharacter;

UCLASS()
class GAR_API UGarAnimationInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend class UGarLinkedAnimationInstance;
	friend class UGarPhysicalAnimationComponent;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGarBoneNameTable> BoneNameTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<AGarCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarLayeringAnimInstance> LayeringAnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarRagdollingAnimInstance> RagdollingAnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "State", Transient, Meta = (ClampMin = 0, ClampMax = 1))
	float IdleAdditiveAmount{1.0f};

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
	FTransform InteractionTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGarCharacterMovementState CharacterMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "State", Transient)
	FGarPoseState PoseState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTag ViewRotationMode{GarRotationModeTags::ViewDirection};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewOffset", Transient, Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float ViewYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewOffset", Transient, Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float ViewPitchAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewOffset", Transient, Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float SpineYawAngle{0.0f};

public:
	const FGameplayTagContainer& GetCurrentGameplayTags() const;

public:
	virtual void NativeInitializeAnimation() override;

	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaTime) override;

	virtual void NativeThreadSafeUpdateAnimation(float DeltaTime) override;

	virtual void NativePostUpdateAnimation();

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	// Movement Analysis

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool IsMoving() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool IsStarting() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool IsPivoting() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool JustStopped() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool ShouldTurnInPlace() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool ShouldSpinTransition() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool JustLanded() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool JustLanded_Light() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool JustLanded_Heavy() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	bool JustTraversed() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Movement Analysis", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	float Speed2D() const;

	// Pose

private:
	void RefreshPose();

	// Character Movement

private:
	void RefreshCharacterMovementOnGameThread(float DeltaTime);

	// Utility

public:
	UFUNCTION(BlueprintPure, Category = "GAR|ViewState", Meta = (BlueprintThreadSafe, ReturnDisplayName = "ReturnValue"))
	float GetViewPitchAmount() const { return 0.5f - ViewPitchAngle / 180.f; }

	float GetCurveValueClamped01(const FName& CurveName) const;
};

inline const FGameplayTagContainer& UGarAnimationInstance::GetCurrentGameplayTags() const
{
	return CurrentGameplayTags;
}
