#pragma once

#include "GarGameplayTags.h"
#include "GarCharacterSettings.generated.h"

UENUM(BlueprintType)
enum class EGarInAirRotationMode : uint8
{
	RotateToVelocityOnJump,
	KeepRelativeRotation,
	KeepWorldRotation
};

UCLASS(Blueprintable, BlueprintType)
class GAR_API UGarCharacterSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float MovingSpeedThreshold{50.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg/s"))
	float AdjustControllRotationSpeed{15.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ForceUnits = "s"))
	float CapsuleUpdateSpeed{0.3f};

	// When FirstPerson Or RotateToVelocityWhenSprinting is False And DesiredRotationMode is not VelocityDirection,
	// sprint will allow if View Relative Angle less than this value. 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float ViewRelativeAngleThresholdForSprint{50.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bAllowAimingWhenInAir : 1{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bSprintHasPriorityOverAiming : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bRotateToVelocityWhenSprinting : 1{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bAutoTurnOffSprint : 1{false};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float SprintOffSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|GameplayTag")
	FGameplayTagContainer OverlayModeTags{GarOverlayModeTags::Root};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|GameplayTag")
	FGameplayTagContainer ActionTags{GarLocomotionActionTags::Root};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|GameplayTag")
	FGameplayTagContainer BlockMoveInputTags{GarLocomotionActionTags::Root};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|GameplayTag")
	TMap<FGameplayTag, FGameplayTag> DesiredToActualMap{
		{GarDesiredRotationModeTags::VelocityDirection, GarRotationModeTags::VelocityDirection},
		{GarDesiredRotationModeTags::ViewDirection, GarRotationModeTags::ViewDirection},
		{GarDesiredStanceTags::Standing, GarStanceTags::Standing},
		{GarDesiredStanceTags::Crouching, GarStanceTags::Crouching},
		{GarDesiredStanceTags::Lying, GarStanceTags::Lying},
		{GarDesiredGaitTags::Walking, GarGaitTags::Walking},
		{GarDesiredGaitTags::Running, GarGaitTags::Running},
		{GarDesiredGaitTags::Sprinting, GarGaitTags::Sprinting},
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|GameplayTag")
	TMap<FGameplayTag, FName> TagToMovementModeMap{
		{GarLocomotionActionTags::Dying, TEXT("Ragdolling")},
		{GarLocomotionActionTags::Unconsious, TEXT("Ragdolling")},
		{GarLocomotionActionTags::FreeFalling, TEXT("Ragdolling")},
	};
};
