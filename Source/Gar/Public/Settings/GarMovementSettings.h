#pragma once

#include "Engine/DataAsset.h"
#include "GarGameplayTags.h"
#include "GarMovementSettings.generated.h"

class UCurveFloat;
class UCurveVector;

USTRUCT(BlueprintType)
struct GAR_API FGarMovementGaitSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float WalkSpeed{175.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float RunSpeed{375.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float SprintSpeed{650.0f};

	// Gait amount to acceleration, deceleration, and ground friction curve.
	// Gait amount ranges from 0 to 3, where 0 is stopped, 1 is walking, 2 is running, and 3 is sprinting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UCurveVector> AccelerationAndDecelerationAndGroundFrictionCurve{nullptr};

	// Gait amount to rotation interpolation speed curve.
	// Gait amount ranges from 0 to 3, where 0 is stopped, 1 is walking, 2 is running, and 3 is sprinting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TObjectPtr<UCurveFloat> RotationInterpolationSpeedCurve{nullptr};

public:
	float GetSpeedByGait(const FGameplayTag& Gait) const;
};

USTRUCT(BlueprintType)
struct GAR_API FGarMovementStanceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ForceInlineRow))
	TMap<FGameplayTag, FGarMovementGaitSettings> Stances
	{
		{GarStanceTags::Standing, {}},
		{GarStanceTags::Crouching, {}},
		{GarStanceTags::Lying, {}}
	};
};

UCLASS(Blueprintable, BlueprintType)
class GAR_API UGarMovementSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ForceInlineRow))
	TMap<FGameplayTag, FGarMovementStanceSettings> RotationModes
	{
		{GarRotationModeTags::VelocityDirection, {}},
		{GarRotationModeTags::ViewDirection, {}},
		{GarRotationModeTags::Aiming, {}}
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float TurnSpeedInAir{200.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float TurnSpeed{720.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float MaxRotationSpeed{1080.0f};
};

inline float FGarMovementGaitSettings::GetSpeedByGait(const FGameplayTag& Gait) const
{
	if (Gait == GarGaitTags::Walking)
	{
		return WalkSpeed;
	}

	if (Gait == GarGaitTags::Running)
	{
		return RunSpeed;
	}

	if (Gait == GarGaitTags::Sprinting)
	{
		return SprintSpeed;
	}

	return 0.0f;
}
