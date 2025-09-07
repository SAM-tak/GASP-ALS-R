#pragma once

#include "GarInAirRotationMode.generated.h"

UENUM(BlueprintType)
enum class EGarInAirRotationMode : uint8
{
	RotateToVelocityOnJump,
	KeepRelativeRotation,
	KeepWorldRotation
};
