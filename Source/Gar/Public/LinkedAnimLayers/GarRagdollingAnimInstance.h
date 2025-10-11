#pragma once

#include "GarLinkedAnimationInstance.h"
#include "GarRagdollingAnimInstance.generated.h"

// Ragdolling Linked Anim Layer
// Tag : "Ragdolling"
UCLASS(Abstract)
class GAR_API UGarRagdollingAnimInstance : public UGarLinkedAnimationInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient)
	FPoseSnapshot FinalPose;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient, Meta = (ForceUnits = "deg"))
	float LyingDownYawAngleDelta{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient, Meta = (ClampMin = 0, ForceUnits = "s"))
	float StartBlendTime{0.3f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient)
	uint8 bActive : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient)
	uint8 bGroundedAndAged : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient)
	uint8 bFacingUpward : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient)
	uint8 bRagdollingTaskActive : 1{false};

public:
	FPoseSnapshot& GetFinalPoseSnapshot();

	void Freeze();

	void UnFreeze();

	FORCEINLINE void SetStartBlendTime(float NewStartBlendTime)
	{
		StartBlendTime = NewStartBlendTime;
	}

	FORCEINLINE void SetRagdollingTaskActive(bool bNewRagdollingTaskActive)
	{
		bRagdollingTaskActive = bNewRagdollingTaskActive;
	}

	void Refresh(const struct FGarRagdollingState& State, bool bNewActive);
};
