// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/GarGameplayAbility_OverlayMode.h"
#include "Abilities/Tasks/GarAbilityTask_Tick.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarAnimationInstance.h"
#include "GarAbilitySystemComponent.h"
#include "GarPhysicalAnimationComponent.h"
#include "LinkedAnimLayers/GarOverlayAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/GarOverrideModeComponent.h"
#include "Net/UnrealNetwork.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayAbility_OverlayMode)

UGarGameplayAbility_OverlayMode::UGarGameplayAbility_OverlayMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(GarOverlayModeTags::Default));
	ActivationOwnedTags.AddTag(GarOverlayModeTags::Default);
	CancelAbilitiesWithTag.AddTag(GarOverlayModeTags::Root);
	BlockAbilitiesWithTag.AddTag(GarOverlayModeTags::Default);
}
