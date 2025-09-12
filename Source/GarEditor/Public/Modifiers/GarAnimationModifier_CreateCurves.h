#pragma once

#include "AnimationModifier.h"
#include "GarConstants.h"
#include "GarAnimationModifier_CreateCurves.generated.h"

USTRUCT(BlueprintType)
struct GAREDITOR_API FGarAnimationCurveKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	int32 Frame{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	float Value{0.0f};
};

USTRUCT(BlueprintType)
struct GAREDITOR_API FGarAnimationCurve
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bAddKeyOnEachFrame : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TArray<FGarAnimationCurveKey> Keys
	{
		{0, 0.0f}
	};
};

UCLASS(DisplayName = "Gar Create Curves Animation Modifier")
class GAREDITOR_API UGarAnimationModifier_CreateCurves : public UAnimationModifier
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bOverrideExistingCurves : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FGarAnimationCurve> Curves
	{
		{UGarConstants::PoseGaitCurveName()},
		{UGarConstants::PoseMovingCurveName()},
		{UGarConstants::PoseStandingCurveName()},
		{UGarConstants::PoseCrouchingCurveName()},
		{UGarConstants::PoseInAirCurveName()},
		{UGarConstants::PoseGroundedCurveName()},

		{UGarConstants::FootLeftIkCurveName()},
		{UGarConstants::FootRightIkCurveName()},
		{UGarConstants::FootPlantedCurveName()},
		{UGarConstants::FeetCrossingCurveName()},

		{UGarConstants::AllowTransitionsCurveName()},
		{UGarConstants::BlockSprintCurveName()},
		{UGarConstants::FootstepSoundBlockCurveName()}
	};

public:
	virtual void OnApply_Implementation(UAnimSequence* Sequence) override;
};
