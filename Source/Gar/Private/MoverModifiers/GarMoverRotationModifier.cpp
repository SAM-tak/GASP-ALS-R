// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModifiers/GarMoverRotationModifier.h"

#include "GarGameplayTags.h"

FGarMoverViewDirectionModifier::FGarMoverViewDirectionModifier()
{
	ActiveTag = GarRotationModeTags::ViewDirection;
}

FMovementModifierBase* FGarMoverViewDirectionModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverViewDirectionModifier(*this);
	return CopyPtr;
}

FString FGarMoverViewDirectionModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("ViewDirection Modifier"));
}

FGarMoverVelocityDirectionModifier::FGarMoverVelocityDirectionModifier()
{
	ActiveTag = GarRotationModeTags::VelocityDirection;
}

FMovementModifierBase* FGarMoverVelocityDirectionModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverVelocityDirectionModifier(*this);
	return CopyPtr;
}

FString FGarMoverVelocityDirectionModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("VelocityDirection Modifier"));
}

FGarMoverAimingModifier::FGarMoverAimingModifier()
{
	ActiveTag = GarRotationModeTags::Aiming;
}

FMovementModifierBase* FGarMoverAimingModifier::Clone() const
{
	auto* CopyPtr = new FGarMoverAimingModifier(*this);
	return CopyPtr;
}

FString FGarMoverAimingModifier::ToSimpleString() const
{
	return FString::Printf(TEXT("Aiming Modifier"));
}
