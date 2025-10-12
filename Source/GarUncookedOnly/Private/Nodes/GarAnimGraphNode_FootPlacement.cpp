// Copyright Epic Games, Inc. All Rights Reserved.

#include "Nodes/GarAnimGraphNode_FootPlacement.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimGraphNode_FootPlacement)

/////////////////////////////////////////////////////
// UAnimGraphNode_FootPlacement

#define LOCTEXT_NAMESPACE "GarAnimGraphNode_FootPlacement"

UGarAnimGraphNode_FootPlacement::UGarAnimGraphNode_FootPlacement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UGarAnimGraphNode_FootPlacement::GetControllerDescription() const
{
	return LOCTEXT("GarFootPlacement", "Foot Placement For Mover");
}

FText UGarAnimGraphNode_FootPlacement::GetTooltipText() const
{
	return LOCTEXT("FootPlacementTooltip", "Foot Placement.");
}

FLinearColor UGarAnimGraphNode_FootPlacement::GetNodeTitleColor() const
{
	return FLinearColor(FColor(153.f, 0.f, 0.f));
}

FText UGarAnimGraphNode_FootPlacement::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

#undef LOCTEXT_NAMESPACE
