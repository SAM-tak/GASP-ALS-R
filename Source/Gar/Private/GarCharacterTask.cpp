#include "GarCharacterTask.h"

#include "GarCharacter.h"
#include "Engine/InputDelegateBinding.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacterTask)

void UGarCharacterTask::OnRegister()
{
	if (!Character.IsValid())
	{
		Character = Cast<AGarCharacter>(GetOuter());
		ensure(Character.IsValid());
	}
}

void UGarCharacterTask::Begin()
{
	ensure(Character.IsValid());
	if (!bActive)
	{
		bActive = true;
		bEpilogRunningCurrently = false;
		BindInput(Character->InputComponent.Get());
		K2_OnBegin();
	}
}

void UGarCharacterTask::Tick(float DeltaTime)
{
	if (bActive)
	{
		K2_OnTick(DeltaTime);
	}
}

void UGarCharacterTask::OnEnd(bool bWasCancelled)
{
	K2_OnEnd(bWasCancelled);
	if (Character.IsValid())
	{
		UnbindInput(Character->InputComponent.Get());
	}
}

void UGarCharacterTask::OnFinished()
{
	K2_OnFinished();
}

void UGarCharacterTask::End()
{
	if (bActive)
	{
		OnEnd(false);
		bEpilogRunningCurrently = IsEpilogRunning();
		bActive = false;
		if (!bEpilogRunningCurrently)
		{
			OnFinished();
		}
	}
	else if(bEpilogRunningCurrently)
	{
		bEpilogRunningCurrently = IsEpilogRunning();
		if (!bEpilogRunningCurrently)
		{
			OnFinished();
		}
	}
}

void UGarCharacterTask::Cancel()
{
	if (bActive)
	{
		OnEnd(true);
		bActive = false;
		bEpilogRunningCurrently = IsEpilogRunning();
		bActive = false;
		if (!bEpilogRunningCurrently)
		{
			OnFinished();
		}
	}
}

bool UGarCharacterTask::IsEpilogRunning_Implementation() const
{
	return false;
}

void UGarCharacterTask::OnPossessed(AController* NewController)
{
	if (IsValid(NewController) && IsValid(NewController->InputComponent))
	{
		BindInput(NewController->InputComponent);
	}
}

void UGarCharacterTask::OnUnPossessed(AController* PreviousController)
{
	if (IsValid(PreviousController) && IsValid(PreviousController->InputComponent))
	{
		UnbindInput(PreviousController->InputComponent);
	}
}

void UGarCharacterTask::BindInput(UInputComponent* InputComponent)
{
	if(bEnableInputBinding && !bInputBinded && IsValid(InputComponent))
	{
		UInputDelegateBinding::BindInputDelegates(GetClass(), InputComponent, this);
		bInputBinded = true;
	}
}

void UGarCharacterTask::UnbindInput(UInputComponent* InputComponent)
{
	if(bInputBinded && IsValid(InputComponent))
	{
		InputComponent->ClearBindingsForObject(this);
	}
	bInputBinded = false;
}

UWorld* UGarCharacterTask::GetWorld() const
{
	return Character.IsValid() ? Character->GetWorld() : nullptr;
}
