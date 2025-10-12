// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/GarAbilitySet.h"

#include "GarAbilitySystemComponent.h"
#include "Abilities/GarGameplayAbility.h"
#include "Utility/GarLog.h"

#include "Abilities/GarGameplayAbility_OverlayMode.h"
#include "Abilities/GarGameplayAbility_DeltaOverlay.h"
#include "Abilities/Actions/GarGameplayAbility_Ragdolling.h"
#include "Components/GarOverlayModeComponent.h"
#include "Components/GarOverrideModeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAbilitySet)

void FGarAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FGarAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FGarAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* Set)
{
	GrantedAttributeSets.Add(Set);
}

void FGarAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* GarASC)
{
	check(GarASC);

	if (!GarASC->IsOwnerActorAuthoritative())
	{
		// Must be authoritative to give or take ability sets.
		return;
	}

	for (const auto& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			GarASC->ClearAbility(Handle);
		}
	}

	for (const auto& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			GarASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (const auto& Set : GrantedAttributeSets)
	{
		GarASC->RemoveSpawnedAttribute(Set);
	}

	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

void UGarAbilitySet::GiveToAbilitySystem(UGarAbilitySystemComponent* GarAsc, UObject* SourceObject, FGarAbilitySet_GrantedHandles* OutGrantedHandles) const
{
	check(GarAsc);

	if (!GarAsc->IsOwnerActorAuthoritative())
	{
		// Must be authoritative to give or take ability sets.
		// but, call register method of OverlayMode Abilities and OverrideMode Abilities to make class map correctly.
		// TODO: move this logic to UGarAbilitySystemComponent::OnRep_ReplicatedAbilities
		if (GarAsc->GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
		{
			auto OverlayModeComponent{GarAsc->GetOwner()->GetComponentByClass<UGarOverlayModeComponent>()};
			if (OverlayModeComponent)
			{
				for (auto& Ability : GrantedGameplayAbilities)
				{
					auto OverlayModeAbility{Cast<UGarGameplayAbility_OverlayMode>(Ability.Ability->GetDefaultObject<UGarGameplayAbility>())};
					if (OverlayModeAbility)
					{
						OverlayModeComponent->RegisterOverlayTask(OverlayModeAbility->GetAssetTags().First(), OverlayModeAbility->OverlayTaskClass);
					}
					auto DeltaOverlayModeAbility{Cast<UGarGameplayAbility_DeltaOverlay>(Ability.Ability->GetDefaultObject<UGarGameplayAbility>())};
					if (DeltaOverlayModeAbility)
					{
						OverlayModeComponent->RegisterDeltaOverlayTask(DeltaOverlayModeAbility->GetAssetTags().First(),
							DeltaOverlayModeAbility->DeltaOverlayTaskClass);
					}
				}
			}
			auto OverrideModeComponent{GarAsc->GetOwner()->GetComponentByClass<UGarOverrideModeComponent>()};
			if (OverrideModeComponent)
			{
				for (auto& Ability : GrantedGameplayAbilities)
				{
					auto RagdollingAbility{Cast<UGarGameplayAbility_Ragdolling>(Ability.Ability->GetDefaultObject<UGarGameplayAbility>())};
					if (RagdollingAbility)
					{
						OverrideModeComponent->RegisterOverrideTask(RagdollingAbility->GetAssetTags().First(), RagdollingAbility->OverrideTaskClass);
					}
				}
			}
		}
		return;
	}

	// Grant the gameplay abilities.
	for (int32 AbilityIndex{0}; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const auto& AbilityToGrant{GrantedGameplayAbilities[AbilityIndex]};

		if (!IsValid(AbilityToGrant.Ability))
		{
			UE_LOG(LogGar, Error, TEXT("GrantedGameplayAbilities[%d] on ability set [%s] is not valid."), AbilityIndex, *GetNameSafe(this));
			continue;
		}

		auto* AbilityCdo{AbilityToGrant.Ability->GetDefaultObject<UGarGameplayAbility>()};

		FGameplayAbilitySpec AbilitySpec{AbilityCdo, AbilityToGrant.AbilityLevel};
		AbilitySpec.SourceObject = SourceObject;

		const auto AbilitySpecHandle{GarAsc->GiveAbility(AbilitySpec)};

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}

	// Grant the gameplay effects.
	for (int32 EffectIndex{0}; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const auto& EffectToGrant{GrantedGameplayEffects[EffectIndex]};

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOG(LogGar, Error, TEXT("GrantedGameplayEffects[%d] on ability set [%s] is not valid"), EffectIndex, *GetNameSafe(this));
			continue;
		}

		const auto* GameplayEffect{EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>()};
		const auto GameplayEffectHandle{GarAsc->ApplyGameplayEffectToSelf(GameplayEffect, EffectToGrant.EffectLevel, GarAsc->MakeEffectContext())};

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(GameplayEffectHandle);
		}
	}

	// Grant the attribute sets.
	for (int32 SetIndex{0}; SetIndex < GrantedAttributes.Num(); ++SetIndex)
	{
		const auto& SetToGrant{GrantedAttributes[SetIndex]};

		if (!IsValid(SetToGrant.AttributeSet))
		{
			UE_LOG(LogGar, Error, TEXT("GrantedAttributes[%d] on ability set [%s] is not valid"), SetIndex, *GetNameSafe(this));
			continue;
		}

		auto* NewSet{NewObject<UAttributeSet>(GarAsc->GetOwner(), SetToGrant.AttributeSet)};
		GarAsc->AddAttributeSetSubobject(NewSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}
}
