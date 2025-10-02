// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MoverModifiers/GarMoverModifier.h"
#include "GarMoverRotationModifier.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarMoverViewDirectionModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverViewDirectionModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverViewDirectionModifier> : public TStructOpsTypeTraitsBase2<FGarMoverViewDirectionModifier>
{
	enum
	{
		WithCopy = true
	};
};


USTRUCT(BlueprintType)
struct GAR_API FGarMoverVelocityDirectionModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverVelocityDirectionModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverVelocityDirectionModifier> : public TStructOpsTypeTraitsBase2<FGarMoverVelocityDirectionModifier>
{
	enum
	{
		WithCopy = true
	};
};

USTRUCT(BlueprintType)
struct GAR_API FGarMoverAimingModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverAimingModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverAimingModifier> : public TStructOpsTypeTraitsBase2<FGarMoverAimingModifier>
{
	enum
	{
		WithCopy = true
	};
};
