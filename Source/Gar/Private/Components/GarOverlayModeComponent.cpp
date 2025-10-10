#include "Components/GarOverlayModeComponent.h"

#include "AbilitySystemComponent.h"
#include "GarCharacter.h"
#include "LinkedAnimLayers/GarCharacterTaskAnimInstance.h"
#include "CharacterTasks/GarOverlayTask.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverlayModeComponent)

void UGarOverlayModeComponent::BeginPlay()
{
	Super::BeginPlay();

	InstancedOverlayTasks.Reset();
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
		CurrentOverlayTag = OverlayMode;
		CurrentOverlayTask->Begin();
	}
}

void UGarOverlayModeComponent::Multicast_ChangeOverlayTask_Implementation(const FGameplayTag& OverlayMode)
{
	if (!Character->IsLocallyControlled())
	{
		ChangeOverlayTask(OverlayMode);
	}
}

void UGarOverlayModeComponent::OnRefresh_Implementation(float DeltaTime)
{
	Super::OnRefresh_Implementation(DeltaTime);

	FGameplayTagContainer OverlayTagsMask;
	for (auto& KeyValue : OverlayClassMap)
	{
		OverlayTagsMask.AddTag(KeyValue.Key);
	}
	
	FGameplayTagContainer Container;
	Character->GetAbilitySystemComponent()->GetOwnedGameplayTags(Container);
	auto OverlayMode{Container.Filter(OverlayTagsMask).First()};

	if (OverlayMode.IsValid() && CurrentOverlayTag != OverlayMode)
	{
		ChangeOverlayTask(OverlayMode);
	}

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

void UGarOverlayModeComponent::RegisterOverlayTask(const FGameplayTag& OverlayMode, TSubclassOf<UGarOverlayTask> OverlayTaskClass)
{
	if (OverlayMode.IsValid() && OverlayTaskClass)
	{
		OverlayClassMap.Add(OverlayMode, OverlayTaskClass);
	}
}

void UGarOverlayModeComponent::UnregisterOverlayTask(const FGameplayTag& OverlayMode)
{
	if (OverlayMode.IsValid())
	{
		OverlayClassMap.Remove(OverlayMode);
	}
}
