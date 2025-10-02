// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModifiers/GarMoverGaitModifier.h"

#include "GarGameplayTags.h"

FGarMoverWalkingModifier::FGarMoverWalkingModifier()
{
	ActiveTag = GarGaitTags::Walking;
}

FMovementModifierBase* FGarMoverWalkingModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverWalkingModifier(*this);
	return CopyPtr;
}

FString FGarMoverWalkingModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("Walking Modifier"));
}

FGarMoverRunningModifier::FGarMoverRunningModifier()
{
	ActiveTag = GarGaitTags::Running;
}

FMovementModifierBase* FGarMoverRunningModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverRunningModifier(*this);
	return CopyPtr;
}

FString FGarMoverRunningModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("Running Modifier"));
}

FGarMoverSprintingModifier::FGarMoverSprintingModifier()
{
	ActiveTag = GarGaitTags::Sprinting;
}

FMovementModifierBase* FGarMoverSprintingModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverSprintingModifier(*this);
	return CopyPtr;
}

FString FGarMoverSprintingModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("Sprinting Modifier"));
}
