#pragma once

#include "GarViewSettings.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarViewSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	float LookRotationInterpSpeed{15.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	float AdjustControllRotationSpeed{15.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAR")
	uint8 bEnableNetworkSmoothing : 1 {true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAR")
	uint8 bEnableListenServerNetworkSmoothing : 1 {true};
};
