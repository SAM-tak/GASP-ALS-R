#pragma once

#include "GarLayeringAnimInstance.h"
#include "GarAdditiveOverlayAnimInstance.generated.h"

// Additive Overlay Linked Anim Layer
// Tag : "AdditiveOverlay"
UCLASS(Abstract, AutoExpandCategories = ("GAR|Settings"))
class GAR_API UGarAdditiveOverlayAnimInstance : public UGarLinkedAnimationInstance
{
	GENERATED_BODY()
};
