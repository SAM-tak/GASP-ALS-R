#include "Abilities/GarAbilityInputAction.h"
#include "GarAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAbilityInputAction)

void FGarAbilityInputAction::BindAction(UGarAbilitySystemComponent* AbilitySystemComponent, UEnhancedInputComponent* InputComponent) const
{
	AbilitySystemComponent->BindAbilityActivationInput(InputComponent, InputAction, TriggerEvent, InputTag);
}
