#pragma once

#include "AnimationModifier.h"
#include "GarAnimationModifier_CalculateRotationYawSpeed.generated.h"

class UGarBoneNameTable;

// This animation modifier calculates the root rotation speed and is used to create the rotation yaw
// speed curves for the rotation animations. Each curve value represents the rotation speed from
// the current and next frame, so it can be applied to the actors rotation to mimic root motion.
UCLASS(DisplayName = "Gar Calculate Rotation Yaw Speed Animation Modifier")
class GAREDITOR_API UGarAnimationModifier_CalculateRotationYawSpeed : public UAnimationModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGarBoneNameTable> BoneNameTable;

	virtual void OnApply_Implementation(UAnimSequence* Sequence) override;
};
