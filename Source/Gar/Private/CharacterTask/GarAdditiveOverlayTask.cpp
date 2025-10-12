// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarAdditiveOverlayTask.h"
#include "GarCharacter.h"
#include "LinkedAnimLayers/GarAdditiveOverlayAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAdditiveOverlayTask)

void UGarAdditiveOverlayTask::Begin()
{
	if (!bActive && IsValid(AdditiveOverlayAnimClass))
	{
		AdditiveOverlayAnimInstance = Cast<UGarAdditiveOverlayAnimInstance>(
			Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(AdditiveOverlayAnimClass)
		);

		if (!AdditiveOverlayAnimInstance.IsValid())
		{
			Character->GetMesh()->LinkAnimClassLayers(AdditiveOverlayAnimClass);
			AdditiveOverlayAnimInstance = Cast<UGarAdditiveOverlayAnimInstance>(
				Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(AdditiveOverlayAnimClass)
			);
		}
	}
	Super::Begin();
}

void UGarAdditiveOverlayTask::OnFinished()
{
	if (Character.IsValid() && IsValid(AdditiveOverlayAnimClass))
	{
		Character->GetMesh()->UnlinkAnimClassLayers(AdditiveOverlayAnimClass);
		AdditiveOverlayAnimInstance.Reset();
	}

	Super::OnFinished();
}
