#pragma once

#include "GarCharacterTaskAnimInstance.h"
#include "GarOverlayAnimInstance.generated.h"

UCLASS(Abstract, AutoExpandCategories = ("GAR|Settings"))
class GAR_API UGarOverlayAnimInstance : public UGarCharacterTaskAnimInstance
{
	GENERATED_BODY()

public:
	// utility for overlays. overlay accesses View->PitchAmount
	UFUNCTION(BlueprintPure, Category = "GAR|Linked Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe, ReturnDisplayName = "Parent"))
	UGarViewAnimInstance* GetViewUnsafe() const;

	// External aceess:
	// PoseState
	// IdleAdditiveAmount
	// CurrentGameplayTags
};
