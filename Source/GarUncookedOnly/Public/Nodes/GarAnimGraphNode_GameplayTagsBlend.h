#pragma once

#include "AnimGraphNode_BlendListBase.h"
#include "Nodes/GarAnimNode_GameplayTagsBlend.h"
#include "GarAnimGraphNode_GameplayTagsBlend.generated.h"

UCLASS()
class GARUNCOOKEDONLY_API UGarAnimGraphNode_GameplayTagsBlend : public UAnimGraphNode_BlendListBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarAnimNode_GameplayTagsBlend Node;

public:
	UGarAnimGraphNode_GameplayTagsBlend();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& ChangedEvent) override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetTooltipText() const override;

	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& PreviousPins) override;

	virtual FString GetNodeCategory() const override;

	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 PinIndex) const override;

protected:
	static void GetBlendPinProperties(const UEdGraphPin* Pin, bool& bBlendPosePin, bool& bBlendTimePin);
};
