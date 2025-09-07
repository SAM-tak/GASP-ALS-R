#include "Notifies/GarAnimNotifyState_ApplyGameplayTag.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimNotifyState_ApplyGameplayTag)

void UGarAnimNotifyState_ApplyGameplayTag::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	if (auto* Owner = MeshComp->GetOwner())
	{
		if (auto* Asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
		{
			Asc->AddLooseGameplayTag(TagToApply);
		}
	}
}

void UGarAnimNotifyState_ApplyGameplayTag::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (auto* Owner = MeshComp->GetOwner())
	{
		if (auto* Asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
		{
			Asc->RemoveLooseGameplayTag(TagToApply);
		}
	}
}
