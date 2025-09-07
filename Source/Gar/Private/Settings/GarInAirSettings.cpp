#include "Settings/GarInAirSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarInAirSettings)

#if WITH_EDITOR
void FGarInAirSettings::PostEditChangeProperty(const FPropertyChangedEvent& ChangedEvent)
{
	if (ChangedEvent.GetPropertyName() == GET_MEMBER_NAME_STRING_VIEW_CHECKED(FGarInAirSettings, GroundPredictionResponseChannels))
	{
		GroundPredictionSweepResponses.SetAllChannels(ECR_Ignore);

		for (const auto CollisionChannel : GroundPredictionResponseChannels)
		{
			GroundPredictionSweepResponses.SetResponse(CollisionChannel, ECR_Block);
		}
	}
}
#endif
