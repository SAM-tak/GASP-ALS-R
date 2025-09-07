#pragma once

#include "GarLeanState.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarLeanState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -1, ClampMax = 1))
	float RightAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -1, ClampMax = 1))
	float ForwardAmount{0.0f};
};
