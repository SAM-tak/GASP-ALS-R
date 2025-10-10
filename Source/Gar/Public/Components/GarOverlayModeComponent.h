#pragma once

#include "GarCharacterComponent.h"
#include "GameplayTags.h"
#include "GarOverlayModeComponent.generated.h"

class UGarOverlayTask;

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

protected:
	virtual void BeginPlay() override;

	virtual void OnRefresh_Implementation(float DeltaTime) override;

	virtual void OnPossessed_Implementation(AController* NewController) override;

	virtual void OnUnPossessed_Implementation(AController* PreviousController) override;

	void ChangeOverlayTask(const FGameplayTag& OverlayMode);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ChangeOverlayTask(const FGameplayTag& Tag);

public:
	void RegisterOverlayTask(const FGameplayTag& OverlayMode, TSubclassOf<UGarOverlayTask> OverlayTaskClass);

	void UnregisterOverlayTask(const FGameplayTag& OverlayMode);
};
