#include "Components/GarLocalMontageComponent.h"

#include "GarCharacter.h"
#include "GarMotionWarpingComponent.h"
#include "CharacterTasks/GarLocalMontageTask.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarLocalMontageComponent)

void UGarLocalMontageComponent::BeginPlay()
{
	Super::BeginPlay();

	InstancedLocalMontageTasks.Reset();
	LocalMontageTagsMask.Reset();
	for(auto& KeyValue : LocalMontageTaskClassMap)
	{
		LocalMontageTagsMask.AddTag(KeyValue.Key);
	}
}

UGarLocalMontageTask* UGarLocalMontageComponent::Play(const FGameplayTag& LocalMontageTag)
{
	ensure(Character->GetLocalRole() > ROLE_SimulatedProxy);
	
	if (Character->HasServerRole())
	{
		MulticastPlay(LocalMontageTag);
	}

	return PlayImplementation(LocalMontageTag);
}

void UGarLocalMontageComponent::MulticastPlay_Implementation(const FGameplayTag& LocalMontageTag)
{
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		PlayImplementation(LocalMontageTag);
	}
}

UGarLocalMontageTask* UGarLocalMontageComponent::PlayImplementation(const FGameplayTag& LocalMontageTag)
{
	if (CurrentLocalMontageTask.IsValid())
	{
		CurrentLocalMontageTask->Cancel();
		CurrentLocalMontageTask.Reset();
		CurrentLocalMontageTag = FGameplayTag::EmptyTag;
	}

	if (LocalMontageTaskClassMap.Contains(LocalMontageTag))
	{
		if (InstancedLocalMontageTasks.Contains(LocalMontageTag))
		{
			CurrentLocalMontageTask = InstancedLocalMontageTasks[LocalMontageTag];
		}
		else
		{
			auto* NewTask{NewObject<UGarLocalMontageTask>(Character.Get(), LocalMontageTaskClassMap[LocalMontageTag])};
			NewTask->Component = this;
			InstancedLocalMontageTasks.Add(LocalMontageTag, NewTask);
			CurrentLocalMontageTask = NewTask;
			CurrentLocalMontageTask->OnRegister();
		}

		if (CurrentLocalMontageTask.IsValid())
		{
			CurrentLocalMontageTag = LocalMontageTag;
			CurrentLocalMontageTask->Begin();
		}
		else
		{
			UE_LOG(LogGar, Error, TEXT("UGarLocalMontageComponent : Correspond Local Montage Task was not found for '%s'"), *LocalMontageTag.ToString());
		}
	}

	return CurrentLocalMontageTask.Get();
}

void UGarLocalMontageComponent::OnEndTask(UGarLocalMontageTask* Task)
{
	if (CurrentLocalMontageTask == Task)
	{
		CurrentLocalMontageTask.Reset();
		CurrentLocalMontageTag = FGameplayTag::EmptyTag;
	}
}

void UGarLocalMontageComponent::OnRefresh_Implementation(float DeltaTime)
{
	Super::OnRefresh_Implementation(DeltaTime);

	if (CurrentLocalMontageTask.IsValid())
	{
		CurrentLocalMontageTask->Refresh(DeltaTime);
	}
}

void UGarLocalMontageComponent::OnPossessed_Implementation(AController* NewController)
{
	Super::OnPossessed_Implementation(NewController);
	if (CurrentLocalMontageTask.IsValid())
	{
		CurrentLocalMontageTask->OnPossessed(NewController);
	}
}

void UGarLocalMontageComponent::OnUnPossessed_Implementation(AController* PreviousController)
{
	Super::OnUnPossessed_Implementation(PreviousController);
	if (CurrentLocalMontageTask.IsValid())
	{
		CurrentLocalMontageTask->OnUnPossessed(PreviousController);
	}
}

void UGarLocalMontageComponent::AddOrUpdateReplicatedWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector TargetLocation,
																									  FRotator TargetRotation)
{
	ensure(Character->GetLocalRole() > ROLE_SimulatedProxy);

	Character->GetMotionWarping()->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);

	if (Character->HasServerRole())
	{
		MulticastAddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
	}
}

void UGarLocalMontageComponent::MulticastAddOrUpdateWarpTargetFromLocationAndRotation_Implementation(FName WarpTargetName, FVector_NetQuantize TargetLocation,
																									 FRotator TargetRotation)
{
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		Character->GetMotionWarping()->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
	}
}
