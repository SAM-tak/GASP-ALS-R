#pragma once

#include "Engine/DataAsset.h"
#include "GarCameraGameplayTags.h"
#include "GarGameplayCameraStateSettings.generated.h"

class UCurveFloat;

USTRUCT(BlueprintType)
struct GARCAMERA_API FGarFirstPersonCameraStateSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FName CameraSocketName{TEXTVIEW("FirstPersonCamera")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FName LeftEyeCameraSocketName{TEXTVIEW("ADSCameraLeft")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FName RightEyeCameraSocketName{TEXTVIEW("ADSCameraRight")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 0.99))
	float FirstPersonFactorThreshold{0.7f};

	// Threshold of Aiming Amount value for aim down sight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 0.99))
	float ADSThreshold{0.9f};

	// If bLeftDominantEye is true, use LeftEyeCameraSocketName instead of RightEyeCameraSocketName.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bLeftDominantEye : 1{false};

	// The distance to move backward from the camera sokcet position.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float RetreatDistance{10.0f};

	// Whiile character has this tag, blocks ADS camera and turns to eye camera.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FGameplayTagContainer BlockSightViewTags;
};

USTRUCT(BlueprintType)
struct GARCAMERA_API FGarThirdPersonCameraStateSettings
{
	GENERATED_BODY()

	// Initial Value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FGameplayTag ShoulderMode{GarCameraShoulderModeTags::Right};

	// If greater than zero, camera location same as FPP when distance from third person camera pivot by blocking by geometry less than this value.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float AutoFPPStartDistance{80.0f};

	// Ends Auto FPP when distance from third person camera pivot by blocking by geometry greater than this value.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float AutoFPPEndDistance{100.0f};

	// The horizontal field of view (in degrees) in panoramic rendering.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float FocusTraceStartOffset{10.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TEnumAsByte<ECollisionChannel> TraceChannel{ECC_Camera};
};

UCLASS(Blueprintable, BlueprintType)
class GARCAMERA_API UGarGameplayCameraStateSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGameplayTag DesiredViewMode{GarCameraViewModeTags::ThirdPerson};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarFirstPersonCameraStateSettings FirstPerson;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarThirdPersonCameraStateSettings ThirdPerson;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "s"))
	float ViewModeChangeBlockTime{0.08f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TEnumAsByte<ECollisionChannel> FocusTraceChannel{ECC_Visibility};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float MinFocalLength{10.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float MaxFocalLength{5000.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float FocusTraceRadius{3.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TObjectPtr<UCurveFloat> HeuristicPitchMapping{nullptr};
};
