#include "LinkedAnimLayers/GarRagdollingAnimInstance.h"
#include "Abilities/Actions/GarGameplayAbility_Ragdolling.h"
#include "GarPhysicalAnimationComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarRagdollingAnimInstance)

FPoseSnapshot& UGarRagdollingAnimInstance::GetFinalPoseSnapshot()
{
	check(IsInGameThread())

	return FinalPose;
}

void UGarRagdollingAnimInstance::Freeze()
{
	check(IsInGameThread())

	if (!FinalPose.bIsValid)
	{
		// Save a snapshot of the current ragdoll pose for use in animation graph to blend out of the ragdoll.
		if (GetSkelMeshComponent()->GetNumComponentSpaceTransforms() > 0) // When stop PIE, SnapshotPose rises Out of range exception.
		{
			SnapshotPose(FinalPose);
		}
	}
}

void UGarRagdollingAnimInstance::UnFreeze()
{
	check(IsInGameThread())

	if (FinalPose.bIsValid)
	{
		FinalPose.Reset();
	}
}

void UGarRagdollingAnimInstance::Refresh(const FGarRagdollingState& State, bool bNewActive)
{
	check(IsInGameThread())

	//bActive = Ability.IsActive() && !Ability.bIsAbilityEnding; // this is not work. bIsAbilityEnding can be true in Super::EndAbility.
	bActive = bNewActive;
	bGroundedAndAged = State.IsGroundedAndAged();
	bFacingUpward = State.bFacingUpward;
	LyingDownYawAngleDelta = State.LyingDownYawAngleDelta;
}
