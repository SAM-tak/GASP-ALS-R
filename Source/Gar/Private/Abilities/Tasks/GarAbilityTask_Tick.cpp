#include "Abilities/Tasks/GarAbilityTask_Tick.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAbilityTask_Tick)

UGarAbilityTask_Tick::UGarAbilityTask_Tick(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bTickingTask = true;
}

UGarAbilityTask_Tick *UGarAbilityTask_Tick::New(UGameplayAbility* OwningAbility, const FName TaskInstanceName)
{
    return NewAbilityTask<UGarAbilityTask_Tick>(OwningAbility, TaskInstanceName);
}

void UGarAbilityTask_Tick::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);
    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnTick.Broadcast(DeltaTime);
    }
}
