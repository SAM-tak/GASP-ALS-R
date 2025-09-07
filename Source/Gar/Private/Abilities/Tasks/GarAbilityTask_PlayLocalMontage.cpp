#include "Abilities/Tasks/GarAbilityTask_PlayLocalMontage.h"
#include "GarCharacter.h"
#include "Components/GarLocalMontageComponent.h"
#include "CharacterTasks/GarLocalMontageTask.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAbilityTask_PlayLocalMontage)

UGarAbilityTask_PlayLocalMontage::UGarAbilityTask_PlayLocalMontage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UGarAbilityTask_PlayLocalMontage* UGarAbilityTask_PlayLocalMontage::CreatePlayLocalMontage(UGameplayAbility* OwningAbility, FName TaskInstanceName,
																						   FGameplayTag _LocalMontageTag)
{
	UGarAbilityTask_PlayLocalMontage* MyObj = NewAbilityTask<UGarAbilityTask_PlayLocalMontage>(OwningAbility, TaskInstanceName);
	MyObj->LocalMontageTag = _LocalMontageTag;
	return MyObj;
}

void UGarAbilityTask_PlayLocalMontage::Activate()
{
	Super::Activate();
	const auto* ActorInfo = Ability->GetCurrentActorInfo();
	auto* Character{Cast<AGarCharacter>(ActorInfo->OwnerActor)};
	ensure(Character);
	auto* Component{Character->FindComponentByClass<UGarLocalMontageComponent>()};
	ensure(Component);

	LocalMontageTask = Component->Play(LocalMontageTag);
	if (LocalMontageTask.IsValid())
	{
		LocalMontageTask->CurrentAbilityTask = this;
	}
}

void UGarAbilityTask_PlayLocalMontage::OnDestroy(bool AbilityEnded)
{
	if (LocalMontageTask.IsValid())
	{
		LocalMontageTask->End();
		LocalMontageTask->CurrentAbilityTask.Reset();
	}

	Super::OnDestroy(AbilityEnded);
}
