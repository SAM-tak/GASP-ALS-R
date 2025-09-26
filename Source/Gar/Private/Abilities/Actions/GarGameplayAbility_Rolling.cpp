// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/Actions/GarGameplayAbility_Rolling.h"

#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "DefaultMovementSet/LayeredMoves/AnimRootMotionLayeredMove.h"
#include "Abilities/Tasks/GarAbilityTask_Tick.h"
#include "GarCharacter.h"
#include "GarAnimationInstance.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacterMoverComponent.h"
#include "GarGameplayTags.h"
#include "Utility/GarMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Rolling)

UGarGameplayAbility_Rolling::UGarGameplayAbility_Rolling(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarLocomotionActionTags::Rolling));
	ActivationOwnedTags.AddTag(GarLocomotionActionTags::Traversal);
	ActivationOwnedTags.AddTag(GarStateFlagTags::RotationLocked);
	CancelAbilitiesWithTag.AddTag(GarLocomotionActionTags::Root);
	BlockAbilitiesWithTag.AddTag(GarLocomotionActionTags::Rolling);
}

float UGarGameplayAbility_Rolling::CalcTargetYawAngle_Implementation() const
{
	auto* Character{GetGarCharacterFromActorInfo()};
	return bRotateToInputOnStart && Character->HasInput()
		? Character->GetInputYawAngle()
		: UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw));
}

void UGarGameplayAbility_Rolling::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
												  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive())
	{
		auto* Character{GetGarCharacterFromActorInfo()};
		auto* AbilitySystem{GetGarAbilitySystemComponentFromActorInfo()};

		auto LayeredMove_TurnTo = MakeShared<FGarLayeredMove_TurnTo>();
		LayeredMove_TurnTo->DurationMs = RotationInterpolationSpeed > 0.f ? 1.0f / RotationInterpolationSpeed * 1000 : 0;
		LayeredMove_TurnTo->StartRotation = Character->GetActorRotation();
		LayeredMove_TurnTo->TargetRotation = FRotator(0.0, CalcTargetYawAngle(), 0.0);
		Character->GetMover()->QueueLayeredMove(LayeredMove_TurnTo);

		auto LayeredMove_AnimRootMotion = MakeShared<FLayeredMove_AnimRootMotion>();
		LayeredMove_AnimRootMotion->Montage = MontageToPlay;
		LayeredMove_AnimRootMotion->StartingMontagePosition = StartTime;
		LayeredMove_AnimRootMotion->PlayRate = PlayRate;
		Character->GetMover()->QueueLayeredMove(LayeredMove_AnimRootMotion);

		TickTask = UGarAbilityTask_Tick::New(this, FName(TEXT("UGarGameplayAbility_Rolling")));
		if (TickTask.IsValid())
		{
			TickTask->OnTick.AddDynamic(this, &ThisClass::Tick);
			TickTask->ReadyForActivation();
		}

		if (bCrouchOnStart)
		{
			Character->Crouch();
		}
	}
}

void UGarGameplayAbility_Rolling::Tick_Implementation(const float DeltaTime)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	auto* AnimInstance{Character->GetMesh()->GetAnimInstance()};

	if (bCancelRollingWhenInAir)
	{
		if(Character->HasMatchingGameplayTag(GarLocomotionModeTags::InAir))
		{
			if (InAirTime >= TimeToCancel)
			{
				EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);

				if (TryActiveWhenCancel.IsValid())
				{
					GetGarAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesBySingleTag(TryActiveWhenCancel);
				}
			}
			else
			{
				InAirTime += DeltaTime;
			}
		}
		else if (InAirTime > 0.0f)
		{
			InAirTime = 0.0f;
		}
	}
}
