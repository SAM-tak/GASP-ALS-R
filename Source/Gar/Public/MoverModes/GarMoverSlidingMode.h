#pragma once

#include "CoreMinimal.h"
#include "MoverModes/GarSmoothWalkingMode.h"
#include "GarMoverSlidingMode.generated.h"

/**
 * BP_MovementMode_Slide の C++ 変換。
 * SmoothWalkingMode を継承し、スロープ角に基づいてMaxSpeed/Acceleration/Decelerationを制御する。
 * アクティブ化時に InitialBoost フェーズ(タイマー駆動)で速度・加速度を上書き。
 */
UCLASS(Blueprintable, BlueprintType)
class GAR_API UGarMoverSlidingMode : public UGarSmoothWalkingMode
{
	GENERATED_BODY()

public:
	UGarMoverSlidingMode();

	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	virtual void GenerateWalkMove_Implementation(
		FMoverTickStartData& StartState,
		float DeltaSeconds,
		const FVector& DesiredVelocity,
		const FQuat& DesiredFacing,
		const FQuat& CurrentFacing,
		FVector& InOutAngularVelocityDegrees,
		FVector& InOutVelocity) override;

	virtual void Activate() override;
	virtual void Deactivate() override;

protected:
	virtual void OnRegistered(const FName ModeName) override;
	virtual void OnUnregistered() override;

	// ---- Initial Boost ----

	/** アクティブ化直後の初速ブーストの継続時間 (秒) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|InitialBoost", meta = (ForceUnits = "s"))
	double InitialBoostTime = 0.2;

	/** ブーストフェーズ中の MaxSpeedOverride (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|InitialBoost", meta = (ForceUnits = "cm/s"))
	double InitialBoostSpeed = 800.0;

	/** ブーストフェーズ中の Acceleration (cm/s^2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|InitialBoost", meta = (ClampMin = "0", ForceUnits = "cm/s^2"))
	double InitialBoostAcceleration = 2000.0;

	// ---- Post-boost ----

	/** ブースト終了後の Acceleration (cm/s^2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|InitialBoost", meta = (ClampMin = "0", ForceUnits = "cm/s^2"))
	double AfterBoostAcceleration = 300.0;

	// ---- Slope thresholds ----

	/** これ以上の傾斜角を「急傾斜」と見なす (度) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Slope", meta = (ForceUnits = "deg"))
	double SteepSlopeAngle = 40.0;

	/** これ以下の傾斜角を「緩傾斜」と見なす (度) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Slope", meta = (ForceUnits = "deg"))
	double ShallowSlopeAngle = 10.0;

	// ---- Speed by slope ----

	/** 急傾斜での MaxSpeedOverride (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Slope", meta = (ForceUnits = "cm/s"))
	double SteepSlopeSpeed = 800.0;

	/** 緩傾斜での MaxSpeedOverride (cm/s)。ShallowSlopeAngle 以下では FlatGroundSpeed を使用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Slope", meta = (ForceUnits = "cm/s"))
	double ShallowSlopeSpeed = 500.0;

	/** ほぼ平地 (SlopeAngle < ShallowSlopeAngle) での MaxSpeedOverride (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Slope", meta = (ForceUnits = "cm/s"))
	double FlatGroundSpeed = 100.0;

	// ---- Deceleration by slope ----

	/** 急傾斜での Deceleration (cm/s^2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Slope", meta = (ClampMin = "0", ForceUnits = "cm/s^2"))
	double SteepSlopeDeceleration = 2000.0;

	/** 平地/緩傾斜での Deceleration (cm/s^2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Slope", meta = (ClampMin = "0", ForceUnits = "cm/s^2"))
	double FlatGroundDeceleration = 500.0;

	// ---- Steering ----

	/** スライディング中の曲げ強度。1.0 = 入力が完全横向きのとき DesiredDir が 45° 曲がる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Steering", meta = (ClampMin = "0"))
	double SteeringStrength = 1.0;

	/** XY速度がこの値以下なら Walking へ遷移 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sliding|Transition", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float ExitToWalkingSpeedThreshold = 200.0f;

private:
	void OnInitialBoostExpired();

	bool bInitialBoost = false;
	FTimerHandle InitialBoostTimerHandle;
};
