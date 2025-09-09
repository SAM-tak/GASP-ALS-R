#pragma once

#include "GarControlRigInput.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarControlRigInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float SpineYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FQuat FootLeftIkRotation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector FootLeftIkLocation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 1))
	float FootLeftIkAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FQuat FootRightIkRotation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector FootRightIkLocation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 1))
	float FootRightIkAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector2f MinMaxPelvisOffsetZ{ForceInit};
};
