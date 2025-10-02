// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModifiers/GarMoverModifier.h"

#include "GarGameplayTags.h"
#include "Utility/GarUtility.h"

FGarMoverModifier::FGarMoverModifier()
{
	DurationMs = -1.0f;
}

bool FGarMoverModifier::HasGameplayTag(FGameplayTag TagToFind, bool bExactMatch) const
{
	if (bExactMatch)
	{
		return TagToFind.MatchesTagExact(ActiveTag);
	}

	return TagToFind.MatchesTag(ActiveTag);
}
