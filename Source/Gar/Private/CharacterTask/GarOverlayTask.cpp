// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarOverlayTask.h"
#include "GarCharacter.h"
#include "Components/GarOverlayModeComponent.h"
#include "LinkedAnimLayers/GarOverlayAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverlayTask)

UGarOverlayTask* UGarOverlayTask::New(UObject* Outer, TSubclassOf<UGarOverlayTask> Class, UGarOverlayModeComponent* InComponent)
{
	auto* Task = NewObject<UGarOverlayTask>(Outer, Class);
	Task->Component = InComponent;
	Task->OnRegister();
	return Task;
}

void UGarOverlayTask::Begin()
{
	if (!bActive && IsValid(OverlayAnimClass))
	{
		OverlayAnimInstance = Cast<UGarOverlayAnimInstance>(Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(OverlayAnimClass));

		if (!OverlayAnimInstance.IsValid())
		{
			Character->GetMesh()->LinkAnimClassLayers(OverlayAnimClass);
			OverlayAnimInstance = Cast<UGarOverlayAnimInstance>(Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(OverlayAnimClass));
		}
	}
	Super::Begin();
}

void UGarOverlayTask::OnFinished()
{
	if (Character.IsValid() && IsValid(OverlayAnimClass))
	{
		Character->GetMesh()->UnlinkAnimClassLayers(OverlayAnimClass);
		OverlayAnimInstance.Reset();
	}

	Super::OnFinished();
}
