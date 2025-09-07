#pragma once

#include "GameplayTagContainer.h"
#include "InputTriggers.h"
#include "GarAbilityInputAction.generated.h"

class UInputAction;
class UEnhancedInputComponent;
class UGarAbilitySystemComponent;

/**
 * FGarAbilityInputAction
 *
 *	Struct used to map a input action to a gameplay input tag.
 */
USTRUCT(BlueprintType)
struct GAR_API FGarAbilityInputAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (DisplayThumbnail = false))
	TObjectPtr<const UInputAction> InputAction{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "Trigger"))
	ETriggerEvent TriggerEvent{ETriggerEvent::Triggered};

	void BindAction(UGarAbilitySystemComponent* AbilitySystemComponent, UEnhancedInputComponent* InputComponent) const;
};
