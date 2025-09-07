// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Actions/GarGameplayAbility_MontageBase.h"
#include "GarGameplayAbility_Montage.generated.h"

/**
 *	A gameplay ability that plays a single montage and applies a GameplayEffect
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_Montage : public UGarGameplayAbility_MontageBase
{
	GENERATED_BODY()

protected:
	/** GameplayEffects to apply and then remove while the animation is playing */
	UPROPERTY(EditDefaultsOnly, Category = "GarAbility|Montage")
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY(EditDefaultsOnly, Category = "GarAbility|Montage")
	float PlayRate{1.0f};

	UPROPERTY(EditDefaultsOnly, Category = "GarAbility|Montage")
	FName SectionName;

	UPROPERTY(EditDefaultsOnly, Category = "GarAbility|Montage", Meta = (ForceUnit = "s"))
	float StartTime;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* OwnerInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
