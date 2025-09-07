#include "GarMotionWarpingComponent.h"

#include "GarCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarMotionWarpingComponent)

void UGarMotionWarpingComponent::AddOrUpdateReplicatedWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector TargetLocation, FRotator TargetRotation)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	ensure(Character->GetLocalRole() > ROLE_SimulatedProxy);

	AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);

	if (Character->HasServerRole())
	{
		MulticastAddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
	}
}

void UGarMotionWarpingComponent::AutonomousAddOrUpdateReplicatedWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector TargetLocation, FRotator TargetRotation)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	ensure(Character->GetLocalRole() > ROLE_SimulatedProxy);

	AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);

	if (Character->IsCharacterSelf())
	{
		ServerAddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
	}
}

void UGarMotionWarpingComponent::ServerAddOrUpdateWarpTargetFromLocationAndRotation_Implementation(FName WarpTargetName, FVector_NetQuantize TargetLocation,
																								   FRotator TargetRotation)
{
	AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);

	MulticastAddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
}

void UGarMotionWarpingComponent::MulticastAddOrUpdateWarpTargetFromLocationAndRotation_Implementation(FName WarpTargetName, FVector_NetQuantize TargetLocation,
																									  FRotator TargetRotation)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
	}
}

void UGarMotionWarpingComponent::AddOrUpdateReplicatedWarpTargetFromComponent(FName WarpTargetName, const USceneComponent* Component, FName BoneName,
																			  bool bFollowComponent)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	ensure(Character->GetLocalRole() > ROLE_SimulatedProxy);

	AddOrUpdateWarpTargetFromComponent(WarpTargetName, Component, BoneName, bFollowComponent, EWarpTargetLocationOffsetDirection::TargetsForwardVector);

	if (Character->HasServerRole())
	{
		MulticastAddOrUpdateWarpTargetFromComponent(WarpTargetName, Component, BoneName, bFollowComponent);
	}
}

void UGarMotionWarpingComponent::AutonomousAddOrUpdateReplicatedWarpTargetFromComponent(FName WarpTargetName, const USceneComponent* Component, FName BoneName,
																						bool bFollowComponent)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	ensure(Character->GetLocalRole() > ROLE_SimulatedProxy);

	AddOrUpdateWarpTargetFromComponent(WarpTargetName, Component, BoneName, bFollowComponent, EWarpTargetLocationOffsetDirection::TargetsForwardVector);

	if (Character->IsCharacterSelf())
	{
		ServerAddOrUpdateWarpTargetFromComponent(WarpTargetName, Component, BoneName, bFollowComponent);
	}
}

void UGarMotionWarpingComponent::ServerAddOrUpdateWarpTargetFromComponent_Implementation(FName WarpTargetName, const USceneComponent* Component, FName BoneName,
																						 bool bFollowComponent)
{
	AddOrUpdateWarpTargetFromComponent(WarpTargetName, Component, BoneName, bFollowComponent, EWarpTargetLocationOffsetDirection::TargetsForwardVector);

	MulticastAddOrUpdateWarpTargetFromComponent(WarpTargetName, Component, BoneName, bFollowComponent);
}

void UGarMotionWarpingComponent::MulticastAddOrUpdateWarpTargetFromComponent_Implementation(FName WarpTargetName, const USceneComponent* Component, FName BoneName,
																							bool bFollowComponent)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		AddOrUpdateWarpTargetFromComponent(WarpTargetName, Component, BoneName, bFollowComponent, EWarpTargetLocationOffsetDirection::TargetsForwardVector);
	}
}
