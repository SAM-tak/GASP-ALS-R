#include "Notifies/GarAnimNotify_SetGroundedEntryMode.h"

#include "GarCharacter.h"
#include "GarAnimationInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimNotify_SetGroundedEntryMode)

UGarAnimNotify_SetGroundedEntryMode::UGarAnimNotify_SetGroundedEntryMode()
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
}

FString UGarAnimNotify_SetGroundedEntryMode::GetNotifyName_Implementation() const
{
	return TEXT("Gar Set Grounded Entry Mode");
}

void UGarAnimNotify_SetGroundedEntryMode::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Sequence,
                                                 const FAnimNotifyEventReference& NotifyEventReference)
{
	Super::Notify(Mesh, Sequence, NotifyEventReference);

	auto* Character{Cast<AGarCharacter>(Mesh->GetOwner())};
	auto* AnimationInstance{Cast<UGarAnimationInstance>(Mesh->GetAnimInstance())};
	if (IsValid(AnimationInstance))
	{
		for(auto& Value : GroundedEntryMode)
		{
			if (!Value.TagsForMatch.IsValid() || Character->HasAllMatchingGameplayTags(Value.TagsForMatch))
			{
				AnimationInstance->SetGroundedEntryMode(Value.GroundedEntryMode, Value.StartPosition);
				break;
			}
		}
	}
}
