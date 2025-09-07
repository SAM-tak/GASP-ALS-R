#pragma once

#include "GarTransitionsSettings.generated.h"

class UAnimSequenceBase;

USTRUCT(BlueprintType)
struct GAR_API FGarTransitionsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float QuickStopBlendInDuration{0.1f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float QuickStopBlendOutDuration{0.2f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	FVector2f QuickStopPlayRate{1.75f, 3.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float QuickStopStartTime{0.3f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> StandingTransitionLeftAnimation{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> StandingTransitionRightAnimation{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> CrouchingTransitionLeftAnimation{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> CrouchingTransitionRightAnimation{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float DynamicTransitionFootLockDistanceThreshold{8.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float DynamicTransitionBlendDuration{0.2f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "x"))
	float DynamicTransitionPlayRate{1.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> StandingDynamicTransitionLeftAnimation{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> StandingDynamicTransitionRightAnimation{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> CrouchingDynamicTransitionLeftAnimation{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UAnimSequenceBase> CrouchingDynamicTransitionRightAnimation{nullptr};
};
