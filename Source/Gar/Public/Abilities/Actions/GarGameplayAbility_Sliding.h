// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GarGameplayAbility.h"
#include "GarGameplayAbility_Sliding.generated.h"

/**
 * Sliding Action
 *
 * スライディング移動モード (UGarMoverSlidingMode) がアクティブな間維持されるアビリティ。
 * GarLocomotionActionTags::Sliding タグを保持し、Mover がスライドモードを抜けると自動終了する。
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_Sliding : public UGarGameplayAbility
{
	GENERATED_UCLASS_BODY()

public:
	/** Blueprint の InputAction バインドから呼び出す。スライディング中の移動入力を設定する */
	UFUNCTION(BlueprintCallable, Category = "GAR|Ability|Sliding")
	void SetSlidingInput(FVector2D Input);

	/** Blueprint バインドから受け取ったスライディング移動入力 */
	FVector2D SlidingInput = FVector2D::ZeroVector;

protected:
	/** アクティブ化時にしゃがみスタンスを強制するか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Sliding")
	uint8 bCrouchOnStart : 1{true};

	/** スライド終了時に立ち上がるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Sliding")
	uint8 bStandUpOnEnd : 1{true};

    /** この XY 速度以下になったらスライド終了 (cm/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Sliding", meta = (ForceUnits = "cm/s"))
    float ExitSpeedThreshold = 200.0f;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Ability|Sliding")
	void Tick(const float DeltaTime);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	TWeakObjectPtr<class UGarAbilityTask_Tick> TickTask;

	/** ActivateAbility 時に横移動と判定されたか（EndAbility で KneesOut タグ削除に使用） */
	uint8 bKneesOut : 1{false};
};
