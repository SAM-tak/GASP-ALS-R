#pragma once

#include "GarMovementDirection.generated.h"

UENUM(BlueprintType)
enum class EGarMovementDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

USTRUCT(BlueprintType)
struct GAR_API FGarMovementDirectionCache
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (AllowPrivateAccess))
	EGarMovementDirection MovementDirection{EGarMovementDirection::Forward};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (AllowPrivateAccess))
	uint8 bForward : 1 {true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (AllowPrivateAccess))
	uint8 bBackward : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (AllowPrivateAccess))
	uint8 bLeft : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (AllowPrivateAccess))
	uint8 bRight : 1 {false};

public:
	constexpr FGarMovementDirectionCache() = default;

	// ReSharper disable once CppNonExplicitConvertingConstructor
	constexpr FGarMovementDirectionCache(const EGarMovementDirection NewMovementDirection)
	{
		MovementDirection = NewMovementDirection;

		bForward = MovementDirection == EGarMovementDirection::Forward;
		bBackward = MovementDirection == EGarMovementDirection::Backward;
		bLeft = MovementDirection == EGarMovementDirection::Left;
		bRight = MovementDirection == EGarMovementDirection::Right;
	}

	constexpr bool IsForward() const
	{
		return bForward;
	}

	constexpr bool IsBackward() const
	{
		return bBackward;
	}

	constexpr bool IsLeft() const
	{
		return bLeft;
	}

	constexpr bool IsRight() const
	{
		return bRight;
	}

	// ReSharper disable once CppNonExplicitConversionOperator
	constexpr operator EGarMovementDirection() const
	{
		return MovementDirection;
	}
};
