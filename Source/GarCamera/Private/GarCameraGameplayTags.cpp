#include "GarCameraGameplayTags.h"

namespace GarCameraTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Camera")
}

namespace GarCameraPerspectiveTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Camera.Perspective")
	UE_DEFINE_GAMEPLAY_TAG(FirstPerson, "Gar.Camera.Perspective.FirstPerson")
	UE_DEFINE_GAMEPLAY_TAG(ThirdPerson, "Gar.Camera.Perspective.ThirdPerson")
}

namespace GarCameraShoulderModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Camera.ShoulderMode")
	UE_DEFINE_GAMEPLAY_TAG(Center, "Gar.Camera.ShoulderMode.Center")
	UE_DEFINE_GAMEPLAY_TAG(Left, "Gar.Camera.ShoulderMode.Left")
	UE_DEFINE_GAMEPLAY_TAG(Right, "Gar.Camera.ShoulderMode.Right")
}

namespace GarCameraStatusTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Camera.Status")
	UE_DEFINE_GAMEPLAY_TAG(ForceTPP, "Gar.Camera.Status.ForceTPP")
}
