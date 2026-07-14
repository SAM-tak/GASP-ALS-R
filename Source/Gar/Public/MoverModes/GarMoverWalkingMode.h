#pragma once

#include "CoreMinimal.h"
#include "MoverModes/GarSmoothWalkingMode.h"
#include "GarMoverWalkingMode.generated.h"

class UGarMovementSettings;

/**
 * UGarSmoothWalkingMode (= UE5.8 USmoothWalkingMode 相当) を継承したウォーキングモード。
 * UGarMovementSettings の Stance/Gait 別スピード設定を毎フレーム
 * MaxSpeedOverride / Acceleration / Deceleration に注入する。
 * 床処理・動的ベース追従などの SimulationTick は UWalkingMode (5.8) を継承。
 */
UCLASS(Blueprintable, BlueprintType)
class UGarMoverWalkingMode : public UGarSmoothWalkingMode
{
	GENERATED_UCLASS_BODY()

public:

	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;

protected:

	virtual void OnRegistered(const FName ModeName, const FMoverSimContext& SimContext) override;
	virtual void OnUnregistered(const FMoverSimContext& SimContext) override;

	TObjectPtr<const UGarMovementSettings> Settings;
};
