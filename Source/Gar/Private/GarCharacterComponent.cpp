#include "GarCharacterComponent.h"

#include "Misc/UObjectToken.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GarCharacter.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacterComponent)

UGarCharacterComponent::UGarCharacterComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bTickInEditor = false;
}

void UGarCharacterComponent::OnRegister()
{
	Super::OnRegister();

	Character = GetPawn<AGarCharacter>();

	if (!Character.IsValid())
	{
		UE_LOG(LogGar, Error, TEXT("[UGarCharacterComponent::OnRegister] This component has been added to a blueprint whose base class is not a child of GarCharacter. To use this component, it MUST be placed on a child of GarCharacter Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("GarCharacterComponent", "NotOnGarCharacterError", "has been added to a blueprint whose base class is not a child of GarCharacter. To use this component, it MUST be placed on a child of GarCharacter Blueprint. This will cause a crash if you PIE!");
			static const FName MessageLogName = TEXT("GarCharacterComponent");
			
			FMessageLog(MessageLogName).Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));
			
			FMessageLog(MessageLogName).Open();
		}
#endif
	}
	else
	{
		Character->OnPossessed_Client.AddUObject(this, &ThisClass::OnPossessed);
		Character->OnUnPossessed_Client.AddUObject(this, &ThisClass::OnUnPossessed);
		Character->OnRefresh.AddUObject(this, &ThisClass::OnRefresh);
	}
}

void UGarCharacterComponent::OnPossessed_Implementation(AController* NewController) {}

void UGarCharacterComponent::OnUnPossessed_Implementation(AController* PreviousController) {}

void UGarCharacterComponent::OnRefresh_Implementation(float DeltaTime) {}
