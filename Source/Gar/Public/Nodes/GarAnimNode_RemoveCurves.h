#pragma once

#include "Animation/AnimNodeBase.h"
#include "GarAnimNode_RemoveCurves.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct GAR_API FGarAnimNode_RemoveCurves : public FAnimNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink SourcePose;

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;

	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;

	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
};
