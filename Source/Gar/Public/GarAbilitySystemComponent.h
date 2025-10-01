#pragma once

#include "AbilitySystemComponent.h"
#include "InputTriggers.h"
#include "GarAbilitySystemComponent.generated.h"

class UEnhancedInputComponent;
class UInputAction;
class UGarAbilitySet;
class AGarCharacter;

/**
 * AbilitySystemComponent for GAR Refactored
 */
UCLASS()
class GAR_API UGarAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UGarAbilitySystemComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbilitySystem|Settings")
	TObjectPtr<UGarAbilitySet> AbilitySet;

protected:
	virtual void OnRegister() override;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|AbilitySystem")
	void OnPossessed(AController* NewController);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|AbilitySystem")
	void OnUnPossessed(AController* PreviousController);

	UFUNCTION(BlueprintCallable, Category = "GAR|AbilitySystem", DisplayName = "CancelAbilityByTags",
			  Meta = (ScriptName = "CancelAbilityByTags", AutoCreateRefTerm = "Tags"))
	void K2_CancelAbilityByTags(const FGameplayTagContainer& Tags)
	{
		CancelAbilities(&Tags);
	}

	UFUNCTION(BlueprintCallable, Category = "GAR|AbilitySystem", DisplayName = "CancelAbilityBySingleTag",
			  Meta = (ScriptName = "CancelAbilityBySingleTag", AutoCreateRefTerm = "Tag"))
	void K2_CancelAbilityBySingleTag(const FGameplayTag& Tag)
	{
		CancelAbilitiesBySingleTag(Tag);
	}

	// Initalize

public:
	virtual void Initialize(AGarCharacter* InOwnerCharacter);

	// Utility

public:
	inline void CancelAbilitiesBySingleTag(const FGameplayTag& Tag)
	{
		FGameplayTagContainer TagContainer{Tag};
		CancelAbilities(&TagContainer);
	}

	inline bool TryActivateAbilitiesBySingleTag(const FGameplayTag& Tag, bool bAllowRemoteActivation = true)
	{
		return TryActivateAbilitiesByTag(FGameplayTagContainer{Tag}, bAllowRemoteActivation);
	}

	// Input binding

public:
	void BindAbilityActivationInput(UEnhancedInputComponent* EnhancedInputComponent, const UInputAction* Action, ETriggerEvent TriggerEvent, const FGameplayTag& InputTag);

	void UnbindAbilityInputs(UEnhancedInputComponent* EnhancedInputComponent, const FGameplayTag& InputTag);

private:
	TMap<FGameplayTag, TArray<uint32>> BindingHandles;

	void ActivateOnInputAction(FGameplayTag InputTag);
};
