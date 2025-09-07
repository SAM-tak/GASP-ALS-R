#pragma once

#include "GarCharacterComponent.h"
#include "Abilities/GarAbilityInputAction.h"
#include "GarCharacterInputComponent.generated.h"

class UInputMappingContext;

UCLASS(Abstract, AutoExpandCategories = ("GarCharacterInput|Settings"))
class GAR_API UGarCharacterInputComponent : public UGarCharacterComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacterInput|Settings")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacterInput|Settings")
	TArray<FGarAbilityInputAction> AbilityInputActions;

protected:
	virtual void OnRegister() override;

	virtual void OnPossessed_Implementation(AController* NewController) override;

	virtual void OnUnPossessed_Implementation(AController* PreviousController) override;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|CharacterInput")
	void OnSetupPlayerInputComponent(UInputComponent* Input);
};
