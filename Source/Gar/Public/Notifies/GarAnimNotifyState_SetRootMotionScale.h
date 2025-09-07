#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GarAnimNotifyState_SetRootMotionScale.generated.h"

UCLASS(DisplayName = "Gar Set Root Motion Scale Animation Notify State")
class GAR_API UGarAnimNotifyState_SetRootMotionScale : public UAnimNotifyState
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "x"))
	float TranslationScale{1.0f};

public:
	UGarAnimNotifyState_SetRootMotionScale();

	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	                         float Duration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	                       const FAnimNotifyEventReference& EventReference) override;
};
