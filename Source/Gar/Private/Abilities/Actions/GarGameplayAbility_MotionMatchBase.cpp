// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Actions/GarGameplayAbility_MotionMatchBase.h"
#include "GarAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_MotionMatchBase)

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGarGameplayAbility_MotionMatchBase
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

FPoseSearchBlueprintResult UGarGameplayAbility_MotionMatchBase::MotionMatch(TArray<UObject*> AssetsToSearch) const
{
	FPoseSearchBlueprintResult Result;
	UAnimInstance* AnimInstance = GetCurrentActorInfo()->GetAnimInstance();
	UPoseSearchLibrary::MotionMatch(AnimInstance, AssetsToSearch, PoseHistoryName, PoseSearchContinuingProperties, PoseSearchFuture, Result);
	return Result;
}
