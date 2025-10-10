#include "Components/GarOverrideModeComponent.h"

#include "AbilitySystemComponent.h"
#include "GarCharacter.h"
#include "GarLinkedAnimationInstance.h"
#include "CharacterTasks/GarOverrideTask.h"
#include "CharacterTasks/GarRagdollingTask.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverrideModeComponent)

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

void UGarOverrideModeComponent::ChangeOverrideTaskIfNeeded(const FGameplayTag& OverrideMode)
{
	if (CurrentOverrideTask.IsValid())
	{
		if (CurrentOverrideTag == OverrideMode)
		{
			return;
		}
		else
		{
			CurrentOverrideTask->End();
			if (CurrentOverrideTask->HasFinished())
			{
				CurrentOverrideTask.Reset();
				CurrentOverrideTag = FGameplayTag::EmptyTag;
			}
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

void UGarOverrideModeComponent::OnRefresh_Implementation(float DeltaTime)
{
	Super::OnRefresh_Implementation(DeltaTime);

	FGameplayTagContainer OverrideTagsMask;
	for (auto& KeyValue : OverrideClassMap)
	{
		OverrideTagsMask.AddTag(KeyValue.Key);
	}

	FGameplayTagContainer Container;
	Character->GetAbilitySystemComponent()->GetOwnedGameplayTags(Container);
	auto OverrideMode{Container.Filter(OverrideTagsMask).First()};

	ChangeOverrideTaskIfNeeded(OverrideMode);

	if (CurrentOverrideTask.IsValid())
	{
		CurrentOverrideTask->Refresh(DeltaTime);
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
