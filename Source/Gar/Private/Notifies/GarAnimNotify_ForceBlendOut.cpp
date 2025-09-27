#include "Notifies/GarAnimNotify_ForceBlendOut.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimNotify_ForceBlendOut)

UGarAnimNotify_ForceBlendOut::UGarAnimNotify_ForceBlendOut()
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif

	bIsNativeBranchingPoint = true;
}

FString UGarAnimNotify_ForceBlendOut::GetNotifyName_Implementation() const
{
	return FString{TEXTVIEW("Gar Force Blend Out")};
}

#if WITH_EDITOR
bool UGarAnimNotify_ForceBlendOut::CanBePlaced(UAnimSequenceBase* Sequence) const
{
	return IsValid(Sequence) && Sequence->IsA<UAnimMontage>();
}
#endif

void UGarAnimNotify_ForceBlendOut::BranchingPointNotify(FBranchingPointNotifyPayload& NotifyPayload)
{
	Super::BranchingPointNotify(NotifyPayload);

	auto* AnimationInstance{NotifyPayload.SkelMeshComponent->GetAnimInstance()};

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
