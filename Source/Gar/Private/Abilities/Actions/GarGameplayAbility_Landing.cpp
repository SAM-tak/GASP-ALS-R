// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/Actions/GarGameplayAbility_Landing.h"

#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarGameplayTags.h"
#include "Utility/GarMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Landing)

UGarGameplayAbility_Landing::UGarGameplayAbility_Landing(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarLocomotionActionTags::Landing));
	ActivationOwnedTags.Reset();
	ActivationOwnedTags.AddTag(GarLocomotionActionTags::Landing);
	BlockAbilitiesWithTag.Reset();
	BlockAbilitiesWithTag.AddTag(GarLocomotionActionTags::Landing);
}

float UGarGameplayAbility_Landing::CalcTargetYawAngle_Implementation() const
{
	auto* Character{GetGarCharacterFromActorInfo()};
	return UE_REAL_TO_FLOAT(FMath::UnwindDegrees(Character->GetActorRotation().Yaw));
}

bool UGarGameplayAbility_Landing::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
													 const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
													 OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		const auto* Character{Cast<AGarCharacter>(ActorInfo->OwnerActor)};
		if (bStartRagdollingOnLand && Character->GetVelocity().Z <= -RagdollingOnLandSpeedThreshold)
		{
			return true;
		}
		else if (bStartRollingOnLand && Character->GetVelocity().Z <= -RollingOnLandSpeedThreshold)
		{
			return true;
		}
	}
	return false;
}

void UGarGameplayAbility_Landing::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
												  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	if (bStartRagdollingOnLand && Character->GetVelocity().Z <= -RagdollingOnLandSpeedThreshold)
	{
		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			return;
		}

		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);

		GetGarAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesBySingleTag(GarLocomotionActionTags::Unconsious);
	}
	else if (bStartRollingOnLand && Character->GetVelocity().Z <= -RollingOnLandSpeedThreshold)
	{
		Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	}
}
