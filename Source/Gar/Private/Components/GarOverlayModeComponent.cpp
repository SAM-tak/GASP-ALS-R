#include "Components/GarOverlayModeComponent.h"

#include "AbilitySystemComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacter.h"
#include "CharacterTasks/GarOverlayTask.h"
#include "CharacterTasks/GarDeltaOverlayTask.h"
#include "Abilities/GarGameplayAbility_OverlayMode.h"
#include "Abilities/GarGameplayAbility_DeltaOverlay.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverlayModeComponent)

void UGarOverlayModeComponent::OnRegister()
{
	Super::OnRegister();

	if (Character.IsValid() && Character->GetAbilitySystemComponent() && Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		// No effect
		Character->GetGarAbilitySystem()->OnRepActivateAbilities.AddUObject(this, &ThisClass::CheckActiveAbility);
	}
}

void UGarOverlayModeComponent::BeginPlay()
{
	Super::BeginPlay();

	InstancedOverlayTasks.Reset();
	InstancedDeltaOverlayTasks.Reset();
}

void UGarOverlayModeComponent::ChangeOverlayTask(const FGameplayTag& NewOverlayMode)
{
	if (CurrentOverlayTask.IsValid())
	{
		CurrentOverlayTask->End();
		if (CurrentOverlayTask->HasFinished())
		{
			CurrentOverlayTask.Reset();
		}
		else
		{
			return;
		}
	}

	if (!CurrentOverlayTask.IsValid() && OverlayClassMap.Contains(NewOverlayMode))
	{
		if (InstancedOverlayTasks.Contains(NewOverlayMode))
		{
			CurrentOverlayTask = InstancedOverlayTasks[NewOverlayMode];
		}
		else
		{
			auto* NewTask{NewObject<UGarOverlayTask>(Character.Get(), OverlayClassMap[NewOverlayMode])};
			NewTask->Component = this;
			InstancedOverlayTasks.Add(NewOverlayMode, NewTask);
			CurrentOverlayTask = NewTask;
			NewTask->OnRegister();
		}
		CurrentOverlayTag = NewOverlayMode;
		CurrentOverlayTask->Begin();
	}
}

void UGarOverlayModeComponent::ChangeDeltaOverlayTask(const FGameplayTag& NewDeltaOverlayMode)
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

void UGarOverlayModeComponent::CheckActiveAbility(UGarAbilitySystemComponent* AbilitySystem)
{
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		for (auto& Ability : AbilitySystem->GetActivatableAbilities())
		{
			auto OverlayModeAbility{Cast<UGarGameplayAbility_OverlayMode>(Ability.Ability)};
			if (OverlayModeAbility)
			{
				RegisterOverlayTask(OverlayModeAbility->GetAssetTags().First(), OverlayModeAbility->OverlayTaskClass);
			}
			auto DeltaOverlayModeAbility{Cast<UGarGameplayAbility_DeltaOverlay>(Ability.Ability)};
			if (DeltaOverlayModeAbility)
			{
				RegisterDeltaOverlayTask(DeltaOverlayModeAbility->GetAssetTags().First(), DeltaOverlayModeAbility->DeltaOverlayTaskClass);
			}
		}
	}
}

void UGarOverlayModeComponent::OnOwnerTick_Implementation(float DeltaTime)
{
	Super::OnOwnerTick_Implementation(DeltaTime);

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
		CurrentOverlayTask->Tick(DeltaTime);
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

void UGarOverlayModeComponent::RegisterDeltaOverlayTask(const FGameplayTag& DeltaOverlayMode, TSubclassOf<UGarDeltaOverlayTask> DeltaOverlayTaskClass)
{
	if (DeltaOverlayMode.IsValid() && DeltaOverlayTaskClass)
	{
		DeltaOverlayClassMap.Add(DeltaOverlayMode, DeltaOverlayTaskClass);
	}
}

void UGarOverlayModeComponent::UnregisterDeltaOverlayTask(const FGameplayTag& DeltaOverlayMode)
{
	if (DeltaOverlayMode.IsValid())
	{
		DeltaOverlayClassMap.Remove(DeltaOverlayMode);
	}
}
