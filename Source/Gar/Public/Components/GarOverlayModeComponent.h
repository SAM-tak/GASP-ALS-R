#pragma once

#include "GarCharacterComponent.h"
#include "GameplayTags.h"
#include "GarOverlayModeComponent.generated.h"

class UGarOverlayTask;
class UGarDeltaOverlayTask;
class UGarAbilitySystemComponent;

UCLASS(AutoExpandCategories = ("GarOverlayModeComponent|Settings"), Meta = (BlueprintSpawnableComponent))
class GAR_API UGarOverlayModeComponent : public UGarCharacterComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient, Meta = (DisplayThumbnail = false))
	TMap<FGameplayTag, TSubclassOf<UGarOverlayTask>> OverlayClassMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient)
	TWeakObjectPtr<UGarOverlayTask> CurrentOverlayTask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient)
	FGameplayTag CurrentOverlayTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient)
	TMap<FGameplayTag, TObjectPtr<UGarOverlayTask>> InstancedOverlayTasks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient, Meta = (DisplayThumbnail = false))
	TMap<FGameplayTag, TSubclassOf<UGarDeltaOverlayTask>> DeltaOverlayClassMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient)
	TWeakObjectPtr<UGarDeltaOverlayTask> CurrentDeltaOverlayTask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient)
	FGameplayTag CurrentDeltaOverlayTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient)
	TMap<FGameplayTag, TObjectPtr<UGarDeltaOverlayTask>> InstancedDeltaOverlayTasks;

protected:
	virtual void OnRegister() override;

	virtual void BeginPlay() override;

	virtual void OnOwnerTick_Implementation(float DeltaTime) override;

	virtual void OnPossessed_Implementation(AController* NewController) override;

	virtual void OnUnPossessed_Implementation(AController* PreviousController) override;

	void ChangeOverlayTask(const FGameplayTag& NewOverlayMode);

	void ChangeDeltaOverlayTask(const FGameplayTag& NewDeltaOverlayMode);

	void CheckActiveAbility(UGarAbilitySystemComponent* AbilitySystem);

public:
	void RegisterOverlayTask(const FGameplayTag& OverlayMode, TSubclassOf<UGarOverlayTask> OverlayTaskClass);

	void UnregisterOverlayTask(const FGameplayTag& OverlayMode);

	void RegisterDeltaOverlayTask(const FGameplayTag& OverlayMode, TSubclassOf<UGarDeltaOverlayTask> DeltaOverlayTaskClass);

	void UnregisterDeltaOverlayTask(const FGameplayTag& OverlayMode);
};
