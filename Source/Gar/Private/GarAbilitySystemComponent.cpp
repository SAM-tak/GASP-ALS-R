#include "GarAbilitySystemComponent.h"

#include "EnhancedInputComponent.h"
#include "Animation/AnimMontage.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "Abilities/GarGameplayAbility.h"
#include "Abilities/GarAbilitySet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAbilitySystemComponent)

UGarAbilitySystemComponent::UGarAbilitySystemComponent()
{
	SetIsReplicated(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UGarAbilitySystemComponent::OnRegister()
{
	Super::OnRegister();
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	if (IsValid(Character))
	{
		Character->OnRefresh.AddUObject(this, &ThisClass::OnRefresh);
		Character->OnPossessed_Client.AddUObject(this, &ThisClass::OnPossessed);
		Character->OnUnPossessed_Client.AddUObject(this, &ThisClass::OnUnPossessed);
	}
}

void UGarAbilitySystemComponent::Initialize(AGarCharacter* InOwnerCharacter)
{
	Super::InitAbilityActorInfo(InOwnerCharacter, InOwnerCharacter);
	if (InOwnerCharacter->HasAuthority() && IsValid(AbilitySet))
	{
		AbilitySet->GiveToAbilitySystem(this, InOwnerCharacter);
	}
}

void UGarAbilitySystemComponent::BindAbilityActivationInput(UEnhancedInputComponent* EnhancedInputComponent, const UInputAction* Action, ETriggerEvent TriggerEvent,
														    const FGameplayTag& InputTag)
{
	if (!ensure(IsValid(EnhancedInputComponent)) || !ensure(IsValid(Action))) return;
	auto Handle{EnhancedInputComponent->BindAction(Action, TriggerEvent, this, &ThisClass::ActivateOnInputAction, InputTag).GetHandle()};
	if (!BindingHandles.Contains(InputTag))
	{
		BindingHandles.Add(InputTag);
	}
	BindingHandles[InputTag].AddUnique(Handle);
}

void UGarAbilitySystemComponent::UnbindAbilityInputs(UEnhancedInputComponent* EnhancedInputComponent, const FGameplayTag& InputTag)
{
	if (!ensure(IsValid(EnhancedInputComponent))) return;
	if (BindingHandles.Contains(InputTag))
	{
		for(auto Handle : BindingHandles[InputTag])
		{
			EnhancedInputComponent->RemoveActionBindingForHandle(Handle);
		}
		BindingHandles.Remove(InputTag);
	}
}

void UGarAbilitySystemComponent::ActivateOnInputAction(FGameplayTag InputTag)
{
	TryActivateAbilitiesBySingleTag(InputTag);
}

void UGarAbilitySystemComponent::OnPossessed_Implementation(AController* NewController)
{
	RefreshAbilityActorInfo();

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.IsActive())
		{
			auto* GarAbility{Cast<UGarGameplayAbility>(Spec.Ability)};
			if (IsValid(GarAbility))
			{
				GarAbility->OnPossessed(NewController);
			}
		}
	}
}

void UGarAbilitySystemComponent::OnUnPossessed_Implementation(AController* PreviousController)
{
	RefreshAbilityActorInfo();

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.IsActive())
		{
			auto* GarAbility{Cast<UGarGameplayAbility>(Spec.Ability)};
			if (IsValid(GarAbility))
			{
				GarAbility->OnUnPossessed(PreviousController);
			}
		}
	}
}

void UGarAbilitySystemComponent::OnRefresh_Implementation(float DeltaTime)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};

	if (Character->GetLocomotionMode() == GarLocomotionModeTags::InAir && Character->IsLocallyControlled())
	{
		TryActivateAbilitiesBySingleTag(GarLocomotionActionTags::Traversal);
	}
}
