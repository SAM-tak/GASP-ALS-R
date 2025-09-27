// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Actions/GarGameplayAbility_MontageBase.h"

#include "Animation/AnimInstance.h"
#include "DefaultMovementSet/LayeredMoves/AnimRootMotionLayeredMove.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_MontageBase)

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGarGameplayAbility_MontageBase
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void UGarGameplayAbility_MontageBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
												 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	auto* AnimInstance = ActorInfo->GetAnimInstance();
	if (AnimInstance)
	{
		FOnMontageEnded EndDelegate;
		AnimInstance->Montage_SetEndDelegate(EndDelegate, CurrentMontage.Get());
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &ThisClass::OnNotifyBeginReceived);
		AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &ThisClass::OnNotifyEndReceived);
	}

	auto* const AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (AbilitySystemComponent)
	{
		if (bWasCancelled && IsValid(GetCurrentMontage()))
		{
			AbilitySystemComponent->StopMontageIfCurrent(*GetCurrentMontage(), BlendOutDurationOnCancel);
			bStopCurrentMontageOnEndAbility = false;
		}

		// Remove any GameplayEffects that we applied

		for (auto& EffectHandle : AppliedEffects)
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
		}
		AppliedEffects.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGarGameplayAbility_MontageBase::OnEndMontage_Implementation(UAnimMontage* Montage, bool bInterrupted)
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, bInterrupted);
	}
}

void UGarGameplayAbility_MontageBase::GetGameplayEffectsWhileAnimating(TArray<const UGameplayEffect*>& OutEffects) const
{
	for (TSubclassOf<UGameplayEffect> EffectClass : GameplayEffectClassesWhileAnimating)
	{
		if (EffectClass)
		{
			OutEffects.Add(EffectClass->GetDefaultObject<UGameplayEffect>());
		}
	}
}

bool UGarGameplayAbility_MontageBase::PlayMontage(const FGameplayAbilityActivationInfo& ActivationInfo, const FGarPlayMontageParameter& Parameter,
												  const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo)
{
	return PlayMontage(ActivationInfo, Parameter.MontageToPlay, Parameter.PlayRate, Parameter.SectionName, Parameter.StartTime, Handle, ActorInfo);
}

bool UGarGameplayAbility_MontageBase::PlayMontage(UAnimMontage* Montage, float PlayRate, FName SectionName, float StartTime)
{
	return PlayMontage(GetCurrentActivationInfo(), Montage, PlayRate, SectionName, StartTime, CurrentSpecHandle, GetCurrentActorInfo());
}

bool UGarGameplayAbility_MontageBase::PlayMontage(const FGameplayAbilityActivationInfo& ActivationInfo, const FGameplayAbilityActorInfo* ActorInfo,
												  const FGarPlayMontageParameter& Parameter)
{
	return PlayMontage(ActivationInfo, Parameter, CurrentSpecHandle, ActorInfo);
}

bool UGarGameplayAbility_MontageBase::PlayMontage(const FGarPlayMontageParameter& Parameter)
{
	return PlayMontage(GetCurrentActivationInfo(), Parameter, CurrentSpecHandle, GetCurrentActorInfo());
}

bool UGarGameplayAbility_MontageBase::PlayMontage(const FGameplayAbilityActivationInfo& ActivationInfo,
												  UAnimMontage* Montage, float PlayRate, FName SectionName, float StartTime,
												  const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo)
{
	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	auto* const AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();

	if (Montage != nullptr && AnimInstance != nullptr && AbilitySystemComponent)
	{
		AppliedEffects.Reset();

		// Apply GameplayEffects
		TArray<const UGameplayEffect*> Effects;
		GetGameplayEffectsWhileAnimating(Effects);
		if (Effects.Num() > 0)
		{
			for (const UGameplayEffect* Effect : Effects)
			{
				auto EffectHandle = AbilitySystemComponent->ApplyGameplayEffectToSelf(Effect, 1.f, MakeEffectContext(Handle, ActorInfo));
				if (EffectHandle.IsValid())
				{
					AppliedEffects.Add(EffectHandle);
				}
			}
		}

		CurrentMotangeDuration = AbilitySystemComponent->PlayMontage(this, ActivationInfo, Montage, PlayRate, SectionName, StartTime);

		if (CurrentMotangeDuration > 0.0f)
		{
			SetUpNotification(AnimInstance, Montage);

			if (Montage->HasRootMotion())
			{
				auto Character{Cast<AGarCharacter>(ActorInfo->OwnerActor.Get())};
				auto LayeredMove_AnimRootMotion = MakeShared<FLayeredMove_AnimRootMotion>();
				LayeredMove_AnimRootMotion->DurationMs = CurrentMotangeDuration * 1000;
				LayeredMove_AnimRootMotion->MixMode = MoveMixMode;
				LayeredMove_AnimRootMotion->Montage = Montage;
				LayeredMove_AnimRootMotion->StartingMontagePosition = StartTime;
				LayeredMove_AnimRootMotion->PlayRate = PlayRate;
				Character->GetMover()->QueueLayeredMove(LayeredMove_AnimRootMotion);
			}

			return true;
		}
	}
	return false;
}

void UGarGameplayAbility_MontageBase::SetUpNotification(UAnimInstance* AnimInstance, UAnimMontage* Montage)
{
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ThisClass::OnEndMontage);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

	if (auto* MontageInstance = AnimInstance->GetActiveInstanceForMontage(Montage))
	{
		// AnimInstance's OnPlayMontageNotifyBegin/End fire for all notify. Then stores Montage's InstanceID
		CurrentMontageInstanceId = MontageInstance->GetInstanceID();

		AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ThisClass::OnNotifyBeginReceived);
		AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &ThisClass::OnNotifyEndReceived);
	}
}

bool UGarGameplayAbility_MontageBase::IsNotifyValid(FName NotifyName, const FBranchingPointNotifyPayload& BPNPayload) const
{
    return CurrentMontageInstanceId != INDEX_NONE && BPNPayload.MontageInstanceID == CurrentMontageInstanceId;
}

void UGarGameplayAbility_MontageBase::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BPNPayload)
{
    if (IsNotifyValid(NotifyName, BPNPayload))
    {
		if (NotifyName == FName(TEXT("EndAbility")))
		{
			EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
		}

        float TriggerTime = BPNPayload.NotifyEvent ? BPNPayload.NotifyEvent->GetTriggerTime() : 0.f;
        float Duration = BPNPayload.NotifyEvent ? BPNPayload.NotifyEvent->GetDuration() : 0.f;
        OnNotifyBegin.Broadcast(NotifyName, TriggerTime, Duration);
    }
}

void UGarGameplayAbility_MontageBase::OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BPNPayload)
{
    if (IsNotifyValid(NotifyName, BPNPayload))
    {
        float TriggerTime = BPNPayload.NotifyEvent ? BPNPayload.NotifyEvent->GetTriggerTime() : 0.f;
        float Duration = BPNPayload.NotifyEvent ? BPNPayload.NotifyEvent->GetDuration() : 0.f;
        OnNotifyEnd.Broadcast(NotifyName, TriggerTime, Duration);
    }
}
