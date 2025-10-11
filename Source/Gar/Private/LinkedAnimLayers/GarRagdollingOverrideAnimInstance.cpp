#include "LinkedAnimLayers/GarRagdollingOverrideAnimInstance.h"

#include "CharacterTasks/GarRagdollingTask.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarRagdollingOverrideAnimInstance)

void UGarRagdollingOverrideAnimInstance::ObserveBlending(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	ObservingFinalBlendWeight = Context.GetContext()->GetFinalBlendWeight();
}
