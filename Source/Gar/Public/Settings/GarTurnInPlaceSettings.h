#pragma once

#include "GarTurnInPlaceSettings.generated.h"

class UAnimSequenceBase;

UCLASS(BlueprintType, EditInlineNew)
class GAR_API UGarTurnInPlaceSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TObjectPtr<UAnimSequenceBase> Animation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "x"))
	float PlayRate{1.2f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	uint8 bScalePlayRateByAnimatedTurnAngle : 1 {true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float AnimatedTurnAngle;
};

USTRUCT(BlueprintType)
struct GAR_API FGarGeneralTurnInPlaceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float ViewYawAngleThreshold{45.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float ViewYawSpeedThreshold{50.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	FVector2f ViewYawAngleToActivationDelay{0.0f, 0.75f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", DisplayName = "Turn 180 Angle Threshold",
		Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float Turn180AngleThreshold{130.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "s"))
	float BlendDuration{0.2f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bDisableFootLock : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Instanced, DisplayName = "Standing Turn 90 Left")
	TObjectPtr<UGarTurnInPlaceSettings> StandingTurn90Left{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Instanced, DisplayName = "Standing Turn 90 Right")
	TObjectPtr<UGarTurnInPlaceSettings> StandingTurn90Right{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Instanced, DisplayName = "Standing Turn 180 Left")
	TObjectPtr<UGarTurnInPlaceSettings> StandingTurn180Left{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Instanced, DisplayName = "Standing Turn 180 Right")
	TObjectPtr<UGarTurnInPlaceSettings> StandingTurn180Right{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Instanced, DisplayName = "Crouching Turn 90 Left")
	TObjectPtr<UGarTurnInPlaceSettings> CrouchingTurn90Left{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Instanced, DisplayName = "Crouching Turn 90 Right")
	TObjectPtr<UGarTurnInPlaceSettings> CrouchingTurn90Right{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Instanced, DisplayName = "Crouching Turn 180 Left")
	TObjectPtr<UGarTurnInPlaceSettings> CrouchingTurn180Left{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Instanced, DisplayName = "Crouching Turn 180 Right")
	TObjectPtr<UGarTurnInPlaceSettings> CrouchingTurn180Right{nullptr};
};
