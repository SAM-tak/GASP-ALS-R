// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarRagdollingTask.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarPhysicalAnimationComponent.h"
#include "LinkedAnimLayers/GarRagdollingAnimInstance.h"
#include "LinkedAnimLayers/GarOverrideAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarRagdollingTask)

void UGarRagdollingTask::Refresh(float DeltaTime)
{
	auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
	auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};

	Super::Refresh(DeltaTime);
	
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
		bool bCharacterTaskActive{OverrideAnimInstance->GetCharacterTaskActive()};
		float BlendWeight{OverrideAnimInstance->GetObservingFinalBlendWeight()};
		//UE_LOG(LogTemp, Log, TEXT("bActive:%d ObservingFinalBlendWeight:%0.2f (%d)"), CharacterTaskActive, BlendWeight,
		//	   bCharacterTaskActive || (0.f < BlendWeight && BlendWeight < 1.f));
		return bCharacterTaskActive || (0.f < BlendWeight && BlendWeight < 1.f);
	}
	return false;
}

bool UGarRagdollingTask::IsGroundedAndAged() const
{
	auto* PhysicalAnimation{Character->GetPhysicalAnimation()};
	auto& RagdollingState{PhysicalAnimation->GetRagdollingState()};
	return RagdollingState.IsGroundedAndAged();
}
