#pragma once

#include "AIController.h"
#include "GarAIController.generated.h"

UCLASS(DisplayName = "Gar AI Controller")
class GAREXTRAS_API AGarAIController : public AAIController
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarAIController|Settings")
	TObjectPtr<UBehaviorTree> BehaviorTree;

protected:
	virtual void OnPossess(APawn* NewPawn) override;

public:
	virtual FVector GetFocalPointOnActor(const AActor* Actor) const override;
};
