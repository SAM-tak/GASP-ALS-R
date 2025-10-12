// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "BoneControllers/AnimNode_FootPlacement.h"

#include "GarAnimNode_FootPlacement.generated.h"

#define UE_API GAR_API

namespace GAR::Anim::FootPlacement
{
	enum class EPlantType
	{
		Unplanted,
		Planted,
		Replanted
	};

	struct FLegRuntimeData
	{
		int32 Idx = -1;

		// Bone information that can be cached once per-lod change.
		struct FBoneData
		{
			FCompactPoseBoneIndex FKIndex = FCompactPoseBoneIndex(INDEX_NONE);
			FCompactPoseBoneIndex BallIndex = FCompactPoseBoneIndex(INDEX_NONE);
			FCompactPoseBoneIndex IKIndex = FCompactPoseBoneIndex(INDEX_NONE);
			FCompactPoseBoneIndex HipIndex = FCompactPoseBoneIndex(INDEX_NONE);
			float LimbLength = 0.0f;
			float FootLength = 0.0f;
		} Bones;

		FName SpeedCurveName = NAME_None;
		FName DisableLockCurveName = NAME_None;
		FName DisableLegCurveName = NAME_None;

		// Helper struct to store values coming directly, or trivial to calculate from just the input pose.
		struct FInputPoseData
		{
			FTransform FootFKTransformCS = FTransform::Identity;
			FTransform FootTransformCS = FTransform::Identity;
			FTransform BallTransformCS = FTransform::Identity;
			FTransform HipTransformCS = FTransform::Identity;
			FTransform BallToFoot = FTransform::Identity;
			FTransform FootToBall = FTransform::Identity;
			#if ENABLE_ANIM_DEBUG
			// These are only used for debug draw at the moment
			// @TODO: Use this info to more precisely figure out foot dimensions
			FTransform FootToGround = FTransform::Identity;
			FTransform BallToGround = FTransform::Identity;
			#endif
			float Speed = 0.0f;
			float DisableLeg = 0.0f;
			float LockAlpha = 0.0f;
			float DistanceToPlant = 0.0f;
			// Calculated from a range of toe speeds to define when to blend in/out ground rotational alignment
			// @TODO: When we have prediction/phase info, replac with roll-phase alpha
			float AlignmentAlpha = 0.0f;
		} InputPose;

		/* Ground */

		struct FPlantData
		{
			EPlantType PlantType = EPlantType::Unplanted;
			EPlantType LastPlantType = EPlantType::Unplanted;
			FPlane PlantPlaneRS = FPlane(FVector::UpVector, 0.0f);
			FQuat TwistCorrection = FQuat::Identity;
			// @TODO: When we have prediction/phase info, replace use-cases with post-plant roll-phase
			float TimeSinceFullyUnaligned = 0.0f;
			// Whether the planted/locked target has ever been reachable this plant
			bool bCanReachTarget = false;
			// Whether we want to plant, independently from any dynamic pose adjustments we may do
			bool bWantsToPlant = false;

			FPlane GetPlantPlaneCS(const FTransform& RootToComponent) const { return PlantPlaneRS.TransformBy(RootToComponent.ToMatrixWithScale()); }
			FPlane GetPlantPlaneWS(const FTransform& RootToComponent, const FTransform& ComponentToWorld) const
			{
				return GetPlantPlaneCS(RootToComponent).TransformBy(ComponentToWorld.ToMatrixWithScale());
			}

		} Plant;

		// Ground-aligned, locked/unlocked bone transform pre-extension adjustments
		FTransform AlignedFootTransformWS = FTransform::Identity;
		FTransform AlignedFootTransformRS = FTransform::Identity;
		// Foot locked/unlocked bone transform, before ground alignment
		FTransform UnalignedFootTransformWS = FTransform::Identity;
		FTransform UnalignedFootTransformRS = FTransform::Identity;

		/* Interpolation */
		struct FInterpolationData
		{
			// Interpolated foot lock offset
			FTransform UnalignedFootOffset = FTransform::Identity;
			// Separating plane spring states
			FVectorSpringState SeparatingPlaneOffsetSpringState;
			FVector SeparatingPlaneOffset = FVector::ZeroVector;
			// Foot lock spring states
			FVectorSpringState PlantOffsetTranslationSpringState;
			FQuaternionSpringState PlantOffsetRotationSpringState;
			// Ground alignment spring states
			FFloatSpringState GroundHeightSpringState;
			FQuaternionSpringState GroundRotationSpringState;
		} Interpolation;
	};

	struct FPlantRuntimeSettings
	{
		float UnplantRadiusSqrd = 0.0f;
		float ReplantRadiusSqrd = 0.0f;
		float CosHalfUnplantAngle = 0.0f;
		float CosHalfReplantAngle = 0.0f;
	};

	struct FPelvisRuntimeData
	{
		/* Bone IDs */
		struct FBones
		{
			FCompactPoseBoneIndex FkBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
			FCompactPoseBoneIndex IkBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
		} Bones;

		/* Settings-based properties */
		float MaxOffsetSqrd = 0.0f;

		/* Input pose properties */
		struct FInputPoseData
		{
			FTransform FKTransformCS = FTransform::Identity;
			FTransform IKRootTransformCS = FTransform::Identity;
			FTransform RootTransformCS = FTransform::Identity;
			FVector FootMidpointCS = FVector::ZeroVector;
		} InputPose;

		/* Interpolation */
		struct FInterpolationData
		{
			// Current pelvis offset and spring states. We use a 3d vector because this interpolates weight rebalancing too.
			FVector PelvisTranslationOffset = FVector::ZeroVector;
			FVectorSpringState PelvisTranslationSpringState;
		} Interpolation;


		float DisablePelvis = 0.0f;
	};

	struct FCharacterData
	{
		FTransform ComponentTransformWS = FTransform::Identity;
		FVector ComponentMoveDeltaWS = FVector::ZeroVector;
		FVector CharacterVelocityWS = FVector::ZeroVector;
		FVector SmoothCapsuleGroundNormalWS = FVector::ZeroVector;
		FQuaternionSpringState SmoothCapsuleGroundNormalSpringState;
		bool bIsOnGround = false;
	};

	// Final result after post-adjustments (extension checks, heel lift, etc.)
	struct FPlantResult
	{
	public:
		FBoneTransform FootTranformCS;
		// @TODO: Add procedural toe rolling
		//FBoneTransform BallTransformCS;
		// @TODO: Look into shifting/rotating the hips to prevent over-extension, and give IK an easier time.
		//FBoneTransform HipTransformCS;
	};

	#if ENABLE_ANIM_DEBUG
	struct FDebugData
	{
		FVector OutputPelvisLocationWS = FVector::ZeroVector;
		FVector InputPelvisLocationWS = FVector::ZeroVector;

		TArray<FVector> OutputFootLocationsWS;
		TArray<FVector> InputFootLocationsWS;

		struct FLegInfo
		{
			float HyperExtensionAmount;
			float RollAmount;
			float PullAmount;
			float DistanceToSeparatingPlane;
		};
		TArray<FLegInfo> LegsInfo;

		void Init(const int32 InSize)
		{
			OutputFootLocationsWS.SetNumUninitialized(InSize);
			InputFootLocationsWS.SetNumUninitialized(InSize);
			LegsInfo.SetNumUninitialized(InSize);
		}
	};
	#endif

	struct FEvaluationContext;
}

USTRUCT(BlueprintInternalUseOnly, Experimental)
struct FGarAnimNode_FootPlacement : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

public:

	// Foot/Ball speed evaluation mode (Graph or Manual) used to decide when the feet are locked
	// Graph mode uses the root motion attribute from the animations to calculate the joint's speed
	// Manual mode uses a per-foot curve name representing the joint's speed
	UPROPERTY(EditAnywhere, Category = "Settings")
	EWarpingEvaluationMode PlantSpeedMode = EWarpingEvaluationMode::Manual;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FBoneReference IKFootRootBone;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FBoneReference PelvisBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (PinHiddenByDefault))
	FFootPlacementPelvisSettings PelvisSettings;

	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<FFootPlacemenLegDefinition> LegDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (PinHiddenByDefault))
	FFootPlacementPlantSettings PlantSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (PinHiddenByDefault))
	FFootPlacementInterpolationSettings InterpolationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (PinHiddenByDefault))
	FFootPlacementTraceSettings TraceSettings;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	FVector BaseTranslationDelta = FVector::ZeroVector;

public:
	UE_API FGarAnimNode_FootPlacement();

	// FAnimNode_Base interface
	UE_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	UE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	UE_API virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	UE_API virtual void EvaluateSkeletalControl_AnyThread(
		FComponentSpacePoseContext& Output,
		TArray<FBoneTransform>& OutBoneTransforms) override;
	UE_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase

private:
	// FAnimNode_SkeletalControlBase interface
	UE_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	// Gather raw or trivially calculated values from input pose
	void GatherPelvisDataFromInputs(const GAR::Anim::FootPlacement::FEvaluationContext& Context);
	void GatherLegDataFromInputs(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		GAR::Anim::FootPlacement::FLegRuntimeData& LegData,
		const FFootPlacemenLegDefinition& LegDef);

	void CalculateFootMidpoint(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		TConstArrayView<GAR::Anim::FootPlacement::FLegRuntimeData> LegData,
		FVector& OutMidpoint) const;

	// Calculate procedural adjustments before solving the desired pelvis position
	void ProcessCharacterState(const GAR::Anim::FootPlacement::FEvaluationContext& Context);
	void ProcessFootAlignment(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		GAR::Anim::FootPlacement::FLegRuntimeData& LegData);

	// Calculate the desired pelvis offset, based on procedural character/foot adjustments
	FTransform SolvePelvis(const GAR::Anim::FootPlacement::FEvaluationContext& Context);

	FTransform UpdatePelvisInterpolationRootSpace(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const FTransform& TargetPelvisTransform);

	// Post-processing adjustments + fix hyper-extension/compression
	GAR::Anim::FootPlacement::FPlantResult FinalizeFootAlignment(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		GAR::Anim::FootPlacement::FLegRuntimeData& LegData,
		const FFootPlacemenLegDefinition& LegDef,
		const FTransform& PelvisTransformCS);

	FVector GetApproachDirWS(const FAnimationBaseContext& Context) const;

	const FTransform& GetRootToComponent() const;

private:
	float CachedDeltaTime = 0.0f;
	FVector LastComponentLocation = FVector::ZeroVector;

	TArray<GAR::Anim::FootPlacement::FLegRuntimeData> LegsData;
	GAR::Anim::FootPlacement::FPlantRuntimeSettings PlantRuntimeSettings;
	GAR::Anim::FootPlacement::FPelvisRuntimeData PelvisData;
	GAR::Anim::FootPlacement::FCharacterData CharacterData;

	// Whether we want to plant, independently from any dynamic pose adjustments we may do
	bool WantsToPlant(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const GAR::Anim::FootPlacement::FLegRuntimeData::FInputPoseData& LegInputPose) const;

	// Get Alignment Alpha based on current foot speed
	// 0.0 is fully unaligned and the foot is in flight.
	// 1.0 is fully aligned and the foot is planted.
	float GetAlignmentAlpha(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const GAR::Anim::FootPlacement::FLegRuntimeData::FInputPoseData& LegInputPose) const;

	// This function looks at both the foot bone and the ball bone, returning the smallest distance to the
	// planting plane. Note this distance can be negative, meaning it's penetrating.
	float CalcTargetPlantPlaneDistance(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const GAR::Anim::FootPlacement::FLegRuntimeData::FInputPoseData& LegInputPose) const;

	struct FPelvisOffsetRangeForLimb
	{
		float MaxExtension;
		float MinExtension;
		float DesiredExtension;
	};

	// Find the horizontal pelvis offset range for the foot to reach:
	void FindPelvisOffsetRangeForLimb(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const GAR::Anim::FootPlacement::FLegRuntimeData& LegData,
		const FVector& PlantTargetLocationCS,
		const FTransform& PelvisTransformCS,
		FPelvisOffsetRangeForLimb& OutPelvisOffsetRangeCS) const;

	// Adjust LastPlantTransformWS to current, to have the foot pivot around the ball instead of the ankle
	FTransform GetFootPivotAroundBallWS(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const GAR::Anim::FootPlacement::FLegRuntimeData::FInputPoseData& LegInputPose,
		const FTransform& LastPlantTransformWS) const;

	// Align the transform the provided world space ground plant plane.
	// Also outputs the twist along the ground plane needed to get there
	void AlignPlantToGround(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const FPlane& PlantPlaneWS,
		const GAR::Anim::FootPlacement::FLegRuntimeData::FInputPoseData& LegInputPose,
		FTransform& InOutFootTransformWS,
		FQuat& OutTwistCorrection) const;

	// Handles horizontal interpolation when unlocking the plant
	FTransform UpdatePlantOffsetInterpolation(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		GAR::Anim::FootPlacement::FLegRuntimeData::FInterpolationData& InOutInterpData) const;

	// Handles the interpolation of the planting plane. Because the plant transform is specified with respect to the 
	// planting plane, it cannot change abruptly without causing an animation pop. It must be interpolated instead.
	void UpdatePlantingPlaneInterpolation(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const FTransform& FootTransformWS,
		const FTransform& LastAlignedFootTransform,
		const float AlignmentAlpha,
		FPlane& InOutPlantPlane,
		const GAR::Anim::FootPlacement::FLegRuntimeData::FInputPoseData& LegInputPose,
		GAR::Anim::FootPlacement::FLegRuntimeData::FInterpolationData& InOutInterpData) const;

	// Checks unplanting and replanting conditions to determine if the foot is planted
	void DeterminePlantType(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const FTransform& FKTransformWS,
		const FTransform& CurrentBoneTransformWS,
		GAR::Anim::FootPlacement::FLegRuntimeData::FPlantData& InOutPlantData,
		const GAR::Anim::FootPlacement::FLegRuntimeData::FInputPoseData& LegInputPose) const;

	float GetMaxLimbExtension(const float DesiredExtension, const float LimbLength) const;
	float GetMinLimbExtension(const float DesiredExtension, const float LimbLength) const;

	void ResetRuntimeData();


	#if ENABLE_FOOTPLACEMENT_DEBUG
	GAR::Anim::FootPlacement::FDebugData DebugData;

	void DrawDebug(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const GAR::Anim::FootPlacement::FLegRuntimeData& LegData,
		const GAR::Anim::FootPlacement::FPlantResult& PlantResult) const;

	void DrawVLog(
		const GAR::Anim::FootPlacement::FEvaluationContext& Context,
		const GAR::Anim::FootPlacement::FLegRuntimeData& LegData,
		const GAR::Anim::FootPlacement::FPlantResult& PlantResult) const;
	#endif

	bool bIsFirstUpdate = false;
	FGraphTraversalCounter UpdateCounter;
};

#undef UE_API
