// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GarCharacterTask.h"
#include "GarOverrideTask.generated.h"

class UGarCharacterTaskAnimInstance;
class UGarOverrideModeComponent;

/**
 *
 */
UCLASS(Abstract)
class GAR_API UGarOverrideTask : public UGarCharacterTask
{
	GENERATED_BODY()

	friend UGarOverrideModeComponent;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UGarCharacterTaskAnimInstance> OverrideAnimClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarOverrideModeComponent> Component;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarCharacterTaskAnimInstance> OverrideAnimInstance;

public:
	virtual void Begin() override;

	virtual void Refresh(float DeltaTime) override;

protected:
	virtual void OnFinished() override;
};
