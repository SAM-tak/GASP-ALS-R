#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "MoveLibrary/ModularMovement.h"
#include "GarMoverWalkingMode.generated.h"

class UGarMovementSettings;
struct FFloorCheckResult;
struct FRelativeBaseInfo;
struct FMovementRecord;

UCLASS(Blueprintable, BlueprintType)
class UGarMoverWalkingMode : public UBaseMovementMode
{
	GENERATED_UCLASS_BODY()

public:

	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;

	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	// Returns the active turn generator. Note: you will need to cast the return value to the generator you expect to get, it can also be none
	UFUNCTION(BlueprintPure, Category = Mover)
	UObject* GetTurnGenerator();

	// Sets the active turn generator to use the class provided. Note: To set it back to the default implementation pass in none
	UFUNCTION(BlueprintCallable, Category = Mover)
	void SetTurnGeneratorClass(UPARAM(meta = (MustImplement = "/Script/Mover.TurnGeneratorInterface", AllowAbstract = "false")) TSubclassOf<UObject> TurnGeneratorClass);

protected:

	/** Optional modular object for generating rotation towards desired orientation. If not specified, linear interpolation will be used. */
	UPROPERTY(EditAnywhere, Instanced, Category = Mover, meta = (ObjectMustImplement = "/Script/Mover.TurnGeneratorInterface"))
	TObjectPtr<UObject> TurnGenerator;

	virtual void OnRegistered(const FName ModeName) override;
	virtual void OnUnregistered() override;

	void CaptureFinalState(USceneComponent* UpdatedComponent, bool bDidAttemptMovement, const FFloorCheckResult& FloorResult, const FMovementRecord& Record, FMoverDefaultSyncState& OutputSyncState) const;

	FRelativeBaseInfo UpdateFloorAndBaseInfo(const FFloorCheckResult& FloorResult) const;

	TObjectPtr<const UGarMovementSettings> Settings;
};
