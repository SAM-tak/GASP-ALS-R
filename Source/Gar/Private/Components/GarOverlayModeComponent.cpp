#include "Components/GarOverlayModeComponent.h"

#include "GarCharacter.h"
#include "LinkedAnimLayers/GarCharacterTaskAnimInstance.h"
#include "CharacterTasks/GarOverlayTask.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverlayModeComponent)

void UGarOverlayModeComponent::OnRegister()
{
	Super::OnRegister();

	if (Character.IsValid())
	{
		Character->OnOverlayModeChanged.AddUObject(this, &ThisClass::OnChangeOverlayMode);
	}
}

void UGarOverlayModeComponent::BeginPlay()
{
	Super::BeginPlay();

	InstancedOverlayTasks.Reset();

	if (Character.IsValid())
	{
		ChangeOverlayTask(Character->GetOverlayMode());
	}
}

void UGarOverlayModeComponent::ChangeOverlayTask(const FGameplayTag& OverlayMode)
{
	if (CurrentOverlayTask.IsValid())
	{
		CurrentOverlayTask->End();
		if (CurrentOverlayTask->HasFinished())
		{
			CurrentOverlayTask.Reset();
		}
	}

	if (!CurrentOverlayTask.IsValid() && OverlayClassMap.Contains(OverlayMode))
	{
		if (InstancedOverlayTasks.Contains(OverlayMode))
		{
			CurrentOverlayTask = InstancedOverlayTasks[OverlayMode];
		}
		else
		{
			auto* NewTask{NewObject<UGarOverlayTask>(Character.Get(), OverlayClassMap[OverlayMode])};
			NewTask->Component = this;
			InstancedOverlayTasks.Add(OverlayMode, NewTask);
			CurrentOverlayTask = NewTask;
			NewTask->OnRegister();
		}
		CurrentOverlayTask->Begin();
	}
}

void UGarOverlayModeComponent::OnRefresh_Implementation(float DeltaTime)
{
	Super::OnRefresh_Implementation(DeltaTime);
	if (CurrentOverlayTask.IsValid())
	{
		CurrentOverlayTask->Refresh(DeltaTime);
	}
}

void UGarOverlayModeComponent::OnPossessed_Implementation(AController* NewController)
{
	Super::OnPossessed_Implementation(NewController);
	if (CurrentOverlayTask.IsValid())
	{
		CurrentOverlayTask->OnPossessed(NewController);
	}
}

void UGarOverlayModeComponent::OnUnPossessed_Implementation(AController* PreviousController)
{
	Super::OnUnPossessed_Implementation(PreviousController);
	if (CurrentOverlayTask.IsValid())
	{
		CurrentOverlayTask->OnUnPossessed(PreviousController);
	}
}

void UGarOverlayModeComponent::OnChangeOverlayMode_Implementation(const FGameplayTag& PreviousOverlayMode)
{
	if (Character.IsValid())
	{
		ChangeOverlayTask(Character->GetOverlayMode());
	}
}
