#pragma once

#include "GarCharacterComponent.h"
#include "GameplayTags.h"
#include "GarOverrideModeComponent.generated.h"

class UGarOverrideTask;

UCLASS(AutoExpandCategories = ("GarOverrideModeComponent|Settings"), Meta = (BlueprintSpawnableComponent))
class GAR_API UGarOverrideModeComponent : public UGarCharacterComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient, Meta = (DisplayThumbnail = false))
	TMap<FGameplayTag, TSubclassOf<UGarOverrideTask>> OverrideClassMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient)
	FGameplayTag CurrentOverrideTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient)
	TWeakObjectPtr<UGarOverrideTask> CurrentOverrideTask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient)
	TMap<FGameplayTag, TObjectPtr<UGarOverrideTask>> InstancedOverrideTasks;

public:
	UFUNCTION(BlueprintCallable)
	void EndCurrentRagdollingTask();

	void RegisterOverrideTask(const FGameplayTag& OverrideMode, TSubclassOf<UGarOverrideTask> OverrideTaskClass);

	void UnregisterOverrideTask(const FGameplayTag& OverrideMode);

protected:
	virtual void BeginPlay() override;

	virtual void OnRefresh_Implementation(float DeltaTime) override;

	virtual void OnPossessed_Implementation(AController* NewController) override;

	virtual void OnUnPossessed_Implementation(AController* PreviousController) override;

	void ChangeOverrideTask(const FGameplayTag& Tag);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ChangeOverrideTask(const FGameplayTag& Tag);
};
