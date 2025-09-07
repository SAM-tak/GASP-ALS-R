#include "LinkedAnimLayers/GarOverlayAnimInstance.h"
#include "GarAnimationInstance.h"
#include "GarCharacter.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverlayAnimInstance)

UGarViewAnimInstance* UGarOverlayAnimInstance::GetViewUnsafe() const
{
	return Parent->GetViewAnimInstance();
}
