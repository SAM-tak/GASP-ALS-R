#include "Components/GarDeltaOverlayModeComponent.h"

#include "AbilitySystemComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacter.h"
#include "CharacterTasks/GarDeltaOverlayTask.h"
#include "Abilities/GarGameplayAbility_DeltaOverlay.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarDeltaOverlayModeComponent)

void UGarDeltaOverlayModeComponent::OnRegister()
{
	Super::OnRegister();

	if (Character.IsValid() && Character->GetAbilitySystemComponent() && Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		// No effect
		Character->GetGarAbilitySystem()->OnRepActivateAbilities.AddUObject(this, &ThisClass::CheckActiveAbility);
	}
}

void UGarDeltaOverlayModeComponent::BeginPlay()
{
	Super::BeginPlay();

	InstancedDeltaOverlayTasks.Reset();
}

void UGarDeltaOverlayModeComponent::ChangeDeltaOverlayTask(const FGameplayTag& NewDeltaOverlayMode)
{
	if (CurrentDeltaOverlayTask.IsValid())
	{
		CurrentDeltaOverlayTask->End();
		if (CurrentDeltaOverlayTask->HasFinished())
		{
			CurrentDeltaOverlayTask.Reset();
		}
		else
		{
			return;
		}
	}

	if (!CurrentDeltaOverlayTask.IsValid() && DeltaOverlayClassMap.Contains(NewDeltaOverlayMode))
	{
		if (InstancedDeltaOverlayTasks.Contains(NewDeltaOverlayMode))
		{
			CurrentDeltaOverlayTask = InstancedDeltaOverlayTasks[NewDeltaOverlayMode];
		}
		else
		{
			auto* NewTask{NewObject<UGarDeltaOverlayTask>(Character.Get(), DeltaOverlayClassMap[NewDeltaOverlayMode])};
			NewTask->Component = this;
			InstancedDeltaOverlayTasks.Add(NewDeltaOverlayMode, NewTask);
			CurrentDeltaOverlayTask = NewTask;
			NewTask->OnRegister();
		}
		CurrentDeltaOverlayTag = NewDeltaOverlayMode;
		CurrentDeltaOverlayTask->Begin();
	}
}

void UGarDeltaOverlayModeComponent::CheckActiveAbility(UGarAbilitySystemComponent* AbilitySystem)
{
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		for (auto& Ability : AbilitySystem->GetActivatableAbilities())
		{
			auto DeltaOverlayModeAbility{Cast<UGarGameplayAbility_DeltaOverlay>(Ability.Ability)};
			if (DeltaOverlayModeAbility)
			{
				RegisterDeltaOverlayTask(DeltaOverlayModeAbility->GetAssetTags().First(), DeltaOverlayModeAbility->DeltaOverlayTaskClass);
			}
		}
	}
}

void UGarDeltaOverlayModeComponent::OnOwnerTick_Implementation(float DeltaTime)
{
	Super::OnOwnerTick_Implementation(DeltaTime);
	
	FGameplayTagContainer DeltaOverlayTagsMask;
	for (auto& KeyValue : DeltaOverlayClassMap)
	{
		DeltaOverlayTagsMask.AddTag(KeyValue.Key);
	}

	FGameplayTagContainer Container;
	Character->GetAbilitySystemComponent()->GetOwnedGameplayTags(Container);
	auto DeltaOverlayMode{Container.Filter(DeltaOverlayTagsMask).First()};

	if (DeltaOverlayMode.IsValid() && CurrentDeltaOverlayTag != DeltaOverlayMode)
	{
		ChangeDeltaOverlayTask(DeltaOverlayMode);
	}

	if (CurrentDeltaOverlayTask.IsValid())
	{
		CurrentDeltaOverlayTask->Tick(DeltaTime);
	}
}

void UGarDeltaOverlayModeComponent::OnPossessed_Implementation(AController* NewController)
{
	Super::OnPossessed_Implementation(NewController);
	if (CurrentDeltaOverlayTask.IsValid())
	{
		CurrentDeltaOverlayTask->OnPossessed(NewController);
	}
}

void UGarDeltaOverlayModeComponent::OnUnPossessed_Implementation(AController* PreviousController)
{
	Super::OnUnPossessed_Implementation(PreviousController);
	if (CurrentDeltaOverlayTask.IsValid())
	{
		CurrentDeltaOverlayTask->OnUnPossessed(PreviousController);
	}
}

void UGarDeltaOverlayModeComponent::RegisterDeltaOverlayTask(const FGameplayTag& DeltaOverlayMode, TSubclassOf<UGarDeltaOverlayTask> DeltaOverlayTaskClass)
{
	if (DeltaOverlayMode.IsValid() && DeltaOverlayTaskClass)
	{
		DeltaOverlayClassMap.Add(DeltaOverlayMode, DeltaOverlayTaskClass);
	}
}

void UGarDeltaOverlayModeComponent::UnregisterDeltaOverlayTask(const FGameplayTag& DeltaOverlayMode)
{
	if (DeltaOverlayMode.IsValid())
	{
		DeltaOverlayClassMap.Remove(DeltaOverlayMode);
	}
}
