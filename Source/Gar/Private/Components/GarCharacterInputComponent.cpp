#include "Components/GarCharacterInputComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GarCharacter.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacterMoverComponent.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacterInputComponent)

void UGarCharacterInputComponent::OnRegister()
{
	Super::OnRegister();

	if (Character.IsValid())
	{
		Character->OnSetupPlayerInputComponent.AddUObject(this, &ThisClass::OnSetupPlayerInputComponent);
	}
}

void UGarCharacterInputComponent::OnPossessed_Implementation(AController* NewController)
{
	Super::OnPossessed_Implementation(NewController);

	auto* NewPlayerController{Cast<APlayerController>(NewController)};
	if (IsValid(NewPlayerController))
	{
		NewPlayerController->InputYawScale_DEPRECATED = 1.0f;
		NewPlayerController->InputPitchScale_DEPRECATED = 1.0f;
		NewPlayerController->InputRollScale_DEPRECATED = 1.0f;

		auto* InputSubsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayerController->GetLocalPlayer())};
		if (IsValid(InputSubsystem))
		{
			FModifyContextOptions Options;
			Options.bNotifyUserSettings = true;

			InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
		}
	}
}

void UGarCharacterInputComponent::OnUnPossessed_Implementation(AController* PreviousController)
{
	Super::OnUnPossessed_Implementation(PreviousController);

	auto* PreviousPlayerController{Cast<APlayerController>(PreviousController)};
	if (IsValid(PreviousPlayerController))
	{
		auto* InputSubsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayerController->GetLocalPlayer())};
		if (IsValid(InputSubsystem))
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
		}
	}
}

void UGarCharacterInputComponent::OnSetupPlayerInputComponent_Implementation(UInputComponent* Input)
{
	auto* EnhancedInput{Cast<UEnhancedInputComponent>(Input)};
	if (IsValid(EnhancedInput))
	{
		auto* GarAbilitySystem{Character->GetGarAbilitySystem()};
		for(auto& AbilityInputAction : AbilityInputActions)
		{
			AbilityInputAction.BindAction(GarAbilitySystem, EnhancedInput);
		}
	}
}
