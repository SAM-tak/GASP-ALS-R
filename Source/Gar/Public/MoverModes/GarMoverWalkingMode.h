#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "MoveLibrary/ModularMovement.h"
#include "GarMoverWalkingMode.generated.h"

class UGarMovementSettings;
struct FFloorCheckResult;
struct FRelativeBaseInfo;
struct FMovementRecord;

// Behavior policy for performing floor checks in walking mode when no movement is occurring. 
UENUM(BlueprintType)
enum class EGarStaticFloorCheckPolicy : uint8
{
	// Always perform floor checks, even when not moving. You may want this if static bases may disappear from underneath.
	Always = 0				UMETA(DisplayName = "Always"),

	// Only perform floor checks when not moving IF we're on a dynamic movement base
	OnDynamicBaseOnly = 1	UMETA(DisplayName = "OnDynamicBaseOnly"),
};

UCLASS(Blueprintable, BlueprintType)
class UGarMoverWalkingMode : public UBaseMovementMode
{
	GENERATED_UCLASS_BODY()

public:

	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;

	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	// Returns the active turn generator. Note: you will need to cast the return value to the generator you expect to get, it can also be none
	UFUNCTION(BlueprintPure, Category = Mover)
	UObject* GetTurnGenerator();

	// Sets the active turn generator to use the class provided. Note: To set it back to the default implementation pass in none
	UFUNCTION(BlueprintCallable, Category = Mover)
	void SetTurnGeneratorClass(UPARAM(meta = (MustImplement = "/Script/Mover.TurnGeneratorInterface", AllowAbstract = "false")) TSubclassOf<UObject> TurnGeneratorClass);

protected:

	/** Choice of behavior for floor checks while not moving.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mover)
	EGarStaticFloorCheckPolicy FloorCheckPolicy = EGarStaticFloorCheckPolicy::OnDynamicBaseOnly;

	/** Optional modular object for generating rotation towards desired orientation. If not specified, linear interpolation will be used. */
	UPROPERTY(EditAnywhere, Instanced, Category = Mover, meta = (ObjectMustImplement = "/Script/Mover.TurnGeneratorInterface"))
	TObjectPtr<UObject> TurnGenerator;

	virtual void OnRegistered(const FName ModeName, const FMoverSimContext& SimContext) override;
	virtual void OnUnregistered(const FMoverSimContext& SimContext) override;

	void CaptureFinalState(USceneComponent* UpdatedComponent, bool bDidAttemptMovement, const FFloorCheckResult& FloorResult, const FMovementRecord& Record, const FVector& AngularVelocityDegrees, FMoverDefaultSyncState& OutputSyncState) const;

	FRelativeBaseInfo UpdateFloorAndBaseInfo(const FFloorCheckResult& FloorResult) const;

	TObjectPtr<const UGarMovementSettings> Settings;
};
