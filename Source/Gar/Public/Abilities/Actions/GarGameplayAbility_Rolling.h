// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Actions/GarGameplayAbility_Montage.h"
#include "GarGameplayAbility_Rolling.generated.h"

/**
 * Rolling Action
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_Rolling : public UGarGameplayAbility_Montage
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Rolling")
	uint8 bCrouchOnStart : 1{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Rolling")
	uint8 bRotateToInputOnStart : 1{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Rolling", Meta = (ClampMin = 0))
	float RotationInterpolationSpeed{10.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Rolling")
	uint8 bCancelRollingWhenInAir : 1{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Rolling", Meta = (EditCondition = bCancelRollingWhenInAir, ForceUnits = "s"))
	float TimeToCancel{0.3f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Rolling", Meta = (EditCondition = bCancelRollingWhenInAir))
	FGameplayTag TryActiveWhenCancel{GarLocomotionActionTags::FreeFalling};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GarAbility|Rolling|State", Transient, Meta = (ForceUnits = "s"))
	float InAirTime{0.0f};

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Ability|Rolling")
	float CalcTargetYawAngle() const;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Ability|Rolling")
	void Tick(const float DeltaTime);

private:
	TWeakObjectPtr<class UGarAbilityTask_Tick> TickTask;
};
