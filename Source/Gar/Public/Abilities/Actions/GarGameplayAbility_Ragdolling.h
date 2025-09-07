// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GarGameplayAbility_Action.h"
#include "GarGameplayAbility_Ragdolling.generated.h"

/**
 * Ragdolling
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_Ragdolling : public UGarGameplayAbility_Action
{
	GENERATED_UCLASS_BODY()

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
									const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
									OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|State", Transient)
	uint8 bOnGroundedAndAgedFired : 1{false};

	UFUNCTION(BlueprintPure, Category = "GAR|Ability|Ragdolling")
	bool IsGroundedAndAged() const;

	UFUNCTION()
	void Tick(const float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|Ability|Ragdolling", DisplayName = "Tick", Meta = (ScriptName = "Tick"))
	void K2_OnTick(const float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|Ability|Ragdolling", DisplayName = "On Grounded And Aged", Meta = (ScriptName = "OnGroundedAndAged"))
	void K2_OnGroundedAndAged();

	TWeakObjectPtr<class UGarAbilityTask_Tick> TickTask;
};
