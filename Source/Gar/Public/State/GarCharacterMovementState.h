#pragma once

#include "GarCharacterMovementState.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarCharacterMovementState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector Velocity{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector VelocityLastFrame{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector VelocityAcceleration{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector LastNonZeroVelocity{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector Acceleration{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	float MaxAcceleration{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	float MaxBrakingDeceleration{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	float WalkableFloorZ{0.0f};
};
