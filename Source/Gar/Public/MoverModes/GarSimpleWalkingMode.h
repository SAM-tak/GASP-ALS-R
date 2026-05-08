#pragma once

#include "CoreMinimal.h"
#include "DefaultMovementSet/Modes/WalkingMode.h"
#include "GarSimpleWalkingMode.generated.h"

/**
 * USimpleWalkingMode を GAR プラグイン側にコピーしたもの。
 * UWalkingMode は MinimalAPI で外部プラグインからの派生が可能。
 * UGarSmoothWalkingMode の基底クラス。
 */
UCLASS(Blueprintable, BlueprintType, Abstract)
class GAR_API UGarSimpleWalkingMode : public UWalkingMode
{
	GENERATED_BODY()

public:
	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Generate Simple Walk Move"))
	void GenerateWalkMove(UPARAM(ref) FMoverTickStartData& StartState, float DeltaSeconds, const FVector& DesiredVelocity,
		const FQuat& DesiredFacing, const FQuat& CurrentFacing, UPARAM(ref) FVector& InOutAngularVelocityDegrees, UPARAM(ref) FVector& InOutVelocity);

	/** ≥0 の場合 CommonLegacyMovementSettings の MaxSpeed を上書きする (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Walking Settings", meta = (ForceUnits = "cm/s"))
	float MaxSpeedOverride = -1.0f;
};
