// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterTasks/GarOverrideTask.h"
#include "GarCharacter.h"
#include "GarCharacterTaskAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarOverrideTask)

void UGarOverrideTask::Begin()
{
	if (!bActive && IsValid(OverrideAnimClass))
	{
		OverrideAnimInstance = Cast<UGarCharacterTaskAnimInstance>(Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(OverrideAnimClass));

		if (!OverrideAnimInstance.IsValid())
		{
			Character->GetMesh()->LinkAnimClassLayers(OverrideAnimClass);
			OverrideAnimInstance = Cast<UGarCharacterTaskAnimInstance>(Character->GetMesh()->GetLinkedAnimLayerInstanceByClass(OverrideAnimClass));
		}

		OverrideAnimInstance->Refresh(this);
	}
	Super::Begin();
}

void UGarOverrideTask::Refresh(float DeltaTime)
{
	Super::Refresh(DeltaTime);
	if (OverrideAnimInstance.IsValid())
	{
		OverrideAnimInstance->Refresh(this);
	}
}

void UGarOverrideTask::OnFinished()
{
	if (Character.IsValid() && IsValid(OverrideAnimClass))
	{
		Character->GetMesh()->UnlinkAnimClassLayers(OverrideAnimClass);
		OverrideAnimInstance.Reset();
	}

	Super::OnFinished();
}
