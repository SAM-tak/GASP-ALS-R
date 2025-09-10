#pragma once

#include "AnimGraphNode_Base.h"
#include "Nodes/GarAnimNode_CurvesBlend.h"
#include "GarAnimGraphNode_CurvesBlend.generated.h"

UCLASS()
class GARUNCOOKEDONLY_API UGarAnimGraphNode_CurvesBlend : public UAnimGraphNode_Base
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarAnimNode_CurvesBlend Node;

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetTooltipText() const override;

	virtual FString GetNodeCategory() const override;
};
