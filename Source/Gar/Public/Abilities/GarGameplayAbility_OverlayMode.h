// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GarGameplayAbility.h"
#include "GarGameplayAbility_OverlayMode.generated.h"

class UGarOverlayTask;

/**
 * Overlay
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_OverlayMode : public UGarGameplayAbility
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAR", Transient, Meta = (DisplayThumbnail = false))
	TSubclassOf<UGarOverlayTask> OverlayTaskClass;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};
