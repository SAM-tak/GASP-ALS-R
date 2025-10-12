#pragma once

#include "GarLayeringAnimInstance.h"
#include "GarDeltaOverlayAnimInstance.generated.h"

// Additive Overlay Linked Anim Layer
// Tag : "AdditiveOverlay"
UCLASS(Abstract, AutoExpandCategories = ("GAR|Settings"))
class GAR_API UGarDeltaOverlayAnimInstance : public UGarLinkedAnimationInstance
{
	GENERATED_BODY()
};
