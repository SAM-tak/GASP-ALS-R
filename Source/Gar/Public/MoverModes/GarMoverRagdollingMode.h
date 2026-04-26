#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "MoverDataModelTypes.h"
#include "GarMoverRagdollingMode.generated.h"

/**
 * ラグドール中は Chaos 物理の非決定論的な結果により AP とサーバーのカプセル位置が常に 5cm 以上乖離する。
 * FMoverDefaultSyncState::ShouldReconcile の固定閾値 5cm による毎フレーム reconciliation を防ぐため、
 * ShouldReconcile を常に false にオーバーライドした派生型を使用する。
 * FindDataByType は GetSuperStruct() を辿るため Mover 内部の FindDataByType<FMoverDefaultSyncState>() との互換性は維持される。
 */
USTRUCT()
struct FGarMoverRagdollingSyncState : public FMoverDefaultSyncState
{
	GENERATED_BODY()

	virtual FMoverDataStructBase* Clone() const override { return new FGarMoverRagdollingSyncState(*this); }
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override
	{
		return FMoverDefaultSyncState::NetSerialize(Ar, Map, bOutSuccess);
	}
	// ラグドール中は Chaos 物理が非決定論的なため AP とサーバーの位置は常に乖離する。
	// Reconciliation を抑制し、AP のローカル物理ボディ追従を妨げない。
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override { return false; }
	// FMoverSyncState::ShouldReconcile の MovementMode 比較も含めた全 reconcile を抑制する。
	// AP が "Ragdolling In Air"、サーバーが "Ragdolling" に先行遷移した場合の MovementMode
	// 不一致による reconcile (1m 釣り上げ) を防ぐ。
	virtual bool ShouldContainerReconcile() const override { return false; }
};

template<>
struct TStructOpsTypeTraits<FGarMoverRagdollingSyncState> : public TStructOpsTypeTraitsBase2<FGarMoverRagdollingSyncState>
{
	enum { WithNetSerializer = true };
};

class UGarMovementSettings;

/**
 * FallingMode: a default movement mode for moving through the air and jumping, typically influenced by gravity and air control
 */
UCLASS(Blueprintable, BlueprintType)
class UGarMoverRagdollingMode : public UBaseMovementMode
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * The bone name to trace
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mover)
	FName TopBoneName{TEXTVIEW("pelvis")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mover)
	float MinSpeed{300.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mover)
	float MaxSpeed{5000.0f};

	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;

	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

protected:
	virtual void OnRegistered(const FName ModeName) override;
	virtual void OnUnregistered() override;

	void CaptureFinalState(USceneComponent* UpdatedComponent, FMovementRecord& Record, const FMoverDefaultSyncState& StartSyncState, const FVector& AngularVelocityDegrees, FMoverDefaultSyncState& OutputSyncState, const float DeltaSeconds) const;

	TObjectPtr<const UGarMovementSettings> Settings;
};
