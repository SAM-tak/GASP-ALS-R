// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/Actions/GarGameplayAbility_Sliding.h"

#include "Abilities/Tasks/GarAbilityTask_Tick.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Sliding)

UGarGameplayAbility_Sliding::UGarGameplayAbility_Sliding(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarLocomotionActionTags::Sliding));
	ActivationOwnedTags.AddTag(GarLocomotionActionTags::Sliding);
	CancelAbilitiesWithTag.AddTag(GarLocomotionActionTags::Root);
	BlockAbilitiesWithTag.AddTag(GarLocomotionActionTags::Sliding);
}

void UGarGameplayAbility_Sliding::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
												  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	RemainingSlidingModeCheckDelayTicks = FMath::Max(0, SlidingModeCheckDelayTicks);
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!IsActive())
	{
		return;
	}

	auto* Character{GetGarCharacterFromActorInfo()};

	if (bCrouchOnStart)
	{
		Character->SetInputStance(GarStanceTags::Crouching);
	}

	// カメラ方向に対して横移動中かを判定し、横移動なら KneesOut タグを付与
	bKneesOut = false;
	if (IsValid(Character))
	{
		const FVector VelDir = Character->GetVelocity().GetSafeNormal2D();
		if (!VelDir.IsNearlyZero())
		{
			const FRotator CamYaw(0.0, Character->GetViewRotation().Yaw, 0.0);
			const FVector CamForward = CamYaw.Vector();
			const FVector CamRight = FRotationMatrix(CamYaw).GetScaledAxis(EAxis::Y);
			const float ForwardDot = FMath::Abs(FVector::DotProduct(VelDir, CamForward));
			const float RightDot   = FMath::Abs(FVector::DotProduct(VelDir, CamRight));
			bKneesOut = RightDot > ForwardDot;
		}
	}

	if (bKneesOut)
	{
		if (auto* ASC = GetGarAbilitySystemComponentFromActorInfo())
		{
			ASC->AddLooseGameplayTag(GarSlidingActionTags::KneesOut);
		}
	}

	TickTask = UGarAbilityTask_Tick::New(this, FName(TEXT("UGarGameplayAbility_Sliding")));
	if (TickTask.IsValid())
	{
		TickTask->OnTick.AddDynamic(this, &ThisClass::Tick);
		TickTask->ReadyForActivation();
	}
}

void UGarGameplayAbility_Sliding::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
											 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bKneesOut)
	{
		if (auto* ASC = GetGarAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveLooseGameplayTag(GarSlidingActionTags::KneesOut);
		}
		bKneesOut = false;
	}

	if (bStandUpOnEnd)
	{
		if (auto* Character = GetGarCharacterFromActorInfo())
		{
			Character->SetInputStance(GarStanceTags::Standing);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGarGameplayAbility_Sliding::Tick_Implementation(const float DeltaTime)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	if (!IsValid(Character))
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

	UGarCharacterMoverComponent* MoverComp = Character->GetMover();
	if (!IsValid(MoverComp))
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

	const FName CurrentMovementMode = MoverComp->GetMovementModeName();
	if (CurrentMovementMode == TEXT("Sliding"))
	{
		RemainingSlidingModeCheckDelayTicks = 0;
		return;
	}

	// 起動直後はモード反映遅延を許容
	if (RemainingSlidingModeCheckDelayTicks > 0)
	{
		--RemainingSlidingModeCheckDelayTicks;
		return;
	}

	// Sliding モードを抜けたら終了（空中遷移含む）
	if (CurrentMovementMode != TEXT("Sliding"))
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}
