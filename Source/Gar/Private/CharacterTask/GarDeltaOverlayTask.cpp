// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarDeltaOverlayTask.h"
#include "GarCharacter.h"
#include "LinkedAnimLayers/GarDeltaOverlayAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarDeltaOverlayTask)

void UGarDeltaOverlayTask::Begin()
{
	if (!bActive && IsValid(DeltaOverlayAnimClass))
	{
		DeltaOverlayAnimInstance = Cast<UGarDeltaOverlayAnimInstance>(Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(DeltaOverlayAnimClass));

		if (!DeltaOverlayAnimInstance.IsValid())
		{
			Character->GetMesh()->LinkAnimClassLayers(DeltaOverlayAnimClass);
			DeltaOverlayAnimInstance = Cast<UGarDeltaOverlayAnimInstance>(Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(DeltaOverlayAnimClass));
		}
	}
	Super::Begin();
}

void UGarDeltaOverlayTask::OnFinished()
{
	if (Character.IsValid() && IsValid(DeltaOverlayAnimClass))
	{
		Character->GetMesh()->UnlinkAnimClassLayers(DeltaOverlayAnimClass);
		DeltaOverlayAnimInstance.Reset();
	}

	Super::OnFinished();
}
