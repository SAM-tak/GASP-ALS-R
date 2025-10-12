// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/GarGameplayAbility_OverlayMode.h"

#include "GarAbilitySystemComponent.h"
#include "Components/GarOverlayModeComponent.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_OverlayMode)

void UGarGameplayAbility_OverlayMode::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActorInfo->OwnerActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->OwnerActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->RegisterOverlayTask(GetAssetTags().First(), OverlayTaskClass);
		}
	}
}

void UGarGameplayAbility_OverlayMode::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

	if (ActorInfo->AvatarActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->AvatarActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->RegisterOverlayTask(GetAssetTags().First(), OverlayTaskClass);
		}
	}
}

void UGarGameplayAbility_OverlayMode::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (ActorInfo->OwnerActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->OwnerActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->UnregisterOverlayTask(GetAssetTags().First());
		}
	}
	if (ActorInfo->AvatarActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->AvatarActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->UnregisterOverlayTask(GetAssetTags().First());
		}
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}
