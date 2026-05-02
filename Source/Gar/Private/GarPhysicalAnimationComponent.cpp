// Fill out your copyright notice in the Description page of Project Settings.


#include "GarPhysicalAnimationComponent.h"

#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Curves/CurveFloat.h"
#include "Engine/Canvas.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "ChooserFunctionLibrary.h"
#include "AbilitySystemComponent.h"
#include "GarCharacter.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "GarAnimationInstance.h"
#include "GarLinkedAnimationInstance.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacterMoverComponent.h"
#include "Abilities/Actions/GarGameplayAbility_Ragdolling.h"
#include "LinkedAnimLayers/GarRagdollingAnimInstance.h"
#include "Settings/GarRagdollingSettings.h"
#include "Utility/GarUtility.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarPhysicalAnimationComponent)

bool UGarPhysicalAnimationComponent::IsProfileExist(const FName& ProfileName) const
{
	for (auto Body : GetSkeletalMesh()->Bodies)
	{
		if (USkeletalBodySetup* BodySetup = Cast<USkeletalBodySetup>(Body->BodySetup.Get()))
		{
			if (BodySetup->FindPhysicalAnimationProfile(ProfileName))
			{
				return true;
			}
		}
	}
	return false;
}

bool UGarPhysicalAnimationComponent::HasAnyProfile(const USkeletalBodySetup* BodySetup) const
{
	if (CurrentProfileNames.IsEmpty())
	{
		return bRagdolling && BodySetup->PhysicsType != EPhysicsType::PhysType_Kinematic;
	}

	for (const auto& ProfileName : CurrentProfileNames)
	{
		if (BodySetup->FindPhysicalAnimationProfile(ProfileName))
		{
			return true;
		}
	}

	return false;
}

bool UGarPhysicalAnimationComponent::NeedsProfileChange()
{
	bool bRetVal = CurrentGameplayTags != PreviousGameplayTags;
	PreviousGameplayTags = CurrentGameplayTags;
	return bRetVal;
}

void UGarPhysicalAnimationComponent::ClearGameplayTags()
{
	CurrentGameplayTags.Reset();
	PreviousGameplayTags.Reset();
}

void UGarPhysicalAnimationComponent::RefreshBodyState(float DeltaTime)
{
	auto* Mesh{GetSkeletalMesh()};

	bool bNeedUpdate = bActive;

	if (!bActive && (!CurrentProfileNames.IsEmpty() || bRagdolling))
	{
		for (auto Body : Mesh->Bodies)
		{
			if (USkeletalBodySetup* BodySetup = Cast<USkeletalBodySetup>(Body->BodySetup.Get()))
			{
				if (CurveValues.GetLockedValue(BodySetup->BoneName, CurveBoneMappings) <= 0.0f && HasAnyProfile(BodySetup))
				{
					bNeedUpdate = true;
					break;
				}
			}
		}
	}

	bool bActiveAny = false;

	if (bNeedUpdate)
	{
		for (auto Body : Mesh->Bodies)
		{
			if (USkeletalBodySetup* BodySetup = Cast<USkeletalBodySetup>(Body->BodySetup.Get()))
			{
				float LockedValue{CurveValues.GetLockedValue(BodySetup->BoneName, CurveBoneMappings)};
				if (!FAnimWeight::IsRelevant(LockedValue) && HasAnyProfile(BodySetup))
				{
					bActiveAny = true;
					if (Body->IsInstanceSimulatingPhysics())
					{
						float Speed = 1.0f / FMath::Max(0.000001f, BlendTimeOfBlendWeightOnActivate);
						Body->PhysicsBlendWeight = FMath::Min(1.0f, FMath::FInterpConstantTo(Body->PhysicsBlendWeight, 1.0f, DeltaTime, Speed));
					}
					else
					{
						Body->SetInstanceSimulatePhysics(true);
						Body->PhysicsBlendWeight = 0.0f;
					}
				}
				else
				{
					if (FAnimWeight::IsRelevant(LockedValue))
					{
						if (Body->IsInstanceSimulatingPhysics())
						{
							Body->PhysicsBlendWeight = FMath::FInterpConstantTo(Body->PhysicsBlendWeight,
								FMath::Max(0.0f, 1.0f - LockedValue), DeltaTime, 15.0f);
						}
						else
						{
							if (!FAnimWeight::IsFullWeight(LockedValue))
							{
								Body->SetInstanceSimulatePhysics(true);
							}
							Body->PhysicsBlendWeight = 0.0f;
						}
					}
					else
					{
						float Speed = 1.0f / FMath::Max(0.000001f, BlendTimeOfBlendWeightOnDeactivate);
						Body->PhysicsBlendWeight = FMath::FInterpConstantTo(Body->PhysicsBlendWeight, 0.0f, DeltaTime, Speed);
					}
					if (Body->PhysicsBlendWeight == 0.0f)
					{
						if (Body->IsInstanceSimulatingPhysics())
						{
							Body->SetInstanceSimulatePhysics(false);
						}
					}
					else
					{
						bActiveAny = true;
					}
				}
			}
		}

		// Trick. Calling a no effect method To call skeletal mesh's private method "UpdateEndPhysicsTickRegisteredState" and "UpdateClothTickRegisteredState".
		Mesh->AccumulateAllBodiesBelowPhysicsBlendWeight(NAME_None, 0.0f);
	}
	
	if (bActiveAny && !bActive)
	{
		PrevCollisionObjectType = TEnumAsByte(Mesh->GetCollisionObjectType());
		PrevCollisionEnabled = TEnumAsByte(Mesh->GetCollisionEnabled());
		Mesh->SetCollisionObjectType(ECC_PhysicsBody);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		bActive = true;

		OriginalUpdateMode = Mesh->PhysicsTransformUpdateMode;
		Mesh->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic; // avoid feedback loop
	}

	if (!bActiveAny && bActive)
	{
		Mesh->SetCollisionObjectType(PrevCollisionObjectType);
		Mesh->SetCollisionEnabled(PrevCollisionEnabled);
		bActive = false;
		Mesh->PhysicsTransformUpdateMode = OriginalUpdateMode;
	}
}

void UGarPhysicalAnimationComponent::ChooseProfile_Implementation(FGarPAProfileChooserResult& OutResult) const
{
	if(!IsValid(ProfileChooser)) return;

	auto* Character{Cast<AGarCharacter>(GetOwner())};

	if(!IsValid(Character)) return;

	const FInstancedStruct ChooserInstance = UChooserFunctionLibrary::MakeEvaluateChooser(ProfileChooser);

	FChooserEvaluationContext Context;
	Context.AddStructParam(const_cast<FGameplayTagContainer&>(CurrentGameplayTags));
	Context.AddStructParam(OutResult);

	UChooserFunctionLibrary::EvaluateObjectChooserBase(Context, ChooserInstance, nullptr);
}

void UGarPhysicalAnimationComponent::SelectProfile()
{
	FGarPAProfileChooserResult Choosen;
	ChooseProfile(Choosen);

	if (Choosen.ProfileNames != CurrentProfileNames)
	{
		bool bFirst = true;
		for (const auto& NextProfileName : Choosen.ProfileNames)
		{
			ApplyPhysicalAnimationProfileBelow(NAME_None, NextProfileName);
			GetSkeletalMesh()->SetConstraintProfileForAll(NextProfileName, bFirst);
			bFirst = false;
		}
		CurrentProfileNames = Choosen.ProfileNames;
	}
}

void UGarPhysicalAnimationComponent::OnRegister()
{
	Super::OnRegister();
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	if (IsValid(Character))
	{
		RagdollingState.Character = Character;
	}
}

void UGarPhysicalAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	if (IsValid(Character))
	{
		RagdollingState.RagdollingAnimInstance = Character->GetGarAnimationInstace()->RagdollingAnimInstance.Get();
	}
}

void UGarPhysicalAnimationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};

	// Apply special behaviour when changed Ragdolling state
	
	CurrentRagdolling = FGameplayTag::EmptyTag;
	for(auto& KeyValue : RagdollingSettingsMap)
	{
		if (Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(KeyValue.Key))
		{
			CurrentRagdolling = KeyValue.Key;
			break;
		}
	}

	if (CurrentRagdolling.IsValid())
	{
		if (!bRagdolling)
		{
			bRagdolling = true;

			RagdollingState.Start(RagdollingSettingsMap[CurrentRagdolling], this);
		}

		RagdollingState.Tick(DeltaTime, this);

		if (RagdollingState.bGrounded)
		{
			//Character->SetLocomotionMode(GarLocomotionModeTags::Grounded);
		}
		else
		{
			//Character->SetLocomotionMode(GarLocomotionModeTags::InAir);
		}
	}
	else
	{
		if (bRagdolling)
		{
			bRagdolling = false;

			RagdollingState.End(this);

			CurrentProfileNames.Reset();
			ClearGameplayTags();
			//GetSkeletalMesh()->SetAllBodiesSimulatePhysics(false);
			//GetSkeletalMesh()->SetAllBodiesPhysicsBlendWeight(0.0f);
			//GetSkeletalMesh()->SetConstraintProfileForAll(NAME_None, true);

			if (bActive)
			{
				//GetSkeletalMesh()->SetCollisionObjectType(PrevCollisionObjectType);
				//GetSkeletalMesh()->SetCollisionEnabled(PrevCollisionEnabled);
				bActive = false;
			}
		}
	}

	Character->GetAbilitySystemComponent()->GetOwnedGameplayTags(CurrentGameplayTags);

	CurveValues.Refresh(Character, CurveBoneMappings);

	// Choose Physical Animation Profile

	if (NeedsProfileChange())
	{
		SelectProfile();
	}

	// Update PhysicsBlendWeight and Collision settings

	if (!bRagdolling || !RagdollingState.bFreezing)
	{
		RefreshBodyState(DeltaTime);
	}

	// workaround for crash since 5.6
	const TArray<FTransform>& SpaceBases = GetSkeletalMesh()->GetEditableComponentSpaceTransforms();
	if (SpaceBases.IsEmpty())
	{
		return;
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// SimProxy: Mover の VisualComponentOffset スムージングでメッシュが移動した後、
	// Super の UpdateTargetActors(None) はキネマティックターゲットを新しいメッシュ位置に
	// 設定するが、シミュレーション中のボディは物理ソルバが動くまで旧位置に残る。
	// TeleportPhysics で呼び直すことでボディを即座にターゲット位置にスナップする。
	// ただしラグドール中は PhysicsBlendWeight < 1.0 のブレンド期間にボディを
	// アニメ+物理ブレンド位置へテレポートして物理シミュレーションと干渉するため除外する。
	// AutonomousProxy でもトラバーサル中は MotionWarping による高速移動で通常の追従が
	// 間に合わないため、同様に TeleportPhysics でスナップする。
	if (Character && !bRagdolling)
	{
		const ENetRole LocalRole = Character->GetLocalRole();
		const bool bSimProxy = (LocalRole == ROLE_SimulatedProxy);
		const bool bApDuringTraversal = (LocalRole == ROLE_AutonomousProxy)
			&& Character->GetAbilitySystemComponent()
				->HasMatchingGameplayTag(GarLocomotionActionTags::Traversal);

		if (bSimProxy || bApDuringTraversal)
		{
			UpdateTargetActors(ETeleportType::TeleportPhysics);
		}
	}
}

void UGarPhysicalAnimationComponent::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& HorizontalLocation, float& VerticalLocation)
{
	const auto Scale{FMath::Min(Canvas->SizeX / (1280.0f * Canvas->GetDPIScale()), Canvas->SizeY / (720.0f * Canvas->GetDPIScale()))};

	VerticalLocation += 4.0f * Scale;

	FCanvasTextItem Text{
		FVector2D::ZeroVector,
		FText::GetEmpty(),
		GEngine->GetMediumFont(),
		FLinearColor::White
	};

	Text.Scale = {Scale * 0.75f, Scale * 0.75f};
	Text.EnableShadow(FLinearColor::Black);

	const auto RowOffset{12.0f * Scale};
	const auto ColumnOffset{145.0f * Scale};
	
	TStringBuilder<256> DebugStringBuilder;

	for (const auto& ProfileName : CurrentProfileNames)
	{
		DebugStringBuilder.Appendf(TEXT("%s "), *ProfileName.ToString());
	}

	Text.Text = FText::AsCultureInvariant(DebugStringBuilder.ToString());
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;

	for (auto Body : GetSkeletalMesh()->Bodies)
	{
		Text.SetColor(FMath::Lerp(FLinearColor::Gray, FLinearColor::Red, UGarMath::Clamp01(Body->PhysicsBlendWeight)));

		Text.Text = FText::AsCultureInvariant(FString::Printf(TEXT("%s %s %1.2f"), *Body->GetBodySetup()->BoneName.ToString(),
			Body->IsInstanceSimulatingPhysics() ? TEXT("ON") : TEXT("OFF"),
			Body->PhysicsBlendWeight));
		Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

		VerticalLocation += RowOffset;
	}
}

bool UGarPhysicalAnimationComponent::HasRagdollingSettings(const FGameplayTag& Tag) const
{
	return RagdollingSettingsMap.Contains(Tag);
}

bool UGarPhysicalAnimationComponent::IsRagdolling() const
{
	return CurrentRagdolling.IsValid();
}

void UGarPhysicalAnimationComponent::TeleportPhysics()
{
	// UpdateKinematicBonesToAnim(TeleportPhysics) はシミュレーション中ボディにも
	// SetGlobalPose を呼び、アニメーションポーズ位置へ即座にスナップする。
	auto* Mesh{GetSkeletalMesh()};
	if (IsValid(Mesh))
	{
		Mesh->UpdateKinematicBonesToAnim(Mesh->GetComponentSpaceTransforms(), ETeleportType::TeleportPhysics, false);
	}
	// PAC の TargetActors（キネマティックゴールアクター）も新位置に同期。
	UpdateTargetActors(ETeleportType::TeleportPhysics);
}

bool UGarPhysicalAnimationComponent::IsRagdollingAndGroundedAndAged() const
{
	return IsRagdolling() && RagdollingState.IsGroundedAndAged();
}

bool UGarPhysicalAnimationComponent::IsRagdollingFacingUpward() const
{
	return RagdollingState.bFacingUpward;
}

bool UGarPhysicalAnimationComponent::IsBoneUnderSimulation(const FName& BoneName) const
{
	auto* Body = GetSkeletalMesh()->GetBodyInstance(BoneName);
	return Body && Body->IsInstanceSimulatingPhysics();
}

// FGarPhysicalAnimationCurveValues

void FGarPhysicalAnimationCurveValues::Refresh(AGarCharacter *Character, const TArray<FGarPACurveBoneMapping>& Mappings)
{
	auto AnimInstance{Character->GetGarAnimationInstace()};
	if (AnimInstance)
	{
		if(Mappings.Num() != Values.Num())
		{
			Values.Empty();
		}
		for(auto& Mapping : Mappings)
		{
			auto Value = AnimInstance->GetCurveValueClamped01(Mapping.CurveName);
			if(Values.Contains(Mapping.CurveName))
			{
				Values[Mapping.CurveName] = Value;
			}
			else
			{
				Values.Add(Mapping.CurveName, Value);
			}
		}
	}
}

float FGarPhysicalAnimationCurveValues::GetLockedValue(const FName& BoneName, const TArray<FGarPACurveBoneMapping>& Mappings) const
{
	for(auto& Mapping : Mappings)
	{
		if (Mapping.BoneNames.Contains(BoneName))
		{
			if(Values.Contains(Mapping.CurveName))
			{
				return Values[Mapping.CurveName];
			}
			else
			{
				break;
			}
		}
	}
	return 0.0f;
}

// FGarRagdollingState

void FGarRagdollingState::Start(UGarRagdollingSettings* NewSettings, const UGarPhysicalAnimationComponent* PhysicalAnimation)
{
	Settings = NewSettings;

	// ---- AnimInstance の有無に関わらず常に実行する初期化 ----
	// (旧実装では RagdollingAnimInstance が null の場合に early return していたため、
	//  DedicatedServer 上の NPC でカプセルコリジョン・Mover 設定がスキップされていた)

	PullForce = 0.0f;
	ElapsedTime = 0.0f;
	TimeAfterGrounded = TimeAfterGroundedAndStopped = 0.0f;
	bFacingUpward = bGrounded = false;
	bPreviousGrounded = true;
	bFreezing = false;
	bPendingInitialVelocity = false;
	PrevActorLocation = Character->GetActorLocation();

	auto* Mover{Character->GetMover()};

	// Initialize bFacingUpward flag by current movement direction. If Velocity is Zero, it is chosen bFacingUpward is true.
	// And determine target yaw angle of the character.

	const auto PoleDirection = Mover->GetVelocity().GetSafeNormal2D();

	if (PoleDirection.SizeSquared2D() > 0.0)
	{
		bFacingUpward = Character->GetActorForwardVector().Dot(PoleDirection) < -0.25f;
		LyingDownYawAngleDelta = UGarMath::DirectionToAngleXY(bFacingUpward ? -PoleDirection : PoleDirection) - Character->GetActorRotation().Yaw;
	}
	else
	{
		bFacingUpward = true;
		LyingDownYawAngleDelta = 0.0;
	}

	// Stop any active montages (AnimInstance が存在する場合のみ).

	auto* AnimInstance{Character->GetGarAnimationInstace()};
	if (IsValid(AnimInstance))
	{
		AnimInstance->Montage_Stop(Settings->StartBlendTime);
	}

	// Disable movement corrections and reset network smoothing.

	Mover->SmoothingMode = EMoverSmoothingMode::None;
	Mover->bSyncInputsForSimProxy = false;

	// Disable capsule collision. other physics states will be changed by physical aniamtion process

	//Character->GetCapsule()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // TODO: Change
	auto Capsule{Character->GetCapsule()};
	// オブジェクトタイプをPawnに設定（必要に応じてWorldDynamicでも可）
	Capsule->SetCollisionObjectType(ECC_Pawn);

	// 全チャンネルをIgnoreで初期化
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);

	// 地形と動的オブジェクトにはBlock
	Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	// 他のPawnとはOverlap（押しのけられるが干渉しない）
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 必要に応じて Visibility や Camera も Ignore
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Clear the character movement mode and set the locomotion action to ragdolling.

	//Mover->SetMovementMode(MOVE_Custom);
	//Mover->SetMovementModeLocked(true);

	// SimProxy: Mover のレプリケート済み速度を初速として記録。
	// 物理シミュレーション開始後の最初の Tick で TopBone ボディに注入する。
	// CMC 時代の ACharacter::PostNetReceivePhysicState() に相当する処理。
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		InitialRagdollVelocity = Mover->GetVelocity();
		bPendingInitialVelocity = !InitialRagdollVelocity.IsNearlyZero();
	}

	// ---- AnimInstance 依存の初期化 ----

	if (!IsValid(RagdollingAnimInstance))
	{
		return;
	}

	// Ensure freeze flag is off.

	RagdollingAnimInstance->UnFreeze();

	RagdollingAnimInstance->SetStartBlendTime(Settings->StartBlendTime);

	RagdollingAnimInstance->Refresh(*this, true);
}

FVector FGarRagdollingState::TraceGround()
{
	auto* Mover{Character->GetMover()};

	const auto Capsule{Character->GetCapsule()};
	const auto CapsuleHalfHeight{Capsule->GetScaledCapsuleHalfHeight()};

	const auto TraceStart{Character->GetActorLocation()};
	const FVector TraceEnd{TraceStart.X, TraceStart.Y, TraceStart.Z - CapsuleHalfHeight};

	FHitResult Hit;

	Character->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, Capsule->GetCollisionObjectType(),
													{__FUNCTION__, false, Character},
													Capsule->GetCollisionResponseToChannel(Capsule->GetCollisionObjectType()));

	bGrounded = Mover->IsWalkable(Hit);

	return {
		TraceStart.X, TraceStart.Y,
		bGrounded ? Hit.ImpactPoint.Z + CapsuleHalfHeight + UGarCharacterMoverComponent::MIN_FLOOR_DIST : TraceStart.Z
	};
}

bool FGarRagdollingState::IsGroundedAndAged() const
{
	return bGrounded && ElapsedTime > Settings->StartBlendTime;
}

void FGarRagdollingState::Tick(float DeltaTime, const UGarPhysicalAnimationComponent* PhysicalAnimation)
{
	if (bFreezing)
	{
		return;
	}

#if ENABLE_DRAW_DEBUG
	bool bDisplayDebug{UGarUtility::ShouldDisplayDebugForActor(Character, UGarConstants::PADebugDisplayName())};
#endif

	auto* Mover{Character->GetMover()};

	auto NetMode{Character->GetWorld()->GetNetMode()};

	// just for info.
	//Mover->GetVelocity() = FMath::VInterpTo(Mover->GetVelocity(),
	//										DeltaTime > 0.0f ? (Character->GetActorLocation() - PrevActorLocation) / DeltaTime : FVector::Zero(),
	//										DeltaTime, Settings->VelocityInterpolationSpeed);
	PrevActorLocation = Mover->GetUpdatedComponentTransform().GetLocation();

	// Prevent the capsule from going through the ground when the ragdoll is lying on the ground.

	// While we could get rid of the line trace here and just use TargetLocation
	// as the character's location, we don't do that because the camera depends on the
	// capsule's bottom location, so its removal will cause the camera to behave erratically.

	bGrounded = Character->HasMatchingGameplayTag(GarLocomotionModeTags::Grounded);

	// SimProxy: 初速注入 - 物理シミュレーション開始後の最初の Tick で TopBone に適用
	// CMC の PostNetReceivePhysicState() に相当。Mover はレプリケーション経路が別なため手動注入が必要。
	if (bPendingInitialVelocity)
	{
		if (FBodyInstance* TopBody = Character->GetMesh()->GetBodyInstance(PhysicalAnimation->GetTopBoneName()))
		{
			if (TopBody->IsInstanceSimulatingPhysics())
			{
				TopBody->SetLinearVelocity(InitialRagdollVelocity, false);
				bPendingInitialVelocity = false;
			}
		}
	}

	// Clip velocity each body

	if (Settings->MaxBodySpeed > 0.0f)
	{
		for (auto& Body : Character->GetMesh()->Bodies)
		{
			auto Vel{Body->GetUnrealWorldVelocity()};
			Body->SetLinearVelocity(Vel.GetClampedToMaxSize(Settings->MaxBodySpeed) - Vel, true);
		}
	}

	if (IsGroundedAndAged())
	{
		// Determine whether the ragdoll is facing upward or downward.

		const auto TopRotation{Character->GetMesh()->GetBoneTransform(PhysicalAnimation->GetTopBoneName()).Rotator()};

		const auto TopDirDotUp{TopRotation.RotateVector(FVector::RightVector).Dot(FVector::UpVector)};

		if (bFacingUpward)
		{
			if (TopDirDotUp < -0.5f)
			{
				bFacingUpward = false;
			}
		}
		else
		{
			if (TopDirDotUp > 0.5f)
			{
				bFacingUpward = true;
			}
		}
	}

	{
		auto TargetTransform{Character->GetMesh()->GetBoneTransform(PhysicalAnimation->GetTopBoneName())};
		auto TargetDirection{TargetTransform.GetRotation().RotateVector(FVector::RightVector)};
		auto DotY{FVector::UpVector.Dot(TargetDirection)};
		if (FMath::Abs(DotY) > 0.7)
		{
			if (DotY > 0)
			{
				//TargetDirection = TargetTransform.GetRotation().RotateVector(FVector::BackwardVector);
				bFacingUpward = true;
			}
			else
			{
				//TargetDirection = TargetTransform.GetRotation().RotateVector(FVector::ForwardVector);
				bFacingUpward = false;
			}
		}
		//const FRotator CurrentRotation = Character->GetActorRotation();
		//const FRotator RDiff{(TargetDirection.GetSafeNormal2D().Rotation() - CurrentRotation).GetNormalized()};
	}

	if (IsValid(RagdollingAnimInstance))
	{
		RagdollingAnimInstance->Refresh(*this, true);
	}

	if (Settings->bAllowFreeze)
	{
		RootBoneSpeed = Mover->GetVelocity().Size();

		if (IsValid(RagdollingAnimInstance))
		{
			RagdollingAnimInstance->UnFreeze();
		}

		if (bGrounded)
		{
			TimeAfterGrounded += DeltaTime;

			if (Settings->TimeAfterGroundedForForceFreezing > 0.0f &&
				TimeAfterGrounded > Settings->TimeAfterGroundedForForceFreezing)
			{
				if (ElapsedTime > Settings->StartBlendTime)
				{
					bFreezing = true;
				}
			}
			else if (RootBoneSpeed < Settings->RootBoneSpeedConsideredAsStopped)
			{
				TimeAfterGroundedAndStopped += DeltaTime;

				if (ElapsedTime > Settings->StartBlendTime)
				{
					if (Settings->TimeAfterGroundedAndStoppedForForceFreezing > 0.0f &&
						TimeAfterGroundedAndStopped > Settings->TimeAfterGroundedAndStoppedForForceFreezing)
					{
						bFreezing = true;
					}
					else
					{
						MaxBoneSpeed = 0.0f;
						MaxBoneAngularSpeed = 0.0f;
						Character->GetMesh()->ForEachBodyBelow(PhysicalAnimation->GetTopBoneName(), true, false, [&](FBodyInstance* Body) {
							float Speed = Body->GetUnrealWorldVelocity().Size();
							if(Speed > MaxBoneSpeed) MaxBoneSpeed = Speed;
							Speed = FMath::RadiansToDegrees(Body->GetUnrealWorldAngularVelocityInRadians().Size());
							if(Speed > MaxBoneAngularSpeed) MaxBoneAngularSpeed = Speed;
						});
						bFreezing = MaxBoneSpeed < Settings->SpeedThresholdToFreeze && MaxBoneAngularSpeed < Settings->AngularSpeedThresholdToFreeze;
					}
				}
			}
			else
			{
				TimeAfterGroundedAndStopped = 0.0f;
			}

			if (bFreezing)
			{
				if (IsValid(RagdollingAnimInstance))
				{
					RagdollingAnimInstance->Freeze();
				}
				Character->GetMesh()->SetAllBodiesSimulatePhysics(false);
			}
		}
		else
		{
			TimeAfterGrounded = TimeAfterGroundedAndStopped = 0.0f;
		}
	}

	if (ElapsedTime <= Settings->StartBlendTime && ElapsedTime + DeltaTime > Settings->StartBlendTime)
	{
		// Re-initialize bFacingUpward flag by current movement direction. If Velocity is Zero, it is chosen bFacingUpward is true.
		bFacingUpward = Character->GetActorForwardVector().Dot(Mover->GetVelocity().GetSafeNormal2D()) <= 0.0f;
	}

	if (bPreviousGrounded != bGrounded)
	{
		if (bGrounded)
		{
			Character->SetInputStance(GarStanceTags::Crouching);
		}
		else
		{
			Character->SetInputStance(GarStanceTags::Standing);
		}
	}
	bPreviousGrounded = bGrounded;

	ElapsedTime += DeltaTime;
}

void FGarRagdollingState::End(const UGarPhysicalAnimationComponent* PhysicalAnimation)
{
	auto Mover{Character->GetMover()};

	RagdollingAnimInstance->Freeze();
	RagdollingAnimInstance->Refresh(*this, false);

	// Re-enable capsule collision.

	//Character->GetCapsule()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	auto Capsule{Character->GetCapsule()};
	//// 全チャンネルを Ignore に初期化
	//Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);

	//// 必要なチャンネルに Block を設定
	//Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);     // 地形
	//Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);    // 動的オブジェクト
	//Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);            // 他のPawn
	//Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);      // トレース用
	//Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);         // カメラは無視（任意）

	Character->GetCapsule()->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	Character->GetProneCapsule()->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	Mover->SmoothingMode = EMoverSmoothingMode::VisualComponentOffset;
	Mover->bSyncInputsForSimProxy = true;

	if (RagdollingAnimInstance && ElapsedTime > Settings->StartBlendTime)
	{
		const auto TopTransform{Character->GetMesh()->GetBoneTransform(PhysicalAnimation->GetTopBoneName())};
		const auto TopRotation{TopTransform.Rotator()};

		// Determine yaw angle of the character.

		auto NewActorRotation{Mover->GetUpdatedComponentTransform().GetRotation().Rotator()};
		NewActorRotation.Yaw = UGarMath::DirectionToAngleXY(TopRotation.RotateVector(
			FMath::Abs(TopRotation.RotateVector(FVector::ForwardVector).GetSafeNormal2D().Dot(FVector::UpVector)) > 0.5f ?
			(bFacingUpward ? FVector::RightVector : FVector::LeftVector) :
			(bFacingUpward ? FVector::BackwardVector : FVector::ForwardVector)).GetSafeNormal2D());
		//Character->SetActorRotation(NewActorRotation, ETeleportType::TeleportPhysics);

		auto TeleportEffect = MakeShared<FTeleportEffect>();
		TeleportEffect->TargetLocation = Mover->GetUpdatedComponentTransform().GetLocation();
		//TeleportEffect->bUseActorRotation = true;
		TeleportEffect->TargetRotation = NewActorRotation;
		Mover->QueueInstantMovementEffect(TeleportEffect);

		// Restore the pelvis transform to the state it was in before we changed
		// the character and mesh transforms to keep its world transform unchanged.

		const auto& ReferenceSkeleton{Character->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton()};

		const auto TopBoneIndex{ReferenceSkeleton.FindBoneIndex(PhysicalAnimation->GetTopBoneName())};
		auto& FinalRagdollPose{RagdollingAnimInstance->GetFinalPoseSnapshot()};
		if (ensure(TopBoneIndex >= 0) && TopBoneIndex < FinalRagdollPose.LocalTransforms.Num())
		{
			// We expect the pelvis bone to be the root bone or attached to it, so we can safely use the mesh transform here.
			FinalRagdollPose.LocalTransforms[TopBoneIndex] = TopTransform.GetRelativeTransform(Character->GetMesh()->GetComponentTransform());
		}
	}

	// If the ragdoll is on the ground, set the movement mode to walking and play a get up montage. If not, set
	// the movement mode to falling and update the character movement velocity to match the last ragdoll velocity.

	//Mover->SetMovementModeLocked(false);

	if (bGrounded)
	{
		//Mover->SetMovementMode(MOVE_Walking);
	}
	else
	{
		//Mover->SetMovementMode(MOVE_Falling);
	}
}
