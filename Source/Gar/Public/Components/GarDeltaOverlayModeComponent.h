#pragma once

#include "GarCharacterComponent.h"
#include "GameplayTags.h"
#include "GarDeltaOverlayModeComponent.generated.h"

class UGarDeltaOverlayTask;
class UGarAbilitySystemComponent;

UCLASS(AutoExpandCategories = ("GarOverlayModeComponent|Settings"), Meta = (BlueprintSpawnableComponent))
class GAR_API UGarDeltaOverlayModeComponent : public UGarCharacterComponent
{
	GENERATED_BODY()

protected:
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

	void ChangeDeltaOverlayTask(const FGameplayTag& NewDeltaOverlayMode);

	void CheckActiveAbility(UGarAbilitySystemComponent* AbilitySystem);

public:
	void RegisterDeltaOverlayTask(const FGameplayTag& DeltaOverlayMode, TSubclassOf<UGarDeltaOverlayTask> DeltaOverlayTaskClass);

	void UnregisterDeltaOverlayTask(const FGameplayTag& DeltaOverlayMode);
};
