#pragma once

#include "AnimGraphNode_Base.h"
#include "Nodes/GarAnimNode_RemoveCurves.h"
#include "GarAnimGraphNode_RemoveCurves.generated.h"

UCLASS()
class GAREDITOR_API UGarAnimGraphNode_RemoveCurves : public UAnimGraphNode_Base
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarAnimNode_RemoveCurves Node;

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetTooltipText() const override;

	virtual FString GetNodeCategory() const override;
};
