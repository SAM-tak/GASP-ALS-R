// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/Actions/GarGameplayAbility_Rolling.h"

#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/GarAbilityTask_Tick.h"
#include "GarCharacter.h"
#include "GarAnimationInstance.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacterMoverComponent.h"
#include "GarGameplayTags.h"
#include "Utility/GarMath.h"
#include "MoverSimulationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Rolling)

UGarGameplayAbility_Rolling::UGarGameplayAbility_Rolling(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarLocomotionActionTags::Rolling));
	ActivationOwnedTags.AddTag(GarLocomotionActionTags::Rolling);
	ActivationOwnedTags.AddTag(GarStateFlagTags::RotationLocked);
	CancelAbilitiesWithTag.AddTag(GarLocomotionActionTags::Root);
	BlockAbilitiesWithTag.AddTag(GarLocomotionActionTags::Rolling);
}

float UGarGameplayAbility_Rolling::CalcTargetYawAngle_Implementation() const
{
	auto* Character{GetGarCharacterFromActorInfo()};

	if (!bRotateToInputOnStart)
	{
		return UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw));
	}

	// ローカル制御下: MovementInputVector から直接取得
	if (Character->HasMovementInput())
	{
		return Character->GetInputYawAngle();
	}

	// サーバー上のリモートAP: Mover の最終入力コマンドから移動方向を取得
	// (サーバーでは ConsumeMovementInputVector() がゼロを返すため HasMovementInput() が常に false になる)
	if (UGarCharacterMoverComponent* MoverComp = Character->GetMover())
	{
		const FMoverInputCmdContext& LastInputCmd = MoverComp->GetLastInputCmd();
		if (const FCharacterDefaultInputs* CharInputs = LastInputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>())
		{
			const FVector MoveInput = CharInputs->GetMoveInput_WorldSpace();
			if (MoveInput.SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
			{
				return UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(MoveInput));
			}
		}
	}

	return UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw));
}

void UGarGameplayAbility_Rolling::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
												  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive())
	{
		auto* Character{GetGarCharacterFromActorInfo()};
		auto MotionWarpingComponent = Character->GetMotionWarping();

		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(FName(TEXTVIEW("Turn")), FVector::ZeroVector,
			FRotator(0.0, CalcTargetYawAngle(), 0.0));

		TickTask = UGarAbilityTask_Tick::New(this, FName(TEXT("UGarGameplayAbility_Rolling")));
		if (TickTask.IsValid())
		{
			TickTask->OnTick.AddDynamic(this, &ThisClass::Tick);
			TickTask->ReadyForActivation();
		}

		if (bCrouchOnStart)
		{
			Character->SetInputStance(GarStanceTags::Crouching);
		}
	}
}

void UGarGameplayAbility_Rolling::Tick_Implementation(const float DeltaTime)
{
	if (bCancelRollingWhenInAir)
	{
		if(GetGarAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(GarLocomotionModeTags::InAir))
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
