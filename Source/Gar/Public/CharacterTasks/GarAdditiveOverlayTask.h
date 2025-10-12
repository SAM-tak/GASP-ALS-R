// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GarCharacterTask.h"
#include "GarAdditiveOverlayTask.generated.h"

class UGarAdditiveOverlayAnimInstance;
class UGarOverlayModeComponent;

/**
 *
 */
UCLASS(Abstract)
class GAR_API UGarAdditiveOverlayTask : public UGarCharacterTask
{
	GENERATED_BODY()

	friend UGarOverlayModeComponent;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UGarAdditiveOverlayAnimInstance> AdditiveOverlayAnimClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarOverlayModeComponent> Component;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarAdditiveOverlayAnimInstance> AdditiveOverlayAnimInstance;

public:
	virtual void Begin() override;

protected:
	virtual void OnFinished() override;
};
