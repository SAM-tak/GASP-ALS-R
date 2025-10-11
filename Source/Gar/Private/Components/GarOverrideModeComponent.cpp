#include "Components/GarOverrideModeComponent.h"

#include "AbilitySystemComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacter.h"
#include "GarLinkedAnimationInstance.h"
#include "CharacterTasks/GarOverrideTask.h"
#include "CharacterTasks/GarRagdollingTask.h"
#include "Abilities/Actions/GarGameplayAbility_Ragdolling.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverrideModeComponent)

void UGarOverrideModeComponent::OnRegister()
{
	Super::OnRegister();

	if (Character.IsValid() && Character->GetAbilitySystemComponent() && Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		// no effect
		Character->GetGarAbilitySystem()->OnRepActivateAbilities.AddUObject(this, &ThisClass::CheckActiveAbility);
	}
}

void UGarOverrideModeComponent::BeginPlay()
{
	Super::BeginPlay();

	InstancedOverrideTasks.Reset();
}

void UGarOverrideModeComponent::EndCurrentRagdollingTask()
{
	if (CurrentOverrideTask.IsValid() && CurrentOverrideTask->IsA(UGarRagdollingTask::StaticClass()))
	{
		CurrentOverrideTask->End();
		if (CurrentOverrideTask->HasFinished())
		{
			CurrentOverrideTask.Reset();
			CurrentOverrideTag = FGameplayTag::EmptyTag;
		}
	}
}

void UGarOverrideModeComponent::ChangeOverrideTask(const FGameplayTag& OverrideMode)
{
	if (CurrentOverrideTask.IsValid())
	{
		CurrentOverrideTask->End();
		if (CurrentOverrideTask->HasFinished())
		{
			CurrentOverrideTask.Reset();
			CurrentOverrideTag = FGameplayTag::EmptyTag;
		}
		else
		{
			return;
		}
	}

	if (!CurrentOverrideTask.IsValid() && OverrideClassMap.Contains(OverrideMode))
	{
		if (InstancedOverrideTasks.Contains(OverrideMode))
		{
			CurrentOverrideTask = InstancedOverrideTasks[OverrideMode];
		}
		else
		{
			auto* NewTask{NewObject<UGarOverrideTask>(Character.Get(), OverrideClassMap[OverrideMode])};
			NewTask->Component = this;
			InstancedOverrideTasks.Add(OverrideMode, NewTask);
			CurrentOverrideTask = NewTask;
			CurrentOverrideTask->OnRegister();
		}
		CurrentOverrideTag = OverrideMode;
		CurrentOverrideTask->Begin();
	}
}

void UGarOverrideModeComponent::CheckActiveAbility(UGarAbilitySystemComponent* AbilitySystem)
{
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		for (auto& Ability : AbilitySystem->GetActivatableAbilities())
		{
			auto RagdollingAbility{Cast<UGarGameplayAbility_Ragdolling>(Ability.Ability)};
			if (RagdollingAbility)
			{
				RegisterOverrideTask(RagdollingAbility->GetAssetTags().First(), RagdollingAbility->OverrideTaskClass);
			}
		}
	}
}

void UGarOverrideModeComponent::OnOwnerTick_Implementation(float DeltaTime)
{
	Super::OnOwnerTick_Implementation(DeltaTime);

	FGameplayTagContainer OverrideTagsMask;
	for (auto& KeyValue : OverrideClassMap)
	{
		OverrideTagsMask.AddTag(KeyValue.Key);
	}

	FGameplayTagContainer Container;
	Character->GetAbilitySystemComponent()->GetOwnedGameplayTags(Container);
	auto OverrideMode{Container.Filter(OverrideTagsMask).First()};

	if (CurrentOverrideTag != OverrideMode)
	{
		ChangeOverrideTask(OverrideMode);
	}

	if (CurrentOverrideTask.IsValid())
	{
		CurrentOverrideTask->Tick(DeltaTime);
	}
}

void UGarOverrideModeComponent::OnPossessed_Implementation(AController* NewController)
{
	Super::OnPossessed_Implementation(NewController);
	if (CurrentOverrideTask.IsValid())
	{
		CurrentOverrideTask->OnPossessed(NewController);
	}
}

void UGarOverrideModeComponent::OnUnPossessed_Implementation(AController* PreviousController)
{
	Super::OnUnPossessed_Implementation(PreviousController);
	if (CurrentOverrideTask.IsValid())
	{
		CurrentOverrideTask->OnUnPossessed(PreviousController);
	}
}

void UGarOverrideModeComponent::RegisterOverrideTask(const FGameplayTag& OverrideMode, TSubclassOf<UGarOverrideTask> OverrideTaskClass)
{
	if (OverrideTaskClass)
	{
		OverrideClassMap.Add(OverrideMode, OverrideTaskClass);
	}
}

void UGarOverrideModeComponent::UnregisterOverrideTask(const FGameplayTag& OverrideMode)
{
	if (OverrideMode.IsValid())
	{
		OverrideClassMap.Remove(OverrideMode);
	}
}
