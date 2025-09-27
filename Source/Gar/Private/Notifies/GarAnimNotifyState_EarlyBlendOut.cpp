#include "Notifies/GarAnimNotifyState_EarlyBlendOut.h"

#include "GarCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimNotifyState_EarlyBlendOut)

UGarAnimNotifyState_EarlyBlendOut::UGarAnimNotifyState_EarlyBlendOut()
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif

	bIsNativeBranchingPoint = true;
}

FString UGarAnimNotifyState_EarlyBlendOut::GetNotifyName_Implementation() const
{
	return FString{TEXTVIEW("Gar Early Blend Out")};
}

#if WITH_EDITOR
bool UGarAnimNotifyState_EarlyBlendOut::CanBePlaced(UAnimSequenceBase* Sequence) const
{
	return IsValid(Sequence) && Sequence->IsA<UAnimMontage>();
}
#endif

void UGarAnimNotifyState_EarlyBlendOut::BranchingPointNotifyTick(FBranchingPointNotifyPayload& NotifyPayload, const float DeltaTime)
{
	Super::BranchingPointNotifyTick(NotifyPayload, DeltaTime);

	const auto* Mesh{NotifyPayload.SkelMeshComponent};
	auto* AnimationInstance{IsValid(Mesh) ? Mesh->GetAnimInstance() : nullptr};
	const auto* Character{IsValid(AnimationInstance) ? Cast<AGarCharacter>(Mesh->GetOwner()) : nullptr};

	if (IsValid(Character) &&
	    ((bCheckInput && Character->HasMovementInput()) ||
	     (bCheckLocomotionMode && Character->GetLocomotionMode() == LocomotionModeEquals) ||
	     (bCheckRotationMode && Character->GetRotationMode() == RotationModeEquals) ||
	     (bCheckStance && Character->GetStance() == StanceEquals)))
	{
		auto* MontageInstance{AnimationInstance->GetMontageInstanceForID(NotifyPayload.MontageInstanceID)};
		if (!ensure(MontageInstance != nullptr))
		{
			return;
		}

		const auto* Montage{MontageInstance->Montage.Get()};

		FMontageBlendSettings BlendOutSettings{Montage->BlendOut};
		BlendOutSettings.Blend.BlendTime = BlendOutDuration;
		BlendOutSettings.BlendMode = Montage->BlendModeOut;
		BlendOutSettings.BlendProfile = Montage->BlendProfileOut;

		MontageInstance->Stop(BlendOutSettings);
	}
}
