// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GarCharacterTask.h"
#include "GarOverlayTask.generated.h"

class UGarCharacterTaskAnimInstance;
class UGarOverlayModeComponent;

/**
 *
 */
UCLASS(Abstract)
class GAR_API UGarOverlayTask : public UGarCharacterTask
{
	GENERATED_BODY()

	friend UGarOverlayModeComponent;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UGarCharacterTaskAnimInstance> OverlayAnimClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarOverlayModeComponent> Component;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGarCharacterTaskAnimInstance> OverlayAnimInstance;

public:
	virtual void Begin() override;

	virtual void Refresh(float DeltaTime) override;

protected:
	virtual void OnFinished() override;
};
