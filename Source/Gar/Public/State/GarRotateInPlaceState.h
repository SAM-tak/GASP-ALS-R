#pragma once

#include "GarRotateInPlaceState.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarRotateInPlaceState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bRotatingLeft : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bRotatingRight : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bFootLockInhibited : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "x"))
	float PlayRate{1.0f};
};
