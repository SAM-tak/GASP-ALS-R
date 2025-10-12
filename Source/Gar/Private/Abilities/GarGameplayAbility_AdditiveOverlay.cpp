// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/GarGameplayAbility_AdditiveOverlay.h"

#include "GarAbilitySystemComponent.h"
#include "Components/GarOverlayModeComponent.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_AdditiveOverlay)

UGarGameplayAbility_AdditiveOverlay::UGarGameplayAbility_AdditiveOverlay(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarAdditiveOverlayModeTags::Default));
	ActivationOwnedTags.AddTag(GarAdditiveOverlayModeTags::Default);
	CancelAbilitiesWithTag.AddTag(GarAdditiveOverlayModeTags::Root);
	BlockAbilitiesWithTag.AddTag(GarAdditiveOverlayModeTags::Default);
}

void UGarGameplayAbility_AdditiveOverlay::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActorInfo->OwnerActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->OwnerActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->RegisterAdditiveOverlayTask(GetAssetTags().First(), AdditiveOverlayTaskClass);
		}
	}
}

void UGarGameplayAbility_AdditiveOverlay::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

	if (ActorInfo->AvatarActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->AvatarActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->RegisterAdditiveOverlayTask(GetAssetTags().First(), AdditiveOverlayTaskClass);
		}
	}
}

void UGarGameplayAbility_AdditiveOverlay::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (ActorInfo->OwnerActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->OwnerActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->UnregisterAdditiveOverlayTask(GetAssetTags().First());
		}
	}
	if (ActorInfo->AvatarActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->AvatarActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->UnregisterAdditiveOverlayTask(GetAssetTags().First());
		}
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}
