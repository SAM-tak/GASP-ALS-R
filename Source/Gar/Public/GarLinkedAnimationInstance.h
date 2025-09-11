#pragma once

#include "Animation/AnimInstance.h"
#include "GarLinkedAnimationInstance.generated.h"

class AGarCharacter;
class UGarAnimationInstance;
class UGarViewAnimInstance;

UCLASS(Abstract)
class GAR_API UGarLinkedAnimationInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAR|State", Transient)
	TWeakObjectPtr<UGarAnimationInstance> Parent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAR|State", Transient)
	TWeakObjectPtr<AGarCharacter> Character;

public:
	virtual void NativeInitializeAnimation() override;

	virtual void NativeBeginPlay() override;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	TMap<FName, float>& GetAnimationCurvesFromProxy(EAnimCurveType InCurveType);

protected:
	// Be very careful when using this function to read your custom variables using the property access system. It is
	// safe to use this function to read variables that change only inside UGarAnimationInstance::NativeUpdateAnimation()
	// because it is guaranteed that this function will be called before parallel animation evaluation. Reading variables
	// that change in other functions can be dangerous because they can be changed in the game thread at the same
	// time as being read in the worker thread, which can lead to undefined behavior or even a crash. If you're not
	// sure what you're doing, then it's better to access your custom variables through the "Parent" variable.
	UFUNCTION(BlueprintPure, Category = "GAR|Linked Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe, ReturnDisplayName = "Parent"))
	UGarAnimationInstance* GetParentUnsafe() const;

	// utility for overlays. overlay accesses View->PitchAmount
	UFUNCTION(BlueprintPure, Category = "GAR|Linked Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe, ReturnDisplayName = "View"))
	UGarViewAnimInstance* GetViewUnsafe() const;

	// utility for overrides.
	UFUNCTION(BlueprintPure, Category = "GAR|Linked Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe, ReturnDisplayName = "View"))
	UGarRagdollingAnimInstance* GetRagdollingUnsafe() const;
};

inline UGarAnimationInstance* UGarLinkedAnimationInstance::GetParentUnsafe() const
{
	return Parent.Get();
}
