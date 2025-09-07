#include "GarAnimationInstanceProxy.h"

#include "GarAnimationInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimationInstanceProxy)

FGarAnimationInstanceProxy::FGarAnimationInstanceProxy(UAnimInstance* AnimationInstance): FAnimInstanceProxy{AnimationInstance} {}

void FGarAnimationInstanceProxy::PostUpdate(UAnimInstance* AnimationInstance) const
{
	FAnimInstanceProxy::PostUpdate(AnimationInstance);

	// Epic does not allow to override the UAnimInstance::PostUpdateAnimation()
	// function in child classes, so we have to resort to this workaround.

	auto* GarAnimationInstance{Cast<UGarAnimationInstance>(AnimationInstance)};
	if (IsValid(GarAnimationInstance))
	{
		GarAnimationInstance->NativePostUpdateAnimation();
	}
}
