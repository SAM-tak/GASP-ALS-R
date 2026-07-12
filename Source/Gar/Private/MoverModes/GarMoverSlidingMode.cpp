// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoverModes/GarMoverSlidingMode.h"

#include "GarCharacterMoverComponent.h"
#include "GarGameplayTags.h"
#include "MoverComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarMoverSlidingMode)

UGarMoverSlidingMode::UGarMoverSlidingMode()
{
	GameplayTags.Reset();
	GameplayTags.AddTag(GarLocomotionModeTags::Grounded);
	GameplayTags.AddTag(GarLocomotionActionTags::Sliding);
}

void UGarMoverSlidingMode::OnRegistered(const FName ModeName, const FMoverSimContext& SimContext)
{
	Super::OnRegistered(ModeName, SimContext);
}

void UGarMoverSlidingMode::OnUnregistered(const FMoverSimContext& SimContext)
{
	Super::OnUnregistered(SimContext);
}

void UGarMoverSlidingMode::Activate(const FMoverEventContext& Context, FName PrevModeName, const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, FMoverSyncState* OutSyncState, FMoverAuxStateContext* OutAuxState)
{
	bInitialBoost = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InitialBoostTimerHandle,
			this, &UGarMoverSlidingMode::OnInitialBoostExpired,
			static_cast<float>(InitialBoostTime),
			false);
	}

	Super::Activate(Context, PrevModeName, SimContext, StartState, OutSyncState, OutAuxState);
}

void UGarMoverSlidingMode::Deactivate(const FMoverEventContext& Context, FName NextModeName, const FMoverSimContext& SimContext)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialBoostTimerHandle);
	}
	bInitialBoost = false;

	Super::Deactivate(Context, NextModeName, SimContext);
}

void UGarMoverSlidingMode::OnInitialBoostExpired()
{
	bInitialBoost = false;
}

void UGarMoverSlidingMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	Super::SimulationTick_Implementation(Params, OutputState);

	if (OutputState.MovementEndState.NextModeName != NAME_None)
	{
		return;
	}

	const UMoverComponent* MoverComp = GetMoverComponent();
	const UGarCharacterMoverComponent* GarMoverComp = Cast<UGarCharacterMoverComponent>(MoverComp);

	FHitResult FloorHit;
	const bool bHasFloor = IsValid(MoverComp) && MoverComp->TryGetFloorCheckHitResult(FloorHit);
	const bool bIsGrounded = bHasFloor && (!IsValid(GarMoverComp) || GarMoverComp->IsWalkable(FloorHit));
	if (!bIsGrounded)
	{
		OutputState.MovementEndState.NextModeName = TEXT("Falling");
		return;
	}

	const FVector LinearVelocity2D(Params.ProposedMove.LinearVelocity.X, Params.ProposedMove.LinearVelocity.Y, 0.0f);
	if (LinearVelocity2D.Size() <= ExitToWalkingSpeedThreshold)
	{
		OutputState.MovementEndState.NextModeName = TEXT("Walking");
	}
}

void UGarMoverSlidingMode::GenerateWalkMove_Implementation(
	FMoverTickStartData& StartState,
	float DeltaSeconds,
	const FMoverSimContext& SimContext,
	const FVector& DesiredVelocity,
	const FQuat& DesiredFacing,
	const FQuat& CurrentFacing,
	FVector& InOutAngularVelocityDegrees,
	FVector& InOutVelocity)
{
	// スロープ角を先に計算し、今フレームの Super 計算に反映させる
	// BPと同様に移動方向に対する符号付き角度: 上り坂=負、下り坂=正、平地=0

	FHitResult FloorHit;
	const bool bHasFloor = GetMoverComponent()->TryGetFloorCheckHitResult(FloorHit);

	float SlopeAngle = 0.0f;
	if (bHasFloor)
	{
		const FVector VelDir = DesiredVelocity.GetSafeNormal();
		if (!VelDir.IsNearlyZero(UE_KINDA_SMALL_NUMBER))
		{
			const float DotVal = FVector::DotProduct(FloorHit.Normal.GetSafeNormal(), VelDir);
			SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(DotVal)) - 90.0f;
		}
	}

	// InitialBoost フェーズ
	if (bInitialBoost)
	{
		MaxSpeedOverride = static_cast<float>(InitialBoostSpeed);
		Acceleration     = static_cast<float>(InitialBoostAcceleration);
	}
	else
	{
		// SlopeAngle > -ShallowSlopeAngle は平地扱い
		if (SlopeAngle > static_cast<float>(-ShallowSlopeAngle))
		{
			MaxSpeedOverride = static_cast<float>(FlatGroundSpeed);
		}
		else
		{
			MaxSpeedOverride = static_cast<float>(FMath::GetMappedRangeValueClamped(
				FVector2D(-ShallowSlopeAngle, -SteepSlopeAngle),
				FVector2D(ShallowSlopeSpeed, SteepSlopeSpeed),
				static_cast<double>(SlopeAngle)));
		}
		Acceleration = static_cast<float>(AfterBoostAcceleration);
	}

	// 傾斜によるDeceleration (上り坂ほど制動が強い: SteepSlopeDecel、下り坂は FlatGroundDecel)
	Deceleration = static_cast<float>(FMath::GetMappedRangeValueClamped(
		FVector2D(ShallowSlopeAngle, SteepSlopeAngle),
		FVector2D(FlatGroundDeceleration, SteepSlopeDeceleration),
		static_cast<double>(SlopeAngle)));

	// 曲げ処理: 入力の直交成分のみで DesiredVelocity の方向を調整（速さは維持）
	// 平行成分は無視 → スライディング中に前後入力で加速/減速しない
	FVector ModifiedDesiredVelocity = DesiredVelocity;
	const FVector CurrentVel2D(InOutVelocity.X, InOutVelocity.Y, 0.0f);
	const float CurrentSpeed2D = CurrentVel2D.Size();
	const float DesiredSpeed2D = FVector(DesiredVelocity.X, DesiredVelocity.Y, 0.0f).Size();
	if (CurrentSpeed2D > UE_KINDA_SMALL_NUMBER && DesiredSpeed2D > UE_KINDA_SMALL_NUMBER)
	{
		const FVector CurrentDir = CurrentVel2D / CurrentSpeed2D;
		const FVector InputDir2D = FVector(DesiredVelocity.X, DesiredVelocity.Y, 0.0f) / DesiredSpeed2D;
		// InputDir の CurrentDir に対する直交成分
		const FVector PerpInput = InputDir2D - FVector::DotProduct(InputDir2D, CurrentDir) * CurrentDir;
		// 現在方向 + 直交成分 → 新しい方向（速さはそのまま）
		const FVector SteerDir = (CurrentDir + PerpInput * static_cast<float>(SteeringStrength)).GetSafeNormal2D();
		if (!SteerDir.IsNearlyZero(UE_KINDA_SMALL_NUMBER))
		{
			ModifiedDesiredVelocity = FVector(SteerDir.X * DesiredSpeed2D, SteerDir.Y * DesiredSpeed2D, DesiredVelocity.Z);
		}
	}

	Super::GenerateWalkMove_Implementation(
		StartState, DeltaSeconds, SimContext,
		ModifiedDesiredVelocity, DesiredFacing, CurrentFacing,
		InOutAngularVelocityDegrees, InOutVelocity);
}
