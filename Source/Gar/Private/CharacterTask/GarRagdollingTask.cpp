// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarRagdollingTask.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarPhysicalAnimationComponent.h"
#include "LinkedAnimLayers/GarRagdollingOverrideAnimInstance.h"
#include "LinkedAnimLayers/GarRagdollingAnimInstance.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarRagdollingTask)

void UGarRagdollingTask::Begin()
{
	Super::Begin();

	if(OverrideAnimInstance.IsValid())
	{
		auto RagdollingOverrideAnimInstance{Cast<UGarRagdollingOverrideAnimInstance>(OverrideAnimInstance.Get())};
		if (RagdollingOverrideAnimInstance)
		{
			RagdollingOverrideAnimInstance->Reset();
			RagdollingOverrideAnimInstance->SetRagdollingTaskActive(true);

			auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
			auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};
			RagdollingState.RagdollingAnimInstance->SetRagdollingTaskActive(true);
		}
	}
}

void UGarRagdollingTask::End()
{
	Super::End();

	if (OverrideAnimInstance.IsValid())
	{
		auto RagdollingOverrideAnimInstance{Cast<UGarRagdollingOverrideAnimInstance>(OverrideAnimInstance.Get())};
		if (RagdollingOverrideAnimInstance)
		{
			RagdollingOverrideAnimInstance->Reset();
		}
	}
}

void UGarRagdollingTask::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
	auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};

	if (!IsActive() || RagdollingState.bFreezing)
	{
		return;
	}

	if (RagdollingState.IsGroundedAndAged())
	{
		if (!bOnGroundedAndAgedFired)
		{
			bOnGroundedAndAgedFired = true;
			K2_OnGroundedAndAged();
		}
		Character->SetInputStance(GarStanceTags::Lying);

		// local only. not be replicated.
		Character->GetGarAbilitySystem()->SetLooseGameplayTagCount(GarStateFlagTags::FacingUpward, RagdollingState.bFacingUpward ? 1 : 0);
	}
	else
	{
		if (RagdollingState.bGrounded)
		{
			Character->SetInputStance(GarStanceTags::Crouching);
		}
		bOnGroundedAndAgedFired = false;
	}
}

bool UGarRagdollingTask::IsEpilogRunning_Implementation() const
{
	if (OverrideAnimInstance.IsValid())
	{
		auto RagdollingOverrideAnimInstance{Cast<UGarRagdollingOverrideAnimInstance>(OverrideAnimInstance.Get())};
		if (RagdollingOverrideAnimInstance)
		{
			bool bRagdollingTaskActive{RagdollingOverrideAnimInstance->GetRagdollingTaskActive()};
			float BlendWeight{RagdollingOverrideAnimInstance->GetObservingFinalBlendWeight()};
			UE_LOG(LogTemp, Log, TEXT("bActive:%d ObservingFinalBlendWeight:%0.2f (%d)"), bRagdollingTaskActive, BlendWeight,
				bRagdollingTaskActive || BlendWeight < 1.0f);
			return bRagdollingTaskActive || BlendWeight < 1.0f;
		}
	}
	return false;
}

void UGarRagdollingTask::OnFinished()
{
	Super::OnFinished();

	auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
	auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};
	RagdollingState.RagdollingAnimInstance->SetRagdollingTaskActive(false);
}

bool UGarRagdollingTask::IsGroundedAndAged() const
{
	auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
	auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};
	return RagdollingState.IsGroundedAndAged();
}
