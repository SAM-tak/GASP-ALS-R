// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MoverModifiers/GarMoverModifier.h"
#include "GarMoverGaitModifier.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarMoverWalkingModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverWalkingModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverWalkingModifier> : public TStructOpsTypeTraitsBase2<FGarMoverWalkingModifier>
{
	enum
	{
		WithCopy = true
	};
};

USTRUCT(BlueprintType)
struct GAR_API FGarMoverRunningModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverRunningModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverRunningModifier> : public TStructOpsTypeTraitsBase2<FGarMoverRunningModifier>
{
	enum
	{
		WithCopy = true
	};
};

USTRUCT(BlueprintType)
struct GAR_API FGarMoverSprintingModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverSprintingModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverSprintingModifier> : public TStructOpsTypeTraitsBase2<FGarMoverSprintingModifier>
{
	enum
	{
		WithCopy = true
	};
};
