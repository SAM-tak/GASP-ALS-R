#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GarGameplayTags.h"
#include "GarConstants.generated.h"

UCLASS(Meta = (BlueprintThreadSafe))
class GAR_API UGarConstants : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Layering Animation Curves

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerHeadCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerHeadAdditiveCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerHeadSlotCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerArmLeftCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerArmLeftAdditiveCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerArmLeftLocalSpaceCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerArmLeftSlotCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerArmRightCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerArmRightAdditiveCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerArmRightLocalSpaceCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerArmRightSlotCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerHandLeftCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerHandRightCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerSpineCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerSpineAdditiveCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerSpineSlotCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerPelvisCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerPelvisSlotCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerLegsCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& LayerLegsSlotCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& HandLeftIkCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& HandRightIkCurveName();

	// Pose Animation Curves

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseGroundedCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseInAirCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseGaitCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseMovingCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseStandingCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseCrouchingCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseLyingCurveName();

	// Feet Animation Curves

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& FootLeftIkCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& FootRightIkCurveName();

	// Other Animation Curves

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& AllowTransitionsCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& BlockViewCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& BlockSprintCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& AllowAimingCurveName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& FootstepSoundBlockCurveName();

	// Debug

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Debug", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& CurvesDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Debug", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& StateDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Debug", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& ShapesDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Debug", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& TracesDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Debug", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& TraversalDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants|Debug", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& PADebugDisplayName();

	// GameplayTag

	UFUNCTION(BlueprintPure, Category = "GAR|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FGameplayTagContainer& ViewModeRoot();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FGameplayTagContainer& AimingModeRoot();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FGameplayTagContainer& LocomotionModeRoot();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FGameplayTagContainer& RotationModeRoot();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FGameplayTagContainer& StanceRoot();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FGameplayTagContainer& GaitRoot();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FGameplayTagContainer& OverlayModeRoot();

	UFUNCTION(BlueprintPure, Category = "GAR|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FGameplayTagContainer& LocomotionActionRoot();
};

inline const FName& UGarConstants::LayerHeadCurveName()
{
	static const FName Name{TEXTVIEW("layering_head")};
	return Name;
}

inline const FName& UGarConstants::LayerHeadAdditiveCurveName()
{
	static const FName Name{TEXTVIEW("layering_head_add")};
	return Name;
}

inline const FName& UGarConstants::LayerHeadSlotCurveName()
{
	static const FName Name{TEXTVIEW("layering_head_slot")};
	return Name;
}

inline const FName& UGarConstants::LayerArmLeftCurveName()
{
	static const FName Name{TEXTVIEW("layering_arm_l")};
	return Name;
}

inline const FName& UGarConstants::LayerArmLeftAdditiveCurveName()
{
	static const FName Name{TEXTVIEW("layering_arm_l_add")};
	return Name;
}

inline const FName& UGarConstants::LayerArmLeftLocalSpaceCurveName()
{
	static const FName Name{TEXTVIEW("layering_arm_l_ls")};
	return Name;
}

inline const FName& UGarConstants::LayerArmLeftSlotCurveName()
{
	static const FName Name{TEXTVIEW("layering_Arm_l_slot")};
	return Name;
}

inline const FName& UGarConstants::LayerArmRightCurveName()
{
	static const FName Name{TEXTVIEW("layering_arm_r")};
	return Name;
}

inline const FName& UGarConstants::LayerArmRightAdditiveCurveName()
{
	static const FName Name{TEXTVIEW("layering_arm_r_add")};
	return Name;
}

inline const FName& UGarConstants::LayerArmRightLocalSpaceCurveName()
{
	static const FName Name{TEXTVIEW("layering_arm_r_ls")};
	return Name;
}

inline const FName& UGarConstants::LayerArmRightSlotCurveName()
{
	static const FName Name{TEXTVIEW("layering_arm_r_slot")};
	return Name;
}

inline const FName& UGarConstants::LayerHandLeftCurveName()
{
	static const FName Name{TEXTVIEW("layering_hand_l")};
	return Name;
}

inline const FName& UGarConstants::LayerHandRightCurveName()
{
	static const FName Name{TEXTVIEW("layering_hand_r")};
	return Name;
}

inline const FName& UGarConstants::LayerSpineCurveName()
{
	static const FName Name{TEXTVIEW("layering_spine")};
	return Name;
}

inline const FName& UGarConstants::LayerSpineAdditiveCurveName()
{
	static const FName Name{TEXTVIEW("layering_spine_add")};
	return Name;
}

inline const FName& UGarConstants::LayerSpineSlotCurveName()
{
	static const FName Name{TEXTVIEW("layering_spine_slot")};
	return Name;
}

inline const FName& UGarConstants::LayerPelvisCurveName()
{
	static const FName Name{TEXTVIEW("layering_pelvis")};
	return Name;
}

inline const FName& UGarConstants::LayerPelvisSlotCurveName()
{
	static const FName Name{TEXTVIEW("layering_pelvis_slot")};
	return Name;
}

inline const FName& UGarConstants::LayerLegsCurveName()
{
	static const FName Name{TEXTVIEW("layering_legs")};
	return Name;
}

inline const FName& UGarConstants::LayerLegsSlotCurveName()
{
	static const FName Name{TEXTVIEW("layering_legs_slot")};
	return Name;
}

inline const FName& UGarConstants::HandLeftIkCurveName()
{
	static const FName Name{TEXTVIEW("enable_handik_l")};
	return Name;
}

inline const FName& UGarConstants::HandRightIkCurveName()
{
	static const FName Name{TEXTVIEW("enable_handik_r")};
	return Name;
}

inline const FName& UGarConstants::AllowAimingCurveName()
{
	static const FName Name{TEXTVIEW("allow_aiming")};
	return Name;
}

inline const FName& UGarConstants::PoseGroundedCurveName()
{
	static const FName Name{ TEXTVIEW("pose_grounded") };
	return Name;
}

inline const FName& UGarConstants::PoseInAirCurveName()
{
	static const FName Name{ TEXTVIEW("pose_inair") };
	return Name;
}

inline const FName& UGarConstants::PoseGaitCurveName()
{
	static const FName Name{TEXTVIEW("pose_gait")};
	return Name;
}

inline const FName& UGarConstants::PoseMovingCurveName()
{
	static const FName Name{TEXTVIEW("pose_moving")};
	return Name;
}

inline const FName& UGarConstants::PoseStandingCurveName()
{
	static const FName Name{TEXTVIEW("pose_standing")};
	return Name;
}

inline const FName& UGarConstants::PoseCrouchingCurveName()
{
	static const FName Name{TEXTVIEW("pose_crouching")};
	return Name;
}

inline const FName& UGarConstants::PoseLyingCurveName()
{
	static const FName Name{TEXTVIEW("pose_lying")};
	return Name;
}

inline const FName& UGarConstants::FootLeftIkCurveName()
{
	static const FName Name{TEXTVIEW("enable_footik_l")};
	return Name;
}

inline const FName& UGarConstants::FootRightIkCurveName()
{
	static const FName Name{TEXTVIEW("enable_footik_r")};
	return Name;
}

inline const FName& UGarConstants::AllowTransitionsCurveName()
{
	static const FName Name{TEXTVIEW("allow_transitions")};
	return Name;
}

inline const FName& UGarConstants::BlockViewCurveName()
{
	static const FName Name{ TEXTVIEW("block_view") };
	return Name;
}

inline const FName& UGarConstants::BlockSprintCurveName()
{
	static const FName Name{TEXTVIEW("block_sprint")};
	return Name;
}

inline const FName& UGarConstants::FootstepSoundBlockCurveName()
{
	static const FName Name{TEXTVIEW("block_footstep_sound")};
	return Name;
}

inline const FName& UGarConstants::CurvesDebugDisplayName()
{
	static const FName Name{TEXTVIEW("GAR.Curves")};
	return Name;
}

inline const FName& UGarConstants::StateDebugDisplayName()
{
	static const FName Name{TEXTVIEW("GAR.State")};
	return Name;
}

inline const FName& UGarConstants::ShapesDebugDisplayName()
{
	static const FName Name{TEXTVIEW("GAR.Shapes")};
	return Name;
}

inline const FName& UGarConstants::TracesDebugDisplayName()
{
	static const FName Name{TEXTVIEW("GAR.Traces")};
	return Name;
}

inline const FName& UGarConstants::TraversalDebugDisplayName()
{
	static const FName Name{TEXTVIEW("GAR.Traversal")};
	return Name;
}

inline const FName& UGarConstants::PADebugDisplayName()
{
	static const FName Name{TEXTVIEW("GAR.PhysicalAnimation")};
	return Name;
}

inline const FGameplayTagContainer& UGarConstants::ViewModeRoot()
{
	static const FGameplayTagContainer Container{GarPerspectiveTags::Root};
	return Container;
}

inline const FGameplayTagContainer& UGarConstants::AimingModeRoot()
{
	static const FGameplayTagContainer Container{GarAimingModeTags::Root};
	return Container;
}

inline const FGameplayTagContainer& UGarConstants::LocomotionModeRoot()
{
	static const FGameplayTagContainer Container{GarLocomotionModeTags::Root};
	return Container;
}

inline const FGameplayTagContainer& UGarConstants::RotationModeRoot()
{
	static const FGameplayTagContainer Container{GarRotationModeTags::Root};
	return Container;
}

inline const FGameplayTagContainer& UGarConstants::StanceRoot()
{
	static const FGameplayTagContainer Container{GarStanceTags::Root};
	return Container;
}

inline const FGameplayTagContainer& UGarConstants::GaitRoot()
{
	static const FGameplayTagContainer Container{GarGaitTags::Root};
	return Container;
}

inline const FGameplayTagContainer& UGarConstants::OverlayModeRoot()
{
	static const FGameplayTagContainer Container{GarOverlayModeTags::Root};
	return Container;
}

inline const FGameplayTagContainer& UGarConstants::LocomotionActionRoot()
{
	static const FGameplayTagContainer Container{GarLocomotionActionTags::Root};
	return Container;
}
