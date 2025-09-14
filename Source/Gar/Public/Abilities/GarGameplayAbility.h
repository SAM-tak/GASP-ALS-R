// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GarGameplayTags.h"
#include "GarGameplayAbility.generated.h"

class AGarCharacter;
class UGarAbilitySystemComponent;
class UAnimMontage;

USTRUCT(BlueprintType)
struct GAR_API FGarPlayMontageParameter
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = GarMontageAbility)
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY(EditDefaultsOnly, Category = GarMontageAbility)
	float PlayRate{1.0f};

	UPROPERTY(EditDefaultsOnly, Category = GarMontageAbility)
	FName SectionName;

	UPROPERTY(EditDefaultsOnly, Category = GarMontageAbility, Meta = (ForceUnit = "s"))
	float StartTime{0.0f};
};

/**
 * GameplayAbility for GAR
 * Can bind Input action
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility : public UGameplayAbility
{
	GENERATED_UCLASS_BODY()

	friend class UGarAbilitySystemComponent;

protected:
	// Bind input actions on activate this ability and unbind when finished. (default : true)
	// If activates too frequency and no needs to input binding, turn off for better performance.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GarAbility)
	uint8 bEnableInputBinding : 1{true};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GarAbility)
	uint8 bStopCurrentMontageOnEndAbility : 1{false};

	// A minus value means not override (default)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GarAbility)
	float OverrideBlendOutTimeOnEndAbility{-1.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|State", Transient)
	uint8 bInputBinded : 1{false};

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Ability")
	AGarCharacter* GetGarCharacterFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category = "GAR|Ability")
	UGarAbilitySystemComponent* GetGarAbilitySystemComponentFromActorInfo() const;

	virtual UWorld* GetWorld() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool PlayMontage(const FGameplayAbilityActivationInfo& ActivationInfo, const FGameplayAbilityActorInfo* ActorInfo, const FGarPlayMontageParameter& Parameter);

	virtual bool PlayMontage(const FGarPlayMontageParameter& Parameter);

	void StopCurrentMontage(const FGameplayAbilityActorInfo* ActorInfo, float OverrideBlendOutTime = -1.0f) const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Ability|Montage")
	void StopCurrentMontage(float OverrideBlendOutTime = -1.0f) const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Ability")
	void SetInputBlocked(bool bBlocked) const;

	virtual void OnPossessed(AController* NewController);

	virtual void OnUnPossessed(AController* PreviousController);

	void BindInput(UInputComponent* InputComponent);

	void UnbindInput(UInputComponent* InputComponent);
};
