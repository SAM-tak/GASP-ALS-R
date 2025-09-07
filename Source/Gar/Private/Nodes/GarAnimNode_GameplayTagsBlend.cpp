#include "Nodes/GarAnimNode_GameplayTagsBlend.h"

#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimNode_GameplayTagsBlend)

int32 FGarAnimNode_GameplayTagsBlend::GetActiveChildIndex()
{
	const auto& CurrentContainer{GetContainer()};

	auto Index{1};

	for (auto& TagMatch : GetTagMatches())
	{
		if (TagMatch.bAll)
		{
			if (CurrentContainer.IsValid() && CurrentContainer.HasAll(TagMatch.Tags))
			{
				return Index;
			}
		}
		else
		{
			if (CurrentContainer.IsValid() && CurrentContainer.HasAny(TagMatch.Tags))
			{
				return Index;
			}
		}
		++Index;
	}

	return 0;
}

const FGameplayTagContainer& FGarAnimNode_GameplayTagsBlend::GetContainer() const
{
	return GET_ANIM_NODE_DATA(FGameplayTagContainer, Container);
}

const TArray<FGarGameplayTagContainerMatch>& FGarAnimNode_GameplayTagsBlend::GetTagMatches() const
{
	return GET_ANIM_NODE_DATA(TArray<FGarGameplayTagContainerMatch>, TagMatches);
}

#if WITH_EDITOR
void FGarAnimNode_GameplayTagsBlend::RefreshPosePins()
{
	const auto Difference{BlendPose.Num() - GetTagMatches().Num() - 1};
	if (Difference == 0)
	{
		return;
	}

	if (Difference > 0)
	{
		for (auto Idx{Difference}; Idx > 0; --Idx)
		{
			RemovePose(BlendPose.Num() - 1);
		}
	}
	else
	{
		for (auto Idx{Difference}; Idx < 0; Idx++)
		{
			AddPose();
		}
	}
}

FText FGarAnimNode_GameplayTagsBlend::GetPosePinName(const FGarGameplayTagContainerMatch& Match)
{
	if (Match.Tags.Num() == 0)
	{
		return FText::GetEmpty();
	}
	if (Match.Tags.Num() == 1)
	{
		return FText::FromName(UGarUtility::GetSimpleTagName(Match.Tags.First()));
	}
	
	TStringBuilder<32> StringBuilder;
	int LineWidth = 0;
	for(auto& Tag : Match.Tags)
	{
		if (StringBuilder.Len() > 0)
		{
			StringBuilder << (Match.bAll ? TEXT("&") : TEXT("|"));
			LineWidth++;
		}
		int PrevLen = StringBuilder.Len();
		StringBuilder << UGarUtility::GetSimpleTagName(Tag);
		LineWidth += StringBuilder.Len() - PrevLen;
		if (LineWidth > 16)
		{
			StringBuilder << TEXT("\n");
			LineWidth = 0;
		}
	}
	return FText::AsCultureInvariant(StringBuilder.ToString());
}
#endif
