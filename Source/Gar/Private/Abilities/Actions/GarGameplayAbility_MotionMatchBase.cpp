// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Actions/GarGameplayAbility_MotionMatchBase.h"
#include "GarAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

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
	if (!AnimInstance)
	{
		return Result;
	}

	// Block until any in-flight parallel animation evaluation task completes.
	// PoseHistory is written by FAnimNode_PoseSearchHistoryCollector during parallel eval (worker thread)
	// and read here on the game thread; without synchronization this is a data race caught by UE_MT_SCOPED_WRITE_ACCESS.
	if (USkeletalMeshComponent* SkelMesh = AnimInstance->GetOwningComponent())
	{
		SkelMesh->HandleExistingParallelEvaluationTask(/*bBlockOnTask=*/true, /*bPerformPostAnimEvaluation=*/false);
	}

	UPoseSearchLibrary::MotionMatch(AnimInstance, AssetsToSearch, PoseHistoryName, PoseSearchContinuingProperties, PoseSearchFuture, Result);
	return Result;
}
