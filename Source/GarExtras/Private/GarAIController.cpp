#include "GarAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAIController)

AGarAIController::AGarAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bAttachToPawn = true;
}

void AGarAIController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);

	RunBehaviorTree(BehaviorTree);
}

FVector AGarAIController::GetFocalPointOnActor(const AActor* Actor) const
{
	const auto* FocusedPawn{Cast<APawn>(Actor)};
	if (IsValid(FocusedPawn))
	{
		return FocusedPawn->GetPawnViewLocation();
	}

	return Super::GetFocalPointOnActor(Actor);
}
