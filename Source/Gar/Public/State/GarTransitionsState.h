#pragma once

#include "GarTransitionsState.generated.h"

class UAnimSequenceBase;

USTRUCT(BlueprintType)
struct GAR_API FGarTransitionsState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bTransitionsAllowed : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	int32 DynamicTransitionsFrameDelay{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> QueuedTransitionAnimation{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float QueuedTransitionBlendInDuration{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float QueuedTransitionBlendOutDuration{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "x"))
	float QueuedTransitionPlayRate{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float QueuedTransitionStartTime{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bStopTransitionsQueued : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float QueuedStopTransitionsBlendOutDuration{0.0f};
};
