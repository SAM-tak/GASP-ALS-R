#pragma once

#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GarAnimNotify_SetGroundedEntryMode.generated.h"

USTRUCT(BlueprintType)
struct FGarGroundedEntryMode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGameplayTag GroundedEntryMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float StartPosition{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGameplayTagContainer TagsForMatch;
};

UCLASS(DisplayName = "Gar Set Grounded Entry Mode Animation Notify")
class GAR_API UGarAnimNotify_SetGroundedEntryMode : public UAnimNotify
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FGarGroundedEntryMode> GroundedEntryMode;

public:
	UGarAnimNotify_SetGroundedEntryMode();

	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	                    const FAnimNotifyEventReference& EventReference) override;
};
