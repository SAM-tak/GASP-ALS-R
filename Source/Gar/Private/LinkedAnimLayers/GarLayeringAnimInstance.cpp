#include "LinkedAnimLayers/GarLayeringAnimInstance.h"
#include "GarAnimationInstance.h"
#include "GarAnimationInstanceProxy.h"
#include "GarCharacter.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarLayeringAnimInstance)

namespace UGarLayeringAnimInstanceStatic
{
	static const float GetValueByCurves(const TMap<FName, float>& Curves, const FName& CurveName)
	{
		const auto* Value{Curves.Find(CurveName)};
		return Value != nullptr ? *Value : 0.0f;
	}
}

void UGarLayeringAnimInstance::Refresh()
{
	using namespace UGarLayeringAnimInstanceStatic;
	const auto& Curves{GetAnimationCurvesFromProxy(EAnimCurveType::AttributeCurve)};

	HeadBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerHeadCurveName());
	HeadAdditiveBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerHeadAdditiveCurveName());
	HeadSlotBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerHeadSlotCurveName());

	// The mesh space blend will always be 1 unless the local space blend is 1.

	ArmLeftBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerArmLeftCurveName());
	ArmLeftAdditiveBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerArmLeftAdditiveCurveName());
	ArmLeftSlotBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerArmLeftSlotCurveName());
	ArmLeftLocalSpaceBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerArmLeftLocalSpaceCurveName());
	ArmLeftMeshSpaceBlendAmount = !FAnimWeight::IsFullWeight(ArmLeftLocalSpaceBlendAmount);

	// The mesh space blend will always be 1 unless the local space blend is 1.

	ArmRightBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerArmRightCurveName());
	ArmRightAdditiveBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerArmRightAdditiveCurveName());
	ArmRightSlotBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerArmRightSlotCurveName());
	ArmRightLocalSpaceBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerArmRightLocalSpaceCurveName());
	ArmRightMeshSpaceBlendAmount = !FAnimWeight::IsFullWeight(ArmRightLocalSpaceBlendAmount);

	HandLeftBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerHandLeftCurveName());
	HandRightBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerHandRightCurveName());

	SpineBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerSpineCurveName());
	SpineAdditiveBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerSpineAdditiveCurveName());
	SpineSlotBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerSpineSlotCurveName());

	PelvisBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerPelvisCurveName());
	PelvisSlotBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerPelvisSlotCurveName());

	LegsBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerLegsCurveName());
	LegsSlotBlendAmount = GetValueByCurves(Curves, UGarConstants::LayerLegsSlotCurveName());
}
