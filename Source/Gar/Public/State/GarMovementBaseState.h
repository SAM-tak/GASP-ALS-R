#pragma once

#include "GarMovementBaseState.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarMovementBaseState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UPrimitiveComponent> Primitive{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FName BoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bBaseChanged : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bHasRelativeLocation : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bHasRelativeRotation : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector Location{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FQuat Rotation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FRotator DeltaRotation{ForceInit};
};
