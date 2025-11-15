#include "LayeredMoves/GarLayeredMove_TurnTo.h"

#include "MoveLibrary/MovementUtils.h"
#include "GarCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarLayeredMove_TurnTo)

FGarLayeredMove_TurnTo::FGarLayeredMove_TurnTo()
	: StartRotation(ForceInitToZero)
	, TargetRotation(ForceInitToZero)
	, TimeMappingCurve(nullptr)
{
	DurationMs = 1000.f;
	MixMode = EMoveMixMode::OverrideVelocity;
}

float FGarLayeredMove_TurnTo::EvaluateFloatCurveAtFraction(const UCurveFloat& Curve, const float Fraction) const
{
	float MinCurveTime(0.f);
	float MaxCurveTime(1.f);

	Curve.GetTimeRange(MinCurveTime, MaxCurveTime);
	return Curve.GetFloatValue(FMath::GetRangeValue(FVector2f(MinCurveTime, MaxCurveTime), Fraction));
}

bool FGarLayeredMove_TurnTo::GenerateMove(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, const UMoverComponent* MoverComp,
	UMoverBlackboard* SimBlackboard, FProposedMove& OutProposedMove)
{
	OutProposedMove.MixMode = MixMode;

	const float DeltaSeconds = TimeStep.StepMs / 1000.f;

	float MoveFraction = (TimeStep.BaseSimTimeMs - StartSimTimeMs) / DurationMs;

	if (TimeMappingCurve)
	{
		MoveFraction = EvaluateFloatCurveAtFraction(*TimeMappingCurve, MoveFraction);
	}

	const AActor* MoverActor = MoverComp->GetOwner();

	FRotator CurrentTargetRotation = FMath::Lerp<FRotator, float>(StartRotation, TargetRotation, MoveFraction);

	const FRotator CurrentRotation = MoverActor->GetActorRotation();

	OutProposedMove.AngularVelocityDegrees = UMovementUtils::ComputeAngularVelocityDegrees(CurrentRotation, CurrentTargetRotation, DeltaSeconds);

	return true;
}

FLayeredMoveBase* FGarLayeredMove_TurnTo::Clone() const
{
	FGarLayeredMove_TurnTo* CopyPtr = new FGarLayeredMove_TurnTo(*this);
	return CopyPtr;
}

void FGarLayeredMove_TurnTo::NetSerialize(FArchive& Ar)
{
	Super::NetSerialize(Ar);

	Ar << StartRotation;
	Ar << TargetRotation;
}

UScriptStruct* FGarLayeredMove_TurnTo::GetScriptStruct() const
{
	return FGarLayeredMove_TurnTo::StaticStruct();
}

FString FGarLayeredMove_TurnTo::ToSimpleString() const
{
	return FString::Printf(TEXT("Move To"));
}

void FGarLayeredMove_TurnTo::AddReferencedObjects(FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(Collector);
}
