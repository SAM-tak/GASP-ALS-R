// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/GarGameplayAbility_Action.h"
#include "GarCharacter.h"
#include "GarAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Action)

void UGarGameplayAbility_Action::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
											const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	auto* Character{GetGarCharacterFromActorInfo()};
	if (Character)
	{
		auto& LocomotionMode{Character->GetLocomotionMode()};
		if (LocomotionMode == GarLocomotionModeTags::Grounded)
		{
			auto& DesiredStance{Character->GetDesiredStance()};
			if (DesiredStance == GarDesiredStanceTags::Standing)
			{
				Character->UnCrouch();
			}
			else if (DesiredStance == GarDesiredStanceTags::Crouching)
			{
				Character->Crouch();
			}
		}
		else if (LocomotionMode == GarLocomotionModeTags::InAir)
		{
			Character->UnCrouch();
		}
	}
}
