#include "Components/GarOverlayModeComponent.h"

#include "AbilitySystemComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacter.h"
#include "CharacterTasks/GarOverlayTask.h"
#include "CharacterTasks/GarAdditiveOverlayTask.h"
#include "Abilities/GarGameplayAbility_OverlayMode.h"
#include "Abilities/GarGameplayAbility_AdditiveOverlay.h"
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
	InstancedAdditiveOverlayTasks.Reset();
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

void UGarOverlayModeComponent::ChangeAdditiveOverlayTask(const FGameplayTag& NewAdditiveOverlayMode)
{
	if (CurrentAdditiveOverlayTask.IsValid())
	{
		CurrentAdditiveOverlayTask->End();
		if (CurrentAdditiveOverlayTask->HasFinished())
		{
			CurrentAdditiveOverlayTask.Reset();
		}
		else
		{
			return;
		}
	}

	if (!CurrentAdditiveOverlayTask.IsValid() && AdditiveOverlayClassMap.Contains(NewAdditiveOverlayMode))
	{
		if (InstancedAdditiveOverlayTasks.Contains(NewAdditiveOverlayMode))
		{
			CurrentAdditiveOverlayTask = InstancedAdditiveOverlayTasks[NewAdditiveOverlayMode];
		}
		else
		{
			auto* NewTask{NewObject<UGarAdditiveOverlayTask>(Character.Get(), AdditiveOverlayClassMap[NewAdditiveOverlayMode])};
			NewTask->Component = this;
			InstancedAdditiveOverlayTasks.Add(NewAdditiveOverlayMode, NewTask);
			CurrentAdditiveOverlayTask = NewTask;
			NewTask->OnRegister();
		}
		CurrentAdditiveOverlayTag = NewAdditiveOverlayMode;
		CurrentAdditiveOverlayTask->Begin();
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
			auto AdditiveOverlayModeAbility{Cast<UGarGameplayAbility_AdditiveOverlay>(Ability.Ability)};
			if (AdditiveOverlayModeAbility)
			{
				RegisterAdditiveOverlayTask(AdditiveOverlayModeAbility->GetAssetTags().First(), AdditiveOverlayModeAbility->AdditiveOverlayTaskClass);
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

void UGarOverlayModeComponent::RegisterAdditiveOverlayTask(const FGameplayTag& OverlayMode, TSubclassOf<UGarAdditiveOverlayTask> AdditiveOverlayTaskClass)
{
	if (OverlayMode.IsValid() && AdditiveOverlayTaskClass)
	{
		AdditiveOverlayClassMap.Add(OverlayMode, AdditiveOverlayTaskClass);
	}
}

void UGarOverlayModeComponent::UnregisterAdditiveOverlayTask(const FGameplayTag& OverlayMode)
{
	if (OverlayMode.IsValid())
	{
		AdditiveOverlayClassMap.Remove(OverlayMode);
	}
}
