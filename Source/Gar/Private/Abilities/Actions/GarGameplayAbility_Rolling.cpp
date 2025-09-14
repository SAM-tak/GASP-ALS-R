// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/Actions/GarGameplayAbility_Rolling.h"
#include "Abilities/Tasks/GarAbilityTask_Tick.h"
#include "GarCharacter.h"
#include "GarAnimationInstance.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacterMovementComponent.h"
#include "GarGameplayTags.h"
#include "Utility/GarMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_Rolling)

UGarGameplayAbility_Rolling::UGarGameplayAbility_Rolling(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarLocomotionActionTags::Rolling));
	ActivationOwnedTags.AddTag(GarLocomotionActionTags::Rolling);
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
	auto* Character{GetGarCharacterFromActorInfo()};

	if (Character->GetLocalRole() < ROLE_Authority)
	{
		Character->GetCharacterMovement()->FlushServerMoves();
	}

	TargetYawAngle = CalcTargetYawAngle();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive())
	{
		TickTask = UGarAbilityTask_Tick::New(this, FName(TEXT("UGarGameplayAbility_Rolling")));
		if (TickTask.IsValid())
		{
			TickTask->OnTick.AddDynamic(this, &ThisClass::Tick);
			TickTask->ReadyForActivation();
		}

		if (Character->GetLocalRole() <= ROLE_SimulatedProxy ||
			Character->GetMesh()->GetAnimInstance()->RootMotionMode <= ERootMotionMode::IgnoreRootMotion)
		{
			PhysicsRotationHandle.Reset();
		}
		else
		{
			PhysicsRotationHandle = Character->GetGarCharacterMovement()->OnPhysicsRotation.AddUObject(this, &ThisClass::RefreshRolling);
		}

		if (bCrouchOnStart)
		{
			Character->Crouch();
		}
	}
}

void UGarGameplayAbility_Rolling::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
											 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	auto* Character{GetGarCharacterFromActorInfo()};

	if (PhysicsRotationHandle.IsValid())
	{
		Character->GetGarCharacterMovement()->OnPhysicsRotation.Remove(PhysicsRotationHandle);
		PhysicsRotationHandle.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGarGameplayAbility_Rolling::Tick_Implementation(const float DeltaTime)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	auto* AnimInstance{Character->GetMesh()->GetAnimInstance()};
	if (!PhysicsRotationHandle.IsValid())
	{
		// Refresh rolling physics here because AGarCharacter::PhysicsRotation()
		// won't be called on simulated proxies or with ignored root motion.

		RefreshRolling(DeltaTime);
	}

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

// ReSharper disable once CppMemberFunctionMayBeConst
void UGarGameplayAbility_Rolling::RefreshRolling(const float DeltaTime)
{
	auto* Character{GetGarCharacterFromActorInfo()};
	auto TargetRotation{Character->GetCharacterMovement()->UpdatedComponent->GetComponentRotation()};

	if (RotationInterpolationSpeed <= 0.0f)
	{
		TargetRotation.Yaw = TargetYawAngle;

		Character->GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		TargetRotation.Yaw = UGarMath::ExponentialDecayAngle(UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(TargetRotation.Yaw)),
		                                                     TargetYawAngle, DeltaTime,
		                                                     RotationInterpolationSpeed);

		Character->GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false);
	}
}
