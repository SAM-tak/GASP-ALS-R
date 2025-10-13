// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/GarGameplayAbility_DeltaOverlay.h"

#include "GarAbilitySystemComponent.h"
#include "Components/GarDeltaOverlayModeComponent.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_DeltaOverlay)

void UGarGameplayAbility_DeltaOverlay::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActorInfo->OwnerActor.IsValid())
	{
		auto DeltaOverlayModeComponent{ActorInfo->OwnerActor->GetComponentByClass<UGarDeltaOverlayModeComponent>()};
		if (DeltaOverlayModeComponent)
		{
			DeltaOverlayModeComponent->RegisterDeltaOverlayTask(GetAssetTags().First(), DeltaOverlayTaskClass);
		}
	}
}

void UGarGameplayAbility_DeltaOverlay::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

	if (ActorInfo->AvatarActor.IsValid())
	{
		auto DeltaOverlayModeComponent{ActorInfo->AvatarActor->GetComponentByClass<UGarDeltaOverlayModeComponent>()};
		if (DeltaOverlayModeComponent)
		{
			DeltaOverlayModeComponent->RegisterDeltaOverlayTask(GetAssetTags().First(), DeltaOverlayTaskClass);
		}
	}
}

void UGarGameplayAbility_DeltaOverlay::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (ActorInfo->OwnerActor.IsValid())
	{
		auto DeltaOverlayModeComponent{ActorInfo->OwnerActor->GetComponentByClass<UGarDeltaOverlayModeComponent>()};
		if (DeltaOverlayModeComponent)
		{
			DeltaOverlayModeComponent->UnregisterDeltaOverlayTask(GetAssetTags().First());
		}
	}
	if (ActorInfo->AvatarActor.IsValid())
	{
		auto DeltaOverlayModeComponent{ActorInfo->AvatarActor->GetComponentByClass<UGarDeltaOverlayModeComponent>()};
		if (DeltaOverlayModeComponent)
		{
			DeltaOverlayModeComponent->UnregisterDeltaOverlayTask(GetAssetTags().First());
		}
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}
