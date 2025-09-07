#pragma once

#include "GarGeneralAnimationSettings.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarGeneralAnimationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float MovingSmoothSpeedThreshold{150.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	float LeanInterpolationSpeed{4.0f};
};
