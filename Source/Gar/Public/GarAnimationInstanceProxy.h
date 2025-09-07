#pragma once

#include "Animation/AnimInstanceProxy.h"
#include "GarAnimationInstanceProxy.generated.h"

class UGarAnimationInstance;
class UGarLinkedAnimationInstance;

USTRUCT()
struct GAR_API FGarAnimationInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	// This allows UGarAnimationInstance and UGarLinkedAnimationInstance to access some protected members of FAnimInstanceProxy.

	friend UGarAnimationInstance;
	friend UGarLinkedAnimationInstance;

public:
	FGarAnimationInstanceProxy() = default;

	explicit FGarAnimationInstanceProxy(UAnimInstance* AnimationInstance);

protected:
	virtual void PostUpdate(UAnimInstance* AnimationInstance) const override;
};
