#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "GarMoverTraversalMode.generated.h"

class UGarMovementSettings;
class UCommonLegacyMovementSettings;

/**
 * FallingMode: a default movement mode for moving through the air and jumping, typically influenced by gravity and air control
 */
UCLASS(Blueprintable, BlueprintType)
class UGarMoverTraversalMode : public UBaseMovementMode
{
	GENERATED_UCLASS_BODY()

public:
	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;

	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

protected:
	virtual void OnRegistered(const FName ModeName) override;
	virtual void OnUnregistered() override;

	void CaptureFinalState(USceneComponent* UpdatedComponent, FMovementRecord& Record, const FMoverDefaultSyncState& StartSyncState, const FVector& AngularVelocityDegrees, FMoverDefaultSyncState& OutputSyncState, const float DeltaSeconds) const;

	TObjectPtr<const UCommonLegacyMovementSettings> CommonLegacySettings;
};
