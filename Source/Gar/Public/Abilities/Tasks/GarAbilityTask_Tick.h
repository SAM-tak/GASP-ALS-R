#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GarAbilityTask_Tick.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGarAbilityTask_OnTickDelegate, float, DeltaTime);

/**
 * Task for abilities that supply tick and its' delta time.
 */
UCLASS()
class GAR_API UGarAbilityTask_Tick : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FGarAbilityTask_OnTickDelegate OnTick;

	UFUNCTION(BlueprintCallable, Category = "GAR|Ability|Tasks", meta = (DisplayName = "New Tick Ability Task", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UGarAbilityTask_Tick* New(UGameplayAbility* OwningAbility, const FName TaskInstanceName);

	virtual void TickTask(float DeltaTime) override;
};
