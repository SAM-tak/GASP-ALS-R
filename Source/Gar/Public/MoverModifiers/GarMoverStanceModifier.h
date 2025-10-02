// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MoverModifiers/GarMoverModifier.h"
#include "GarMoverStanceModifier.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarMoverStandingModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverStandingModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverStandingModifier> : public TStructOpsTypeTraitsBase2<FGarMoverStandingModifier>
{
	enum
	{
		WithCopy = true
	};
};

USTRUCT(BlueprintType)
struct GAR_API FGarMoverCrouchingModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverCrouchingModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverCrouchingModifier> : public TStructOpsTypeTraitsBase2<FGarMoverCrouchingModifier>
{
	enum
	{
		WithCopy = true
	};
};

USTRUCT(BlueprintType)
struct GAR_API FGarMoverLyingModifier : public FGarMoverModifier
{
	GENERATED_BODY()

public:
	FGarMoverLyingModifier();

	// @return newly allocated copy of this FMovementModifier. Must be overridden by child classes
	virtual FMovementModifierBase* Clone() const override;

	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FGarMoverLyingModifier> : public TStructOpsTypeTraitsBase2<FGarMoverLyingModifier>
{
	enum
	{
		WithCopy = true
	};
};
