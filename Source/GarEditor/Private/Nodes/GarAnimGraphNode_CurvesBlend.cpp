#include "Nodes/GarAnimGraphNode_CurvesBlend.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimGraphNode_CurvesBlend)

#define LOCTEXT_NAMESPACE "GarAnimGraphNode_CurvesBlend"

FText UGarAnimGraphNode_CurvesBlend::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Title", "Blend Curves");
}

FText UGarAnimGraphNode_CurvesBlend::GetTooltipText() const
{
	return LOCTEXT("Tooltip", "Blend Curves");
}

FString UGarAnimGraphNode_CurvesBlend::GetNodeCategory() const
{
	return FString{TEXTVIEW("GAR")};
}

#undef LOCTEXT_NAMESPACE
