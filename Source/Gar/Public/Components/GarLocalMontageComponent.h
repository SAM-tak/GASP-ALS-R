#pragma once

#include "GarCharacterComponent.h"
#include "GameplayTags.h"
#include "GarLocalMontageComponent.generated.h"

class UGarLocalMontageTask;

UCLASS(Abstract, AutoExpandCategories = ("GarLocalMontageModeComponent|Settings"))
class GAR_API UGarLocalMontageComponent : public UGarCharacterComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarLocalMontageModeComponent|Settings", Meta = (DisplayThumbnail = false))
	TMap<FGameplayTag, TSubclassOf<UGarLocalMontageTask>> LocalMontageTaskClassMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarLocalMontageModeComponent|State", Transient)
	FGameplayTag CurrentLocalMontageTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarLocalMontageModeComponent|State", Transient)
	TWeakObjectPtr<UGarLocalMontageTask> CurrentLocalMontageTask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarLocalMontageModeComponent|State", Transient)
	FGameplayTagContainer LocalMontageTagsMask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarLocalMontageModeComponent|State", Transient)
	TMap<FGameplayTag, TObjectPtr<UGarLocalMontageTask>> InstancedLocalMontageTasks;

public:
	UGarLocalMontageTask* Play(const FGameplayTag& LocalMontageTag);

	void OnEndTask(class UGarLocalMontageTask *Task);

protected:
	virtual void BeginPlay() override;

	virtual void OnOwnerTick_Implementation(float DeltaTime) override;

	virtual void OnPossessed_Implementation(AController* NewController) override;

	virtual void OnUnPossessed_Implementation(AController* PreviousController) override;

	UFUNCTION(BlueprintCallable, Category = "GarLocalMontageModeComponent")
	void AddOrUpdateReplicatedWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector TargetLocation, FRotator TargetRotation);

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlay(const FGameplayTag& LocalMontageTag);

	UGarLocalMontageTask* PlayImplementation(const FGameplayTag& LocalMontageTag);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAddOrUpdateWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector_NetQuantize TargetLocation, FRotator TargetRotation);
};
