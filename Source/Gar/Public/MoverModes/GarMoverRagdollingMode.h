#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "MoverDataModelTypes.h"
#include "GarMoverRagdollingMode.generated.h"

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

	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;

	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

protected:
	virtual void OnRegistered(const FName ModeName, const FMoverSimContext& SimContext) override;
	virtual void OnUnregistered(const FMoverSimContext& SimContext) override;

	void CaptureFinalState(USceneComponent* UpdatedComponent, FMovementRecord& Record, const FMoverDefaultSyncState& StartSyncState, const FVector& AngularVelocityDegrees, FMoverDefaultSyncState& OutputSyncState, const float DeltaSeconds) const;

	TObjectPtr<const UGarMovementSettings> Settings;
};
