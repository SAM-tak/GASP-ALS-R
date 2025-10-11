#pragma once

#include "Engine/EngineTypes.h"
#include "GarRagdollingSettings.generated.h"

UCLASS(Blueprintable, BlueprintType)
class GAR_API UGarRagdollingSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "s"))
	float StartBlendTime{0.25f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float MaxBodySpeed{5000.0f};

	// for correction in multiplayer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float VelocityInterpolationSpeed{10.0f};

	// for correction in multiplayer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Multiplyer")
	float SimulatedProxyInterpolationSpeed{30.0f};

	// for correction in multiplayer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Multiplyer")
	float SimulatedProxyMeshInterpolationSpeed{10.0f};

	// If checked, it stops the physical simulation and returns control of the bone to kinematic
	// when the conditions mentioned later are met.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	uint8 bAllowFreeze : 1{false};

	// The time until it freezes forcibly after landing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Freezing", Meta = (ClampMin = 0, EditCondition = "bAllowFreeze", ForceUnits = "s"))
	float TimeAfterGroundedForForceFreezing{5.0f};

	// The time until it forcibly freezes after the root bone is considered to have stopped when landing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Freezing", Meta = (ClampMin = 0, EditCondition = "bAllowFreeze", ForceUnits = "s"))
	float TimeAfterGroundedAndStoppedForForceFreezing{1.0f};

	// When the speed is below this value, the root bone is considered to be stopped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Freezing", Meta = (ClampMin = 0, EditCondition = "bAllowFreeze", ForceUnits = "cm/s"))
	float RootBoneSpeedConsideredAsStopped{5.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Freezing", Meta = (ClampMin = 0, EditCondition = "bAllowFreeze", ForceUnits = "cm/s"))
	float SpeedThresholdToFreeze{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Freezing", Meta = (ClampMin = 0, EditCondition = "bAllowFreeze", ForceUnits = "deg"))
	float AngularSpeedThresholdToFreeze{45.0f};
};
