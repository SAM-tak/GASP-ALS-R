#pragma once

#include "GarLinkedAnimationInstance.h"
#include "GarCharacterTaskAnimInstance.generated.h"

class UGarCharacterTask;
struct FAnimUpdateContext;
struct FAnimNodeReference;

UCLASS(Abstract)
class GAR_API UGarCharacterTaskAnimInstance : public UGarLinkedAnimationInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAR|State", Transient)
	uint8 bCharacterTaskActive : 1{false};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAR|State", Transient)
	float ObservingFinalBlendWeight{0.0f};

public:
	virtual void Refresh(const UGarCharacterTask* CharacterTask);

	bool GetCharacterTaskActive()
	{
		return bCharacterTaskActive;
	}

	float GetObservingFinalBlendWeight()
	{
		return ObservingFinalBlendWeight;
	}

protected:
	UFUNCTION(BlueprintCallable, Category = "GAR|Linked Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe))
	void ObserveBlending(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
};
