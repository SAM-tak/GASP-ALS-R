// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarLocalMontageTask.h"
#include "GarCharacter.h"
#include "Components/GarLocalMontageComponent.h"
#include "Abilities/Tasks/GarAbilityTask_PlayLocalMontage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarLocalMontageTask)

bool UGarLocalMontageTask::Play(const FGarPlayMontageParameter& Parameter)
{
	auto* AnimInstance{Character->GetMesh()->GetAnimInstance()};
	if (AnimInstance->Montage_Play(Parameter.MontageToPlay, Parameter.PlayRate, EMontagePlayReturnType::MontageLength, Parameter.StartTime, false))
	{
		CurrentMontage = Parameter.MontageToPlay;

		// Start at a given Section.
		if (Parameter.SectionName != NAME_None)
		{
			AnimInstance->Montage_JumpToSection(Parameter.SectionName, Parameter.MontageToPlay);
		}

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ThisClass::OnEndMontage);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Parameter.MontageToPlay);

		if (auto* MontageInstance = AnimInstance->GetActiveInstanceForMontage(Parameter.MontageToPlay))
		{
			// AnimInstance's OnPlayMontageNotifyBegin/End fire for all notify. Then stores Montage's InstanceID
			MontageInstanceID = MontageInstance->GetInstanceID();

			AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ThisClass::OnNotifyBeginReceived);
			AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &ThisClass::OnNotifyEndReceived);
		}

		return true;
	}
	return false;
}

void UGarLocalMontageTask::Stop(float OverrideBlendOutTime)
{
	if (CurrentMontage.IsValid())
	{
		auto* AnimInstance{Character->GetMesh()->GetAnimInstance()};
		AnimInstance->Montage_Stop(OverrideBlendOutTime, CurrentMontage.Get());
	}
}

bool UGarLocalMontageTask::IsNotifyValid(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) const
{
	return MontageInstanceID != INDEX_NONE && BranchingPointNotifyPayload.MontageInstanceID == MontageInstanceID;
}

void UGarLocalMontageTask::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload)
{
	if (IsNotifyValid(NotifyName, BranchingPointNotifyPayload))
	{
		if (NotifyName == FName(TEXT("EndAbility")))
		{
			End();
		}

		if (CurrentAbilityTask.IsValid())
		{
			CurrentAbilityTask->OnNotifyBegin.Broadcast(NotifyName);
		}

		K2_OnNotifyBegin(NotifyName);
	}
}

void UGarLocalMontageTask::OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload)
{
	if (IsNotifyValid(NotifyName, BranchingPointNotifyPayload))
	{
		if (CurrentAbilityTask.IsValid())
		{
			CurrentAbilityTask->OnNotifyEnd.Broadcast(NotifyName);
		}

		K2_OnNotifyEnd(NotifyName);
	}
}

void UGarLocalMontageTask::OnEndMontage(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted)
	{
		Cancel();
	}
	else
	{
		End();
	}
}

void UGarLocalMontageTask::OnEnd(bool bWasCancelled)
{
	Super::OnEnd(bWasCancelled);

	if (CurrentMontage.IsValid())
	{
		auto* AnimInstance{Character->GetMesh()->GetAnimInstance()};
		if (AnimInstance)
		{
			auto* EndDelegatePtr{AnimInstance->Montage_GetEndedDelegate(CurrentMontage.Get())};
			if (EndDelegatePtr)
			{
				EndDelegatePtr->Unbind();
			}

			AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &ThisClass::OnNotifyBeginReceived);
			AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &ThisClass::OnNotifyEndReceived);
		}

		if (bStopCurrentMontageOnEnd && AnimInstance->Montage_IsPlaying(CurrentMontage.Get()))
		{
			Stop();
		}

		CurrentMontage.Reset();
	}
	MontageInstanceID = INDEX_NONE;

	if (CurrentAbilityTask.IsValid())
	{
		if (bWasCancelled)
		{
			CurrentAbilityTask->OnInterrupted.Broadcast(NAME_None);
		}
		else
		{
			CurrentAbilityTask->OnCompleted.Broadcast(NAME_None);
		}
		CurrentAbilityTask.Reset();
	}

	if (Component.IsValid())
	{
		Component->OnEndTask(this);
	}
}
