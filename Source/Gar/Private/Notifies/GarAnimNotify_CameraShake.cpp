#include "Notifies/GarAnimNotify_CameraShake.h"

#include "Camera/CameraShakeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GarCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimNotify_CameraShake)

UGarAnimNotify_CameraShake::UGarAnimNotify_CameraShake()
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
}

FString UGarAnimNotify_CameraShake::GetNotifyName_Implementation() const
{
	TStringBuilder<256> NotifyNameBuilder{InPlace, TEXTVIEW("Gar Camera Shake: ")};

	if (IsValid(CameraShakeClass))
	{
		NotifyNameBuilder << CameraShakeClass->GetFName();
	}

	return FString{NotifyNameBuilder};
}

void UGarAnimNotify_CameraShake::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
                                        const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(Mesh, Animation, EventReference);

	if (!AnyMatchTags.IsEmpty() || !AllMatchTags.IsEmpty())
	{
		const auto* Character{Cast<AGarCharacter>(Mesh->GetOwner())};
		if (!IsValid(Character)
			|| (!AnyMatchTags.IsEmpty() && !Character->HasAnyMatchingGameplayTags(AnyMatchTags))
			|| (!AllMatchTags.IsEmpty() && !Character->HasAllMatchingGameplayTags(AllMatchTags)))
		{
			return;
		}
	}
	
	const auto* Pawn{Cast<APawn>(Mesh->GetOwner())};
	const auto* PlayerController{IsValid(Pawn) ? Cast<APlayerController>(Pawn->GetController()) : nullptr};
	auto* CameraManager{PlayerController && IsValid(PlayerController) ? PlayerController->PlayerCameraManager.Get() : nullptr};

	if (IsValid(CameraManager))
	{
		CameraManager->StartCameraShake(CameraShakeClass, CameraShakeScale, PlaySpace, UserPlaySpaceRot);
	}
}

#if WITH_EDITOR
void UGarAnimNotify_CameraShake::OnAnimNotifyCreatedInEditor(FAnimNotifyEvent& ContainingAnimNotifyEvent)
{
	ContainingAnimNotifyEvent.bTriggerOnDedicatedServer = false;
}
#endif
