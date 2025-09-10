#include "Nodes/GarAnimGraphNode_RemoveCurves.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimGraphNode_RemoveCurves)

#define LOCTEXT_NAMESPACE "GarAnimGraphNode_RemoveCurves"

FText UGarAnimGraphNode_RemoveCurves::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Title", "Remove Curves");
}

FText UGarAnimGraphNode_RemoveCurves::GetTooltipText() const
{
	return LOCTEXT("Tooltip", "Remove Curves");
}

FString UGarAnimGraphNode_RemoveCurves::GetNodeCategory() const
{
	return FString{TEXTVIEW("GAR")};
}

#undef LOCTEXT_NAMESPACE
