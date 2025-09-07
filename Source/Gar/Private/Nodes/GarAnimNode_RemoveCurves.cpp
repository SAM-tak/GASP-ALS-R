#include "Nodes/GarAnimNode_RemoveCurves.h"

#include "Animation/AnimTrace.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimNode_RemoveCurves)

void FGarAnimNode_RemoveCurves::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	Super::Initialize_AnyThread(Context);

	SourcePose.Initialize(Context);
}

void FGarAnimNode_RemoveCurves::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	Super::Update_AnyThread(Context);

	SourcePose.Update(Context);
}

void FGarAnimNode_RemoveCurves::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()
	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(CurvesBlend, !IsInGameThread());

	Super::Evaluate_AnyThread(Output);

	SourcePose.Evaluate(Output);

	Output.Curve.Empty();
}
