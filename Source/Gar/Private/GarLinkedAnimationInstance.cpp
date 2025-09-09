#include "GarLinkedAnimationInstance.h"

#include "GarAnimationInstance.h"
#include "GarAnimationInstanceProxy.h"
#include "GarCharacter.h"
#include "Utility/GarMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarLinkedAnimationInstance)

UGarLinkedAnimationInstance::UGarLinkedAnimationInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bUseMainInstanceMontageEvaluationData = true;
}

void UGarLinkedAnimationInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Parent = Cast<UGarAnimationInstance>(GetSkelMeshComponent()->GetAnimInstance());
	Character = Cast<AGarCharacter>(GetOwningActor());

#if WITH_EDITOR
	if (!GetWorld()->IsGameWorld())
	{
		// Use default objects for editor preview.

		if (!Parent.IsValid())
		{
			Parent = GetMutableDefault<UGarAnimationInstance>();
		}

		if (!Character.IsValid())
		{
			Character = GetMutableDefault<AGarCharacter>();
		}
	}
#endif
}

void UGarLinkedAnimationInstance::NativeBeginPlay()
{
	ensureMsgf(Parent.IsValid(),
			   TEXT("%s (%s) should only be used as a linked animation instance within the %s animation blueprint!"),
			   GAR_GET_TYPE_STRING(UGarLinkedAnimationInstance).GetData(), *GetClass()->GetName(),
			   GAR_GET_TYPE_STRING(UGarAnimationInstance).GetData());

	Super::NativeBeginPlay();
}

FAnimInstanceProxy* UGarLinkedAnimationInstance::CreateAnimInstanceProxy()
{
	return new FGarAnimationInstanceProxy{this};
}

TMap<FName, float>& UGarLinkedAnimationInstance::GetAnimationCurvesFromProxy(EAnimCurveType InCurveType)
{
	return GetProxyOnAnyThread<FGarAnimationInstanceProxy>().GetAnimationCurves(EAnimCurveType::AttributeCurve);
}

UGarViewAnimInstance* UGarLinkedAnimationInstance::GetViewUnsafe() const
{
	return Parent->ViewAnimInstance.Get();
}
