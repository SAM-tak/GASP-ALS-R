#pragma once

#include "GameplayTagContainer.h"
#include "AnimNodes/AnimNode_BlendListBase.h"
#include "GarAnimNode_GameplayTagsBlend.generated.h"

#if WITH_EDITORONLY_DATA
USTRUCT()
struct GAR_API FGarGameplayTagContainerMatch
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere)
	bool bAll{false};
};
#endif

USTRUCT()
struct GAR_API FGarAnimNode_GameplayTagsBlend : public FAnimNode_BlendListBase
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = Settings, Meta = (FoldProperty, PinShownByDefault))
	FGameplayTagContainer Container;

	UPROPERTY(EditAnywhere, Category = Settings, Meta = (FoldProperty))
	TArray<FGarGameplayTagContainerMatch> TagMatches;
#endif

protected:
	virtual int32 GetActiveChildIndex() override;

public:
	const FGameplayTagContainer& GetContainer() const;

	const TArray<FGarGameplayTagContainerMatch>& GetTagMatches() const;

#if WITH_EDITOR
	void RefreshPosePins();

	static FText GetPosePinName(const FGarGameplayTagContainerMatch& Match);
#endif
};
