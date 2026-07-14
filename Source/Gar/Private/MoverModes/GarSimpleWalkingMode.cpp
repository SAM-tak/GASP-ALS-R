// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModes/GarSimpleWalkingMode.h"

#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "MoveLibrary/RollbackBlackboardLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarSimpleWalkingMode)

// エンジン側 USimpleWalkingMode の "DidGenerateMove" と衝突しないよう Gar 固有名を使用
const FName UGarSimpleWalkingMode::DidGenerateMoveEntry = TEXT("GarDidGenerateMove");

void UGarSimpleWalkingMode::OnRegistered(const FName ModeName, const FMoverSimContext& SimContext)
{
	Super::OnRegistered(ModeName, SimContext);

	URollbackBlackboard::EntrySettings DidGenerateMoveEntrySettings = URollbackBlackboardLibrary::MakeSingleFrameEntrySettings();
	DidGenerateMoveEntrySettings.PersistencePolicy = EBlackboardPersistencePolicy::ThroughNextFrame;

	SimContext.Blackboard.CreateEntry<bool>(DidGenerateMoveEntry, DidGenerateMoveEntrySettings);
}

void UGarSimpleWalkingMode::GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const FMoverDefaultSyncState* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();

	if (CommonLegacySettings.Get() == nullptr || !StartingSyncState)
	{
		return;
	}

	const float DeltaSeconds = TimeStep.StepMs * 0.001f;
	if (DeltaSeconds <= FLT_EPSILON)
	{
		return;
	}

	FVector DesiredVelocity;
	EMoveInputType MoveInputType;
	FVector DesiredFacingDir;

	if (const FCharacterDefaultInputs* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>())
	{
		DesiredVelocity = CharacterInputs->GetMoveInput_WorldSpace();
		MoveInputType = CharacterInputs->GetMoveInputType();
		DesiredFacingDir = CharacterInputs->GetOrientationIntentDir_WorldSpace();
	}
	else
	{
		DesiredVelocity = StartingSyncState->GetIntent_WorldSpace();
		MoveInputType = EMoveInputType::DirectionalIntent;
		DesiredFacingDir = StartingSyncState->GetOrientation_WorldSpace().Quaternion().GetForwardVector();
	}

	float MaxMoveSpeed = MaxSpeedOverride >= 0.0f ? MaxSpeedOverride : CommonLegacySettings->MaxSpeed;

	float DesiredVelMag = DesiredVelocity.Length();
	DesiredVelocity -= DesiredVelocity.ProjectOnTo(GetMoverComponent()->GetUpDirection());
	float DesiredVel2DSquaredLength = DesiredVelocity.SquaredLength();
	if (DesiredVel2DSquaredLength > 0.0f)
	{
		DesiredVelocity *= DesiredVelMag / FMath::Sqrt(DesiredVel2DSquaredLength);
	}

	switch (MoveInputType)
	{
	case EMoveInputType::DirectionalIntent:
		{
			OutProposedMove.DirectionIntent = DesiredVelocity;
			DesiredVelocity *= MaxMoveSpeed;
		}
		break;
	case EMoveInputType::Velocity:
		{
			DesiredVelocity = DesiredVelocity.GetClampedToMaxSize(MaxMoveSpeed);
			OutProposedMove.DirectionIntent = MaxMoveSpeed > UE_KINDA_SMALL_NUMBER ? DesiredVelocity / MaxMoveSpeed : FVector::ZeroVector;
		}
		break;
	case EMoveInputType::None:
	case EMoveInputType::Invalid:
	default:
		{
			DesiredVelocity = FVector::ZeroVector;
			OutProposedMove.DirectionIntent = FVector::ZeroVector;
		}
		break;
	}

	OutProposedMove.bHasDirIntent = !OutProposedMove.DirectionIntent.IsNearlyZero();
	DesiredFacingDir -= DesiredFacingDir.ProjectOnTo(GetMoverComponent()->GetUpDirection());
	FQuat CurrentFacing = StartingSyncState->GetOrientation_WorldSpace().Quaternion();
	FQuat DesiredFacing = CurrentFacing;

	if (DesiredFacingDir.Normalize())
	{
		DesiredFacing = FQuat::FindBetween(FVector::ForwardVector, DesiredFacingDir);
	}

	OutProposedMove.LinearVelocity = StartingSyncState->GetVelocity_WorldSpace();
	FVector AngularVelocityDegrees = StartingSyncState->GetAngularVelocityDegrees_WorldSpace();

	UGarSimpleWalkingMode* MutableSelf = const_cast<UGarSimpleWalkingMode*>(this);
	MutableSelf->GenerateWalkMove(const_cast<FMoverTickStartData&>(StartState), DeltaSeconds, SimContext, DesiredVelocity, DesiredFacing, CurrentFacing, AngularVelocityDegrees, OutProposedMove.LinearVelocity);

	OutProposedMove.AngularVelocityDegrees = AngularVelocityDegrees;

	SimContext.Blackboard.TrySet(DidGenerateMoveEntry, true);
}

void UGarSimpleWalkingMode::GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds, const FMoverSimContext& SimContext,
	const FVector& DesiredVelocity, const FQuat& DesiredFacing, const FQuat& CurrentFacing, FVector& InOutAngularVelocityDegrees, FVector& InOutVelocity)
{
	InOutVelocity = DesiredVelocity;

	FQuat ToFacing = CurrentFacing.Inverse() * DesiredFacing;
	InOutAngularVelocityDegrees = DeltaSeconds > 0.0f ? FMath::RadiansToDegrees(ToFacing.ToRotationVector() / DeltaSeconds) : FVector::ZeroVector;
}
