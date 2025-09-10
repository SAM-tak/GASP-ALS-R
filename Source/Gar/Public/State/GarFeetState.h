#pragma once

#include "Engine/EngineTypes.h"
#include "GarFeetState.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarFootState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 1))
	float IkAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector TargetLocation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FQuat TargetRotation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	float OffsetTargetLocationZ{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FQuat OffsetTargetRotation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FGarSpringFloatState OffsetSpringState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	float OffsetLocationZ{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FQuat OffsetRotation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector IkLocation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FQuat IkRotation{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAR", Transient)
	FHitResult Hit;
};

USTRUCT(BlueprintType)
struct GAR_API FGarFeetState
{
	GENERATED_BODY()

	// Choose whether a foot is planted or about to plant when stopping using the foot planted animation
	// curve. A value less than 0.5 means the foot is planted and a value more than 0.5 means the
	// foot is still in the air. The foot planted curve also determines which foot is planted (or
	// about to plant). Positive values mean the right foot is planted, negative values mean the left.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -1, ClampMax = 1))
	float FootPlantedAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 1))
	float FeetCrossingAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FGarFootState Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FGarFootState Right;

	// Pelvis

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector2f MinMaxPelvisOffsetZ{ForceInit};
};
