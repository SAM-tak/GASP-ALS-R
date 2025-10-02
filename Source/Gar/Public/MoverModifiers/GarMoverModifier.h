// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovementModifier.h"
#include "GarMoverModifier.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarMoverModifier : public FMovementModifierBase
{
	GENERATED_BODY()

public:
	FGarMoverModifier();

	FGameplayTag ActiveTag;

	virtual bool HasGameplayTag(FGameplayTag TagToFind, bool bExactMatch) const override;
};
