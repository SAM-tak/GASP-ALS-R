#pragma once

#include "GarOverrideAnimInstance.h"
#include "GarRagdollingOverrideAnimInstance.generated.h"

class UGarRagdollingTask;
struct FAnimUpdateContext;
struct FAnimNodeReference;

UCLASS(Abstract)
class GAR_API UGarRagdollingOverrideAnimInstance : public UGarOverrideAnimInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAR|State", Transient)
	uint8 bRagdollingTaskActive : 1{false};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAR|State", Transient)
	float ObservingFinalBlendWeight{0.0f};

public:
	void Reset()
	{
		bRagdollingTaskActive = false;
		ObservingFinalBlendWeight = 0.0f;
	}

	void SetRagdollingTaskActive(bool bNewRagdollingTaskActive)
	{
		bRagdollingTaskActive = bNewRagdollingTaskActive;
	}

	bool GetRagdollingTaskActive()
	{
		return bRagdollingTaskActive;
	}

	float GetObservingFinalBlendWeight()
	{
		return ObservingFinalBlendWeight;
	}

protected:
	UFUNCTION(BlueprintCallable, Category = "GAR|Linked Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe))
	void ObserveBlending(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
};
