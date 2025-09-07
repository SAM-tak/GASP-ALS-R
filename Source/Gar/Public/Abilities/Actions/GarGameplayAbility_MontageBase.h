// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GarGameplayAbility_Action.h"
#include "GarGameplayAbility_MontageBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGarMontageNotifyDelegate, FName, NotifyName, float, TriggerTime, float, Duration);

class UAbilitySystemComponent;

/**
 *	A gameplay ability that plays a single montage and applies a GameplayEffect
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_MontageBase : public UGarGameplayAbility_Action
{
	GENERATED_BODY()

protected:
	/** GameplayEffects to apply and then remove while the animation is playing */
	UPROPERTY(EditDefaultsOnly, Category = "GarAbility|Montage")
	TArray<TSubclassOf<UGameplayEffect>> GameplayEffectClassesWhileAnimating;

	UPROPERTY(EditDefaultsOnly, Category = "GarAbility|Montage", Meta = (ForceUnit = "s"))
	float BlendOutDurationOnCancel{0.15f};

	UPROPERTY(BlueprintAssignable, Category = "GarAbility|Montage")
	FGarMontageNotifyDelegate OnNotifyBegin;

	UPROPERTY(BlueprintAssignable, Category = "GarAbility|Montage")
	FGarMontageNotifyDelegate OnNotifyEnd;

	UPROPERTY(VisibleAnywhere, Category = "GarAbility|Montage|State", Transient)
	float CurrentMotangeDuration{0.f};

	UPROPERTY(VisibleAnywhere, Category = "GarAbility|Montage|State", Transient)
	int32 CurrentMontageInstanceId{INDEX_NONE};

private:
	TArray<struct FActiveGameplayEffectHandle> AppliedEffects;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Ability|Montage")
	void OnEndMontage(UAnimMontage *Montage, bool bInterrupted);

	void SetUpNotification(UAnimInstance* AnimInstance, UAnimMontage* Montage);

	void GetGameplayEffectsWhileAnimating(TArray<const UGameplayEffect *> &OutEffects) const;

	bool PlayMontage(const FGameplayAbilityActivationInfo& ActivationInfo, const FGarPlayMontageParameter& Parameter,
					 const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo);

	bool PlayMontage(const FGameplayAbilityActivationInfo& ActivationInfo, UAnimMontage* Montage, float PlayRate, FName SectionName, float StartTime,
					 const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo);

	virtual bool PlayMontage(const FGameplayAbilityActivationInfo& ActivationInfo, const FGameplayAbilityActorInfo* ActorInfo,
							 const FGarPlayMontageParameter& Parameter) override;

	virtual bool PlayMontage(const FGarPlayMontageParameter& Parameter) override;

private:
	UFUNCTION(BlueprintCallable, Category = "GAR|Ability|Montage")
	bool PlayMontage(UAnimMontage* Montage, float PlayRate = 1.0f, FName SectionName = NAME_None, float StartTime = 0.0f);

	bool IsNotifyValid(FName NotifyName, const FBranchingPointNotifyPayload& BPNPayload) const;

	UFUNCTION()
	void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BPNPayload);

	UFUNCTION()
	void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BPNPayload);
};
