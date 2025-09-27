#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "GarAnimNotify_ForceBlendOut.generated.h"

UCLASS(DisplayName = "Gar Force Blend Out Animation Notify")
class GAR_API UGarAnimNotify_ForceBlendOut : public UAnimNotify
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (ForceUnits = "s"))
	float BlendOutDuration{0.25f};

public:
	UGarAnimNotify_ForceBlendOut();

	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Sequence) const override;
#endif

	virtual void BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload) override;
};
