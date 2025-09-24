// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModes/GarMoverRagdollingMode.h"

#include "Components/SkeletalMeshComponent.h"
#include "MoverComponent.h"
#include "MoveLibrary/MovementUtils.h"
#include "MoveLibrary/BasedMovementUtils.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "MoveLibrary/GroundMovementUtils.h"
#include "MoveLibrary/AirMovementUtils.h"
#include "Settings/GarMovementSettings.h"
#include "State/GarCharacterMoverInputs.h"
#include "GarCharacterMoverComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarMoverRagdollingMode)

UGarMoverRagdollingMode::UGarMoverRagdollingMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SharedSettingsClasses.Add(UGarMovementSettings::StaticClass());
}

void UGarMoverRagdollingMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
}

void UGarMoverRagdollingMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	USceneComponent* UpdatedComponent = Params.MovingComps.UpdatedComponent.Get();
	FFloorCheckResult FloorUnderActor;
	UFloorQueryUtils::FindFloor(Params.MovingComps, Settings->FloorSweepDistance, Settings->MaxWalkSlopeCosine, UpdatedComponent->GetComponentLocation(),
		OUT FloorUnderActor);

	if (FloorUnderActor.IsWalkableFloor())
	{
		GameplayTags.Reset();
		GameplayTags.AddTag(GarLocomotionModeTags::Grounded);
	}
	else
	{
		GameplayTags.Reset();
		GameplayTags.AddTag(GarLocomotionModeTags::InAir);
	}
}

void UGarMoverRagdollingMode::OnRegistered(const FName ModeName)
{
	Super::OnRegistered(ModeName);

	Settings = GetMoverComponent()->FindSharedSettings<UGarMovementSettings>();
	ensureMsgf(Settings, TEXT("Failed to find instance of GarMovementSettings on %s. Movement may not function properly."), *GetPathNameSafe(this));
}

void UGarMoverRagdollingMode::OnUnregistered()
{
	Settings = nullptr;

	Super::OnUnregistered();
}
