#pragma once

#include "GarCharacterComponent.h"
#include "GameplayTags.h"
#include "GarOverlayModeComponent.generated.h"

class UGarOverlayTask;

UCLASS(Abstract, AutoExpandCategories = ("GarOverlayModeComponent|Settings"))
class GAR_API UGarOverlayModeComponent : public UGarCharacterComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarOverlayModeComponent|Settings", Meta = (DisplayThumbnail = false))
	TMap<FGameplayTag, TSubclassOf<UGarOverlayTask>> OverlayClassMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient)
	TWeakObjectPtr<UGarOverlayTask> CurrentOverlayTask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverlayModeComponent|State", Transient)
	FGameplayTag CurrentOverlayTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarOverrideModeComponent|State", Transient)
	TMap<FGameplayTag, TObjectPtr<UGarOverlayTask>> InstancedOverlayTasks;

protected:
	virtual void OnRegister() override;

	virtual void BeginPlay() override;

	virtual void OnRefresh_Implementation(float DeltaTime) override;

	virtual void OnPossessed_Implementation(AController* NewController) override;

	virtual void OnUnPossessed_Implementation(AController* PreviousController) override;

	void ChangeOverlayTask(const FGameplayTag& OverlayMode);

private:
	UFUNCTION()
	void OnChangeOverlayModeTag(FGameplayTag Tag, int32 NewCount);
};
