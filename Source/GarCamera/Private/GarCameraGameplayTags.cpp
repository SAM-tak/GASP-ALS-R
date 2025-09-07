#include "GarCameraGameplayTags.h"

namespace GarCameraTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Camera")
}

namespace GarCameraViewModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Camera.ViewMode")
	UE_DEFINE_GAMEPLAY_TAG(FirstPerson, "Gar.Camera.ViewMode.FirstPerson")
	UE_DEFINE_GAMEPLAY_TAG(ThirdPerson, "Gar.Camera.ViewMode.ThirdPerson")
}

namespace GarCameraShoulderModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Camera.ShoulderMode")
	UE_DEFINE_GAMEPLAY_TAG(Center, "Gar.Camera.ShoulderMode.Center")
	UE_DEFINE_GAMEPLAY_TAG(Left, "Gar.Camera.ShoulderMode.Left")
	UE_DEFINE_GAMEPLAY_TAG(Right, "Gar.Camera.ShoulderMode.Right")
}
