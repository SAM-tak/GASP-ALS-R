// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GarCharacterTask.h"
#include "GarDeltaOverlayTask.generated.h"

class UGarDeltaOverlayAnimInstance;
class UGarDeltaOverlayModeComponent;

/**
 *
 */
UCLASS(Abstract)
class GAR_API UGarDeltaOverlayTask : public UGarCharacterTask
{
	GENERATED_BODY()

	friend UGarDeltaOverlayModeComponent;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UGarDeltaOverlayAnimInstance> DeltaOverlayAnimClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarDeltaOverlayModeComponent> Component;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarDeltaOverlayAnimInstance> DeltaOverlayAnimInstance;

public:
	virtual void Begin() override;

protected:
	virtual void OnFinished() override;
};
