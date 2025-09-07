#include "GarCharacterTaskAnimInstance.h"
#include "GarCharacterTask.h"
#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimNodeReference.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacterTaskAnimInstance)

void UGarCharacterTaskAnimInstance::Refresh(const UGarCharacterTask* CharacterTask)
{
	bCharacterTaskActive = CharacterTask->IsActive();
}

void UGarCharacterTaskAnimInstance::ObserveBlending(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	ObservingFinalBlendWeight = Context.GetContext()->GetFinalBlendWeight();
}
