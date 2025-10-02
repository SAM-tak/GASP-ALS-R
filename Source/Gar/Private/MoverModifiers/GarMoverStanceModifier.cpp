// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModifiers/GarMoverStanceModifier.h"

#include "GarGameplayTags.h"

FGarMoverStandingModifier::FGarMoverStandingModifier()
{
	ActiveTag = GarStanceTags::Standing;
}

FMovementModifierBase* FGarMoverStandingModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverStandingModifier(*this);
	return CopyPtr;
}

FString FGarMoverStandingModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("Standing Modifier"));
}

FGarMoverCrouchingModifier::FGarMoverCrouchingModifier()
{
	ActiveTag = GarStanceTags::Crouching;
}

FMovementModifierBase* FGarMoverCrouchingModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverCrouchingModifier(*this);
	return CopyPtr;
}

FString FGarMoverCrouchingModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("Standing Modifier"));
}

FGarMoverLyingModifier::FGarMoverLyingModifier()
{
	ActiveTag = GarStanceTags::Lying;
}

FMovementModifierBase* FGarMoverLyingModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverLyingModifier(*this);
	return CopyPtr;
}

FString FGarMoverLyingModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("Standing Modifier"));
}
