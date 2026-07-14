// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModes/GarMoverWalkingMode.h"

#include "MoverComponent.h"
#include "MoveLibrary/MovementUtils.h"
#include "Settings/GarMovementSettings.h"
#include "State/GarCharacterMoverInputs.h"
#include "GarCharacterMoverComponent.h"
#include "GarGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarMoverWalkingMode)

UGarMoverWalkingMode::UGarMoverWalkingMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// UCommonLegacyMovementSettings は UWalkingMode コンストラクタが追加済み
	SharedSettingsClasses.Add(UGarMovementSettings::StaticClass());

	// UWalkingMode が追加する Mover_IsOnGround を Gar のタグ体系に置き換える
	GameplayTags.Reset();
	GameplayTags.AddTag(GarLocomotionModeTags::Grounded);
}

void UGarMoverWalkingMode::GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const FGarCharacterMoverInputs* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FGarCharacterMoverInputs>();
	const FMoverDefaultSyncState* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	check(StartingSyncState);

	const FGarMovementSpeedSettings* SpeedSettings;
	FRotator IntendedOrientation_WorldSpace;

	if (CharacterInputs)
	{
		SpeedSettings = Settings->GetSpeedSettings(CharacterInputs->Stance, CharacterInputs->Gait);

		// If there's no intent from input to change orientation, use the current orientation
		IntendedOrientation_WorldSpace = CharacterInputs->OrientationIntent.IsNearlyZero()
			? StartingSyncState->GetOrientation_WorldSpace()
			: CharacterInputs->GetOrientationIntentDir_WorldSpace().ToOrientationRotator();
	}
	else
	{
		SpeedSettings = Settings->GetSpeedSettings(FGameplayTag::EmptyTag, FGameplayTag::EmptyTag);
		IntendedOrientation_WorldSpace = StartingSyncState->GetOrientation_WorldSpace();
	}

	IntendedOrientation_WorldSpace = UMovementUtils::ApplyGravityToOrientationIntent(IntendedOrientation_WorldSpace, GetMoverComponent()->GetWorldToGravityTransform(), Settings->bShouldRemainVertical);

	// Stance/Gait 別スピード設定を Smooth Walking 側パラメータに注入。
	// GenerateMove は const だがモード自身の状態を書き換えたいケースのため、エンジンの const_cast パターンを踏襲
	UGarMoverWalkingMode* MutableThis = const_cast<UGarMoverWalkingMode*>(this);
	MutableThis->MaxSpeedOverride = SpeedSettings->GetMaxSpeed(IntendedOrientation_WorldSpace, StartingSyncState->GetOrientation_WorldSpace());
	MutableThis->Acceleration = SpeedSettings->Acceleration;
	MutableThis->Deceleration = SpeedSettings->Deceleration;

	if (UGarCharacterMoverComponent* MoverComp = Cast<UGarCharacterMoverComponent>(GetMoverComponent()))
	{
		MoverComp->CurrentMaxSpeed = SpeedSettings->MaxSpeed;
		MoverComp->CurrentAcceleration = SpeedSettings->Acceleration;
		MoverComp->CurrentDeceleration = SpeedSettings->Deceleration;
	}

	Super::GenerateMove_Implementation(SimContext, StartState, TimeStep, OutProposedMove);
}

void UGarMoverWalkingMode::OnRegistered(const FName ModeName, const FMoverSimContext& SimContext)
{
	Super::OnRegistered(ModeName, SimContext);

	Settings = GetMoverComponent()->FindSharedSettings<UGarMovementSettings>();
	ensureMsgf(Settings, TEXT("Failed to find instance of GarMovementSettings on %s. Movement may not function properly."), *GetPathNameSafe(this));
}

void UGarMoverWalkingMode::OnUnregistered(const FMoverSimContext& SimContext)
{
	Settings = nullptr;

	Super::OnUnregistered(SimContext);
}
