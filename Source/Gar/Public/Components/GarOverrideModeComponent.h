#pragma once

#include "GarCharacterComponent.h"
#include "GameplayTags.h"
#include "GarOverrideModeComponent.generated.h"

class UGarOverrideTask;

UCLASS(Abstract, AutoExpandCategories = ("GarOverrideModeComponent|Settings"))
class GAR_API UGarOverrideModeComponent : public UGarCharacterComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarOverrideModeComponent|Settings", Meta = (DisplayThumbnail = false))
	TMap<FGameplayTag, TSubclassOf<UGarOverrideTask>> OverrideClassMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient)
	FGameplayTag CurrentOverrideTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient)
	TWeakObjectPtr<UGarOverrideTask> CurrentOverrideTask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient)
	FGameplayTagContainer OverrideTagsMask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient)
	TMap<FGameplayTag, TObjectPtr<UGarOverrideTask>> InstancedOverrideTasks;

public:
	UFUNCTION(BlueprintCallable)
	void EndCurrentRagdollingTask();

protected:
	virtual void BeginPlay() override;

	virtual void OnRefresh_Implementation(float DeltaTime) override;

	virtual void OnPossessed_Implementation(AController* NewController) override;

	virtual void OnUnPossessed_Implementation(AController* PreviousController) override;

	void ChangeOverrideTaskIfNeeded(const FGameplayTag& Tag);
};
