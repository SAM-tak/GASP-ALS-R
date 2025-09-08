// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarOverlayTask.h"
#include "GarCharacter.h"
#include "LinkedAnimLayers/GarCharacterTaskAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverlayTask)

void UGarOverlayTask::Begin()
{
	if (!bActive && IsValid(OverlayAnimClass))
	{
		OverlayAnimInstance = Cast<UGarCharacterTaskAnimInstance>(Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(OverlayAnimClass));

		if (!OverlayAnimInstance.IsValid())
		{
			Character->GetMesh()->LinkAnimClassLayers(OverlayAnimClass);
			OverlayAnimInstance = Cast<UGarCharacterTaskAnimInstance>(Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(OverlayAnimClass));
		}

		OverlayAnimInstance->Refresh(this);
	}
	Super::Begin();
}

void UGarOverlayTask::Refresh(float DeltaTime)
{
	Super::Refresh(DeltaTime);
	if (OverlayAnimInstance.IsValid())
	{
		OverlayAnimInstance->Refresh(this);
	}
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
