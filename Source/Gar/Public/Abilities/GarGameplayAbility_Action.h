// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GarGameplayAbility.h"
#include "GarGameplayAbility_Action.generated.h"

class AGarCharacter;
class UGarAbilitySystemComponent;

/**
 *
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_Action : public UGarGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
