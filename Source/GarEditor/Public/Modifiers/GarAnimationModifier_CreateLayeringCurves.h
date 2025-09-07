#pragma once

#include "AnimationModifier.h"
#include "GarConstants.h"
#include "GarAnimationModifier_CreateLayeringCurves.generated.h"

UCLASS(DisplayName = "Gar Create Layering Curves Animation Modifier")
class GAREDITOR_API UGarAnimationModifier_CreateLayeringCurves : public UAnimationModifier
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bOverrideExistingCurves : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bAddKeyOnEachFrame : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float CurveValue{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FName> CurveNames
	{
		UGarConstants::LayerHeadCurveName(),
		UGarConstants::LayerHeadAdditiveCurveName(),
		UGarConstants::LayerArmLeftCurveName(),
		UGarConstants::LayerArmLeftAdditiveCurveName(),
		UGarConstants::LayerArmLeftLocalSpaceCurveName(),
		UGarConstants::LayerArmRightCurveName(),
		UGarConstants::LayerArmRightAdditiveCurveName(),
		UGarConstants::LayerArmRightLocalSpaceCurveName(),
		UGarConstants::LayerHandLeftCurveName(),
		UGarConstants::LayerHandRightCurveName(),
		UGarConstants::LayerSpineCurveName(),
		UGarConstants::LayerSpineAdditiveCurveName(),
		UGarConstants::LayerPelvisCurveName(),
		UGarConstants::LayerLegsCurveName(),

		UGarConstants::HandLeftIkCurveName(),
		UGarConstants::HandRightIkCurveName(),

		UGarConstants::ViewBlockCurveName(),
		UGarConstants::AllowAimingCurveName(),

		UGarConstants::HipsDirectionLockCurveName(),
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bAddSlotCurves : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float SlotCurveValue{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FName> SlotCurveNames
	{
		UGarConstants::LayerHeadSlotCurveName(),
		UGarConstants::LayerArmLeftSlotCurveName(),
		UGarConstants::LayerArmRightSlotCurveName(),
		UGarConstants::LayerSpineSlotCurveName(),
		UGarConstants::LayerPelvisSlotCurveName(),
		UGarConstants::LayerLegsSlotCurveName(),
	};

public:
	virtual void OnApply_Implementation(UAnimSequence* Sequence) override;

private:
	void CreateCurves(UAnimSequence* Sequence, const TArray<FName>& Names, float Value) const;
};
