#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GarGameplayTags.h"
#include "GarAnimNotifyState_ApplyGameplayTag.generated.h"

UCLASS(DisplayName = "Gar Apply GameplayTag Animation Notify State")
class GAR_API UGarAnimNotifyState_ApplyGameplayTag : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TagToApply;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
