// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarOverlayTask.h"
#include "GarCharacter.h"
#include "LinkedAnimLayers/GarOverlayAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverlayTask)

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
