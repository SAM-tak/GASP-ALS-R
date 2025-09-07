// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Actions/GarGameplayAbility_MontageBase.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "GarGameplayAbility_MotionMatchBase.generated.h"

class UAbilitySystemComponent;

/**
 *	A gameplay ability that plays a single montage and applies a GameplayEffect
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_MotionMatchBase : public UGarGameplayAbility_MontageBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|MotionMatch")
	FName PoseHistoryName{TEXTVIEW("PoseHistory")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|MotionMatch")
	FPoseSearchContinuingProperties PoseSearchContinuingProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|MotionMatch")
	FPoseSearchFutureProperties PoseSearchFuture;

	FPoseSearchBlueprintResult MotionMatch(TArray<UObject*> AssetsToSearch) const;
};
