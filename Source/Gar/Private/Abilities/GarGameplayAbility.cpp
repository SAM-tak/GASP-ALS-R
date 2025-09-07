// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/GarGameplayAbility.h"
#include "GarCharacter.h"
#include "GarCharacterMovementComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarMotionWarpingComponent.h"
#include "Engine/InputDelegateBinding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility)

UGarGameplayAbility::UGarGameplayAbility(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

AGarCharacter* UGarGameplayAbility::GetGarCharacterFromActorInfo() const
{
	if (!ensure(CurrentActorInfo))
	{
		return nullptr;
	}
	return Cast<AGarCharacter>(CurrentActorInfo->OwnerActor.Get());
}

UGarAbilitySystemComponent* UGarGameplayAbility::GetGarAbilitySystemComponentFromActorInfo() const
{
	auto* GarCharacter{GetGarCharacterFromActorInfo()};
	return GarCharacter ? GarCharacter->GetGarAbilitySystem() : nullptr;
}

UWorld* UGarGameplayAbility::GetWorld() const
{
	auto* RetVal{Super::GetWorld()};
	if (IsValid(RetVal))
	{
		return RetVal;
	}
	else if (CurrentActorInfo)
	{
		if (CurrentActorInfo->AvatarActor.IsValid())
		{
			return CurrentActorInfo->AvatarActor->GetWorld();
		}
		if (CurrentActorInfo->OwnerActor.IsValid())
		{
			return CurrentActorInfo->OwnerActor->GetWorld();
		}
	}
	return nullptr;
}

void UGarGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
										  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ActorInfo && ActorInfo->OwnerActor.IsValid())
	{
		BindInput(ActorInfo->OwnerActor->InputComponent.Get());
	}
}

void UGarGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
									 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->OwnerActor.IsValid())
	{
		UnbindInput(ActorInfo->OwnerActor->InputComponent.Get());
	}

	if (bStopCurrentMontageOnEndAbility)
	{
		StopCurrentMontage(OverrideBlendOutTimeOnEndAbility);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGarGameplayAbility::PlayMontage(const FGameplayAbilityActivationInfo& ActivationInfo, const FGameplayAbilityActorInfo* ActorInfo,
									  const FGarPlayMontageParameter& Parameter)
{
	auto* const AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();

	if (Parameter.MontageToPlay && AbilitySystemComponent)
	{
		if (AbilitySystemComponent->PlayMontage(this, ActivationInfo, Parameter.MontageToPlay, Parameter.PlayRate, Parameter.SectionName, Parameter.StartTime))
		{
			return true;
		}
	}
	return false;
}

bool UGarGameplayAbility::PlayMontage(const FGarPlayMontageParameter& Parameter)
{
	return PlayMontage(GetCurrentActivationInfo(), GetCurrentActorInfo(), Parameter);
}

void UGarGameplayAbility::StopCurrentMontage(const FGameplayAbilityActorInfo* ActorInfo, float OverrideBlendOutTime) const
{
	if (IsValid(GetCurrentMontage()))
	{
		auto* const AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->StopMontageIfCurrent(*GetCurrentMontage(), OverrideBlendOutTime);
		}
	}
}

void UGarGameplayAbility::StopCurrentMontage(float OverrideBlendOutTime) const
{
	StopCurrentMontage(GetCurrentActorInfo(), OverrideBlendOutTime);
}

void UGarGameplayAbility::AddOrUpdateWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector TargetLocation, FRotator TargetRotation)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	if (Character->GetLocalRole() < ROLE_Authority)
	{
		Character->GetMotionWarping()->AddOrUpdateReplicatedWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
	}
}

void UGarGameplayAbility::AddOrUpdateWarpTargetFromComponent(FName WarpTargetName, const USceneComponent* Component, FName BoneName, bool bFollowComponent)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	if (Character->GetLocalRole() < ROLE_Authority)
	{
		Character->GetMotionWarping()->AddOrUpdateReplicatedWarpTargetFromComponent(WarpTargetName, Component, BoneName, bFollowComponent);
	}
}

void UGarGameplayAbility::SetInputBlocked(bool bBlocked) const
{
	auto* Character{GetGarCharacterFromActorInfo()};
	if (Character->GetLocalRole() < ROLE_Authority)
	{
		Character->GetGarCharacterMovement()->SetInputBlocked(bBlocked);
	}
}

void UGarGameplayAbility::OnPossessed(AController* NewController)
{
	if (IsValid(NewController) && IsValid(NewController->InputComponent))
	{
		BindInput(NewController->InputComponent);
	}
}

void UGarGameplayAbility::OnUnPossessed(AController* PreviousController)
{
	if (IsValid(PreviousController) && IsValid(PreviousController->InputComponent))
	{
		UnbindInput(PreviousController->InputComponent);
	}
}

void UGarGameplayAbility::BindInput(UInputComponent* InputComponent)
{
	if(bEnableInputBinding && !bInputBinded && IsValid(InputComponent))
	{
		UInputDelegateBinding::BindInputDelegates(GetClass(), InputComponent, this);
		bInputBinded = true;
	}
}

void UGarGameplayAbility::UnbindInput(UInputComponent* InputComponent)
{
	if(bInputBinded && IsValid(InputComponent))
	{
		InputComponent->ClearBindingsForObject(this);
		bInputBinded = false;
	}
}
