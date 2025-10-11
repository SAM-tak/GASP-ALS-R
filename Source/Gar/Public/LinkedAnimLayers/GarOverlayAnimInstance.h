#pragma once

#include "GarLayeringAnimInstance.h"
#include "GarOverlayAnimInstance.generated.h"

// Overlay Linked Anim Layer
// Tag : "Overlay"
UCLASS(Abstract, AutoExpandCategories = ("GAR|Settings"))
class GAR_API UGarOverlayAnimInstance : public UGarLinkedAnimationInstance
{
	GENERATED_BODY()
};
