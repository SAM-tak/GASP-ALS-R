#pragma once

#include "AnimationModifier.h"
#include "GarAnimationModifier_CopyCurves.generated.h"

UCLASS(DisplayName = "Gar Copy Curves Animation Modifier")
class GAREDITOR_API UGarAnimationModifier_CopyCurves : public UAnimationModifier
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSoftObjectPtr<UAnimSequence> SourceSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bCopyAllCurves : 1 {true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (EditCondition = "!bCopyAllCurves"))
	TArray<FName> CurveNames;

public:
	virtual void OnApply_Implementation(UAnimSequence* Sequence) override;

private:
	static void CopyCurve(UAnimSequence* SourceSequence, UAnimSequence* TargetSequence, const FName& CurveName);
};
