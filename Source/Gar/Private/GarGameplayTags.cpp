#include "GarGameplayTags.h"

namespace GarTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar")
}

namespace GarDesiredStateTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Desired")
}

namespace GarDesiredRotationModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Desired.RotationMode")
	UE_DEFINE_GAMEPLAY_TAG(VelocityDirection, "Gar.Desired.RotationMode.VelocityDirection")
	UE_DEFINE_GAMEPLAY_TAG(ViewDirection, "Gar.Desired.RotationMode.ViewDirection")
}

namespace GarDesiredStanceTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Desired.Stance")
	UE_DEFINE_GAMEPLAY_TAG(Standing, "Gar.Desired.Stance.Standing")
	UE_DEFINE_GAMEPLAY_TAG(Crouching, "Gar.Desired.Stance.Crouching")
	UE_DEFINE_GAMEPLAY_TAG(Lying, "Gar.Desired.Stance.Lying")
}

namespace GarDesiredGaitTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Desired.Gait")
	UE_DEFINE_GAMEPLAY_TAG(Walking, "Gar.Desired.Gait.Walking")
	UE_DEFINE_GAMEPLAY_TAG(Running, "Gar.Desired.Gait.Running")
	UE_DEFINE_GAMEPLAY_TAG(Sprinting, "Gar.Desired.Gait.Sprinting")
}

namespace GarPerspectiveTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Perspective")
	UE_DEFINE_GAMEPLAY_TAG(FirstPerson, "Gar.Perspective.FirstPerson")
	UE_DEFINE_GAMEPLAY_TAG(ThirdPerson, "Gar.Perspective.ThirdPerson")
}

namespace GarLocomotionModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.LocomotionMode")
	UE_DEFINE_GAMEPLAY_TAG(Grounded, "Gar.LocomotionMode.Grounded")
	UE_DEFINE_GAMEPLAY_TAG(InAir, "Gar.LocomotionMode.InAir")
}

namespace GarRotationModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.RotationMode")
	UE_DEFINE_GAMEPLAY_TAG(VelocityDirection, "Gar.RotationMode.VelocityDirection")
	UE_DEFINE_GAMEPLAY_TAG(ViewDirection, "Gar.RotationMode.ViewDirection")
	UE_DEFINE_GAMEPLAY_TAG(Aiming, "Gar.RotationMode.Aiming")
}

namespace GarAimingModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.AimingMode")
	UE_DEFINE_GAMEPLAY_TAG(AimDownSight, "Gar.AimingMode.AimDownSight")
	UE_DEFINE_GAMEPLAY_TAG(HipFire, "Gar.AimingMode.HipFire")
}

namespace GarStanceTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Stance")
	UE_DEFINE_GAMEPLAY_TAG(Standing, "Gar.Stance.Standing")
	UE_DEFINE_GAMEPLAY_TAG(Crouching, "Gar.Stance.Crouching")
	UE_DEFINE_GAMEPLAY_TAG(Lying, "Gar.Stance.Lying")
}

namespace GarGaitTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.Gait")
	UE_DEFINE_GAMEPLAY_TAG(Walking, "Gar.Gait.Walking")
	UE_DEFINE_GAMEPLAY_TAG(Running, "Gar.Gait.Running")
	UE_DEFINE_GAMEPLAY_TAG(Sprinting, "Gar.Gait.Sprinting")
}

namespace GarOverlayModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.OverlayMode")
	UE_DEFINE_GAMEPLAY_TAG(Default, "Gar.OverlayMode.Default")
	UE_DEFINE_GAMEPLAY_TAG(Injured, "Gar.OverlayMode.Injured")
	UE_DEFINE_GAMEPLAY_TAG(HandsTied, "Gar.OverlayMode.HandsTied")
}

namespace GarLocomotionActionTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.LocomotionAction")
	UE_DEFINE_GAMEPLAY_TAG(Traversal, "Gar.LocomotionAction.Traversal")
	UE_DEFINE_GAMEPLAY_TAG(GettingUp, "Gar.LocomotionAction.GettingUp")
	UE_DEFINE_GAMEPLAY_TAG(GettingDown, "Gar.LocomotionAction.GettingDown")
	UE_DEFINE_GAMEPLAY_TAG(Rolling, "Gar.LocomotionAction.Rolling")
	UE_DEFINE_GAMEPLAY_TAG(Landing, "Gar.LocomotionAction.Landing")
	UE_DEFINE_GAMEPLAY_TAG(FreeFalling, "Gar.LocomotionAction.FreeFalling")
	UE_DEFINE_GAMEPLAY_TAG(Unconsious, "Gar.LocomotionAction.Unconsious")
	UE_DEFINE_GAMEPLAY_TAG(Dying, "Gar.LocomotionAction.Dying")
}

namespace GarStateFlagTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.StateFlag")
	UE_DEFINE_GAMEPLAY_TAG(RotationLocked, "Gar.StateFlag.RotationLocked")
	UE_DEFINE_GAMEPLAY_TAG(BlockUpdateCapsuleSize, "Gar.StateFlag.BlockUpdateCapsuleSize")
	UE_DEFINE_GAMEPLAY_TAG(FacingUpward, "Gar.StateFlag.FacingUpward")
}

namespace GarTraversalActionTags
{
	UE_DEFINE_GAMEPLAY_TAG(Root, "Gar.TraversalAction")
	UE_DEFINE_GAMEPLAY_TAG(Vault, "Gar.TraversalAction.Vault")
	UE_DEFINE_GAMEPLAY_TAG(Hurdle, "Gar.TraversalAction.Hurdle")
	UE_DEFINE_GAMEPLAY_TAG(Mantle, "Gar.TraversalAction.Mantle")
}
