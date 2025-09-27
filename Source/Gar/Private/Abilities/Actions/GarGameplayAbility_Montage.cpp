// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Actions/GarGameplayAbility_Montage.h"
#include "GarAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Montage)

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGarGameplayAbility_Montage
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void UGarGameplayAbility_Montage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	PlayMontage(ActivationInfo, MontageToPlay, PlayRate, SectionName, StartTime, Handle, ActorInfo);

	if (CurrentMotangeDuration <= 0.0f)
	{
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
