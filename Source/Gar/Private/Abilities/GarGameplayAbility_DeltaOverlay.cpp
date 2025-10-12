// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/GarGameplayAbility_DeltaOverlay.h"

#include "GarAbilitySystemComponent.h"
#include "Components/GarOverlayModeComponent.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_DeltaOverlay)

UGarGameplayAbility_DeltaOverlay::UGarGameplayAbility_DeltaOverlay(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarOverlayModeTags::Default));
	ActivationOwnedTags.AddTag(GarOverlayModeTags::Default);
	CancelAbilitiesWithTag.AddTag(GarOverlayModeTags::Root);
	BlockAbilitiesWithTag.AddTag(GarOverlayModeTags::Default);
}

void UGarGameplayAbility_DeltaOverlay::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActorInfo->OwnerActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->OwnerActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->RegisterDeltaOverlayTask(GetAssetTags().First(), DeltaOverlayTaskClass);
		}
	}
}

void UGarGameplayAbility_DeltaOverlay::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

	if (ActorInfo->AvatarActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->AvatarActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->RegisterDeltaOverlayTask(GetAssetTags().First(), DeltaOverlayTaskClass);
		}
	}
}

void UGarGameplayAbility_DeltaOverlay::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (ActorInfo->OwnerActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->OwnerActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->UnregisterDeltaOverlayTask(GetAssetTags().First());
		}
	}
	if (ActorInfo->AvatarActor.IsValid())
	{
		auto OverlayModeComponent{ActorInfo->AvatarActor->GetComponentByClass<UGarOverlayModeComponent>()};
		if (OverlayModeComponent)
		{
			OverlayModeComponent->UnregisterDeltaOverlayTask(GetAssetTags().First());
		}
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}
