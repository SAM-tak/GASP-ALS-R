#pragma once

#include "CoreMinimal.h"
#include "GarCharacterMovementState.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarCharacterMovementState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	FVector Velocity{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	FVector VelocityAcceleration{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	FVector LatestVelocityInAir{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	FVector LastNonZeroVelocity{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	FVector GravityAcceleration{ForceInit};

	/** X = Forward Speed, Y = Strafe Speed, Z = Backwards Speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	FVector CurrentMaxSpeed{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	float CurrentAcceleration{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	float CurrentDeceleration{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CharacterMovement")
	TObjectPtr<class UMoverTrajectoryPredictor> TrajectoryPredictor;
};
