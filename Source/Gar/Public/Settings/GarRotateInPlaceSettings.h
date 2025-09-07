#pragma once

#include "GarRotateInPlaceSettings.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarRotateInPlaceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float ViewYawAngleThreshold{50.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	FVector2f ReferenceViewYawSpeed{180.0f, 460.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	FVector2f PlayRate{1.15f, 3.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bDisableFootLock : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR",
		Meta = (ClampMin = 0, ClampMax = 180, EditCondition = "!bDisableFootLock", ForceUnits = "deg"))
	float FootLockInhibitionViewYawAngleThreshold{120.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR",
		Meta = (ClampMin = 0, EditCondition = "!bDisableFootLock", ForceUnits = "deg/s"))
	float FootLockInhibitionViewYawSpeedThreshold{620.0f};
};
