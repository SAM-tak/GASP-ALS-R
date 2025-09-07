#pragma once

#include "GarTurnInPlaceState.generated.h"

class UGarTurnInPlaceSettings;

USTRUCT(BlueprintType)
struct GAR_API FGarTurnInPlaceState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ForceUnits = "s"))
	float ActivationDelay{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UGarTurnInPlaceSettings> QueuedSettings{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FName QueuedSlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float QueuedTurnYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "x"))
	float PlayRate{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bFootLockInhibited : 1 {false};
};
