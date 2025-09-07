// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/Actions/GarGameplayAbility_Ragdolling.h"
#include "Abilities/Tasks/GarAbilityTask_Tick.h"
#include "GarCharacter.h"
#include "GarCharacterMovementComponent.h"
#include "GarAnimationInstance.h"
#include "GarAbilitySystemComponent.h"
#include "GarPhysicalAnimationComponent.h"
#include "LinkedAnimLayers/GarRagdollingAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/GarOverrideModeComponent.h"
#include "Net/UnrealNetwork.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Ragdolling)

UGarGameplayAbility_Ragdolling::UGarGameplayAbility_Ragdolling(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarLocomotionActionTags::Unconsious));
	ActivationOwnedTags.AddTag(GarLocomotionActionTags::Unconsious);
	CancelAbilitiesWithTag.AddTag(GarLocomotionActionTags::Root);
	BlockAbilitiesWithTag.AddTag(GarLocomotionActionTags::Unconsious);
}

bool UGarGameplayAbility_Ragdolling::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
														const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
														OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	auto Character{GetGarCharacterFromActorInfo()};
	if (IsValid(Character))
	{
		auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
		if (IsValid(PhysicalAnimation))
		{
			const auto& Tag{GetAssetTags().First()};
			if (PhysicalAnimation->HasRagdollingSettings(Tag))
			{
				return true;
			}
			else
			{
				UE_LOG(LogGar, Error, TEXT("PhysicalAnimationComponent Has no Ragdolling Settings for '%s'."), *Tag.ToString());
			}
		}
		else
		{
			UE_LOG(LogGar, Error, TEXT("PhysicalAnimationComponent is Invalid."));
		}
	}
	else
	{
		UE_LOG(LogGar, Error, TEXT("GarCharacter is Invalid."));
	}
	return false;
}

void UGarGameplayAbility_Ragdolling::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
													 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive())
	{
		TickTask = UGarAbilityTask_Tick::New(this, FName(TEXT("UGarGameplayAbility_Ragdolling")));
		if (TickTask.IsValid())
		{
			TickTask->OnTick.AddDynamic(this, &ThisClass::Tick);
			TickTask->ReadyForActivation();
		}
	}
}

void UGarGameplayAbility_Ragdolling::Tick(const float DeltaTime)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
	auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};

	if (!IsActive())
	{
		return;
	}

	K2_OnTick(DeltaTime);

	if (IsGroundedAndAged())
	{
		if (!bOnGroundedAndAgedFired)
		{
			bOnGroundedAndAgedFired = true;
			K2_OnGroundedAndAged();
		}
	}
	else
	{
		bOnGroundedAndAgedFired = false;
	}
}

void UGarGameplayAbility_Ragdolling::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
												const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
	auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};

	auto* OverrideModeComponent{Character->GetComponentByClass<UGarOverrideModeComponent>()};
	OverrideModeComponent->EndCurrentRagdollingTask();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGarGameplayAbility_Ragdolling::IsGroundedAndAged() const
{
	auto* Character{GetGarCharacterFromActorInfo()};
	auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
	auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};
	return RagdollingState.IsGroundedAndAged();
}
