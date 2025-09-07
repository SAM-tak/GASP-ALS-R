#include "Modifiers/GarAnimationModifier_CalculateRotationYawSpeed.h"

#include "Animation/AnimSequence.h"
#include "Settings/GarBoneNameTable.h"
#include "GarConstants.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimationModifier_CalculateRotationYawSpeed)

void UGarAnimationModifier_CalculateRotationYawSpeed::OnApply_Implementation(UAnimSequence* Sequence)
{
	Super::OnApply_Implementation(Sequence);

	if (UAnimationBlueprintLibrary::DoesCurveExist(Sequence, UGarConstants::RotationYawSpeedCurveName(), ERawCurveTrackTypes::RCT_Float))
	{
		UAnimationBlueprintLibrary::RemoveCurve(Sequence, UGarConstants::RotationYawSpeedCurveName());
	}

	UAnimationBlueprintLibrary::AddCurve(Sequence, UGarConstants::RotationYawSpeedCurveName());

	const auto* DataModel{Sequence->GetDataModel()};
	const auto FrameRate{Sequence->GetSamplingFrameRate().AsDecimal()};

	UAnimationBlueprintLibrary::AddFloatCurveKey(Sequence, UGarConstants::RotationYawSpeedCurveName(), 0.0f, 0.0f);

	for (int32 Index = 1; Index < Sequence->GetNumberOfSampledKeys(); Index++)
	{
		auto CurrentPoseTransform{
			DataModel->GetBoneTrackTransform(BoneNameTable->RootBoneName, Index + (Sequence->RateScale >= 0.0f ? -1 : 0))
		};

		auto NextPoseTransform{
			DataModel->GetBoneTrackTransform(BoneNameTable->RootBoneName, Index + (Sequence->RateScale >= 0.0f ? 0 : -1))
		};

		UAnimationBlueprintLibrary::AddFloatCurveKey(Sequence, UGarConstants::RotationYawSpeedCurveName(), Sequence->GetTimeAtFrame(Index),
		                                             UE_REAL_TO_FLOAT(
			                                             (NextPoseTransform.Rotator().Yaw - CurrentPoseTransform.Rotator().Yaw) *
			                                             FMath::Abs(Sequence->RateScale) * FrameRate));
	}
}
