// Copyright Epic Games, Inc. All Rights Reserved.

#include "GarSmoothWalkingState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarSmoothWalkingState)

namespace GarSmoothWalkingStateErrorTolerance
{
	constexpr float VelocityErrorTolerance = 10.f;
	constexpr float AngularVelocityErrorTolerance = 10.f;
	constexpr float AccelerationErrorTolerance = 50.f;
	constexpr float FacingDegreeErrorTolerance = 10.0f;
}

UScriptStruct* FGarSmoothWalkingState::GetScriptStruct() const
{
	return StaticStruct();
}

FMoverDataStructBase* FGarSmoothWalkingState::Clone() const
{
	return new FGarSmoothWalkingState(*this);
}

bool FGarSmoothWalkingState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bSuccess = Super::NetSerialize(Ar, Map, bOutSuccess);

	Ar << SpringVelocity;
	Ar << SpringAcceleration;
	Ar << IntermediateVelocity;
	Ar << IntermediateFacing;
	Ar << IntermediateAngularVelocity;

	return bSuccess;
}

void FGarSmoothWalkingState::ToString(FAnsiStringBuilderBase& Out) const
{
	Super::ToString(Out);

	Out.Appendf("SpringVelocity=%s SpringAcceleration=%s IntVel=%s IntFac=%s IntAng=%s\n",
		*SpringVelocity.ToCompactString(),
		*SpringAcceleration.ToCompactString(),
		*IntermediateVelocity.ToCompactString(),
		*IntermediateFacing.ToString(),
		*IntermediateAngularVelocity.ToString());
}

bool FGarSmoothWalkingState::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	const FGarSmoothWalkingState* AuthoritySpringState = static_cast<const FGarSmoothWalkingState*>(&AuthorityState);

	return (!(SpringVelocity - AuthoritySpringState->SpringVelocity).IsNearlyZero(GarSmoothWalkingStateErrorTolerance::VelocityErrorTolerance) ||
			!(SpringAcceleration - AuthoritySpringState->SpringAcceleration).IsNearlyZero(GarSmoothWalkingStateErrorTolerance::AccelerationErrorTolerance) ||
			!(IntermediateVelocity - AuthoritySpringState->IntermediateVelocity).IsNearlyZero(GarSmoothWalkingStateErrorTolerance::VelocityErrorTolerance) ||
			 (IntermediateFacing.AngularDistance(AuthoritySpringState->IntermediateFacing) > GarSmoothWalkingStateErrorTolerance::FacingDegreeErrorTolerance ||
			!(IntermediateAngularVelocity - AuthoritySpringState->IntermediateAngularVelocity).IsNearlyZero(GarSmoothWalkingStateErrorTolerance::AngularVelocityErrorTolerance)));
}

void FGarSmoothWalkingState::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	const FGarSmoothWalkingState* FromState = static_cast<const FGarSmoothWalkingState*>(&From);
	const FGarSmoothWalkingState* ToState   = static_cast<const FGarSmoothWalkingState*>(&To);

	SpringVelocity = FMath::Lerp(FromState->SpringVelocity, ToState->SpringVelocity, Pct);
	SpringAcceleration = FMath::Lerp(FromState->SpringAcceleration, ToState->SpringAcceleration, Pct);
	IntermediateVelocity = FMath::Lerp(FromState->IntermediateVelocity, ToState->IntermediateVelocity, Pct);
	IntermediateFacing = FQuat::Slerp(FromState->IntermediateFacing, ToState->IntermediateFacing, Pct);
	IntermediateAngularVelocity = FMath::Lerp(FromState->IntermediateAngularVelocity, ToState->IntermediateAngularVelocity, Pct);
}
