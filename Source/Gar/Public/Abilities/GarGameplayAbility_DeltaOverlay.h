// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GarGameplayAbility.h"
#include "GarGameplayAbility_DeltaOverlay.generated.h"

class UGarDeltaOverlayTask;

/**
 * Delta Overlay
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_DeltaOverlay : public UGarGameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAR", Transient, Meta = (DisplayThumbnail = false))
	TSubclassOf<UGarDeltaOverlayTask> DeltaOverlayTaskClass;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};
