// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GarCharacterTask.h"
#include "GarLocalMontageTask.generated.h"

/**
 *
 */
UCLASS(Abstract)
class GAR_API UGarLocalMontageTask : public UGarCharacterTask
{
	GENERATED_BODY()

	friend class UGarLocalMontageComponent;
	friend class UGarAbilityTask_PlayLocalMontage;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LocalMontageTask|Settings", Transient)
	uint8 bStopCurrentMontageOnEnd : 1{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalMontageTask|State", Transient)
	TWeakObjectPtr<class UGarLocalMontageComponent> Component;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalMontageTask|State", Transient)
	TWeakObjectPtr<class UAnimMontage> CurrentMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalMontageTask|State", Transient)
	TWeakObjectPtr<class UGarAbilityTask_PlayLocalMontage> CurrentAbilityTask;

	UFUNCTION(BlueprintCallable, Category = "GAR|LocalMontageTask")
	virtual bool Play(const struct FGarPlayMontageParameter& Parameter);

	UFUNCTION(BlueprintCallable, Category = "GAR|LocalMontageTask")
	virtual void Stop(float OverrideBlendOutTime = -1.0f);

	UFUNCTION()
	virtual void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);

	UFUNCTION()
	virtual void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|LocalMontageTask", DisplayName = "On Notify Begin Received", Meta = (ScriptName = "OnNotifyBeginReceived"))
	void K2_OnNotifyBegin(FName NotifyName);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|LocalMontageTask", DisplayName = "On Notify End Received", Meta = (ScriptName = "OnNotifyEndReceived"))
	void K2_OnNotifyEnd(FName NotifyName);

	void OnEndMontage(class UAnimMontage* Montage, bool bInterrupted);

	virtual void OnEnd(bool bWasCancelled) override;

private:
	int32 MontageInstanceID{INDEX_NONE};

	bool IsNotifyValid(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) const;
};
