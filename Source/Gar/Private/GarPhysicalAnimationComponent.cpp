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
#include "GarCharacter.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "GarAnimationInstance.h"
#include "GarLinkedAnimationInstance.h"
#include "GarAbilitySystemComponent.h"
#include "GarCharacterMovementComponent.h"
#include "Abilities/Actions/GarGameplayAbility_Ragdolling.h"
#include "LinkedAnimLayers/GarRagdollingAnimInstance.h"
#include "Settings/GarRagdollingSettings.h"
#include "Utility/GarUtility.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarPhysicalAnimationComponent)

//# The naming convention for Physical Animation Profile names
//
//If a Physical Animation Profile with a name corresponding to GAR’s LocomotionMode, LocomotionAction, Stance, Gait, and OverlayMode exists, it will be applied.Only the physical bodies included in the Physical Animation Profile will be subject to simulation, and all others will be set to kinematic.
//
//Names should either match exactly with LocomotionMode or LocomotionAction, Stance, Gait, OverlayMode(such as Grounded or Ragdolling), or they should be a concatenation of LocomotionMode or LocomotionAction, Stance, Gait, OverlayMode in that order, separated by `:`.
//
//For example, if you want a profile to be applied only when LocomotionMode = Grounded, Stance = Standing, and Gait = Running, you would name it `Grounded:Standing:Running`.
//
//If there is no profile with a name that matches the current state, and there is a profile named “Default”, then the “Default” profile will be selected.
//If a name starts with `+`, it means that the profile is to be added and applied to any other matching profiles.
//
//If a name starts with `*`, it can overwrite only the physical animation parameters without affecting the on / off state of the physical simulation, after applying other matching profiles.
//
//![Example of physical aniImation profile](https://github.com/Sixze/GAR-Refactored/assets/250165/455644f3-e7cf-4885-a858-2124f8f3d1c5)
//
//The `*Injured` profile, when the overlay mode is Injured, changes only the physical animation parameters without changing the range of physical simulation, after the Default or Mantle profile is applied.
//
//During ragdolling, only profiles named `Ragdolling` or those containing `Ragdolling` in their names, such as `Ragdolling:Injured`, will be applied.
//
//If you only create the `Ragdolling` profile and do not create a `Default` profile, the effects of the physical animation will only be applied during ragdolling.
//
//# Animation Curve
//
//Added new curves below :
//
//- PALockArmLeft
//- PALockArmRight
//- PALockLegLeft
//- PALockLegRight
//- PALockHandLeft
//- PALockHandRight
//- PALockFootLeft
//- PALockFootRight
//
//By setting a value greater than 0 to these animation curves in animation sequence or animation montage, you can temporarily disable the corresponding physical animation without switching profiles.
//
//For example, during Mantling, these curves are set to disable the physical animation of the corresponding parts temporarily, so as not to interfere with the action of lifting the leg high.

UGarPhysicalAnimationComponent::UGarPhysicalAnimationComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetNetAddressable(); // Make DSO components net addressable
	SetIsReplicatedByDefault(true); // Enable replication by default
}

void UGarPhysicalAnimationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RagdollingTargetLocation, Parameters)
}

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
	if (CurrentMultiplyProfileNames != MultiplyProfileNames)
	{
		CurrentMultiplyProfileNames = MultiplyProfileNames;
		bRetVal = true;
	}
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

namespace GarPhysicalAnimationTagCombination
{
	struct FIterator
	{
		const struct FContainer* Container;
		TArray<FName> CurrentNames;
		TArray<int> Indices;

		const TArray<FName>& operator*() const { return CurrentNames; }
		FIterator& operator++();
		bool operator!=(const FIterator& lhs) const { return Container != Container || Indices != lhs.Indices || CurrentNames != lhs.CurrentNames; }
	};

	struct FContainer
	{
		TArray<FName> SourceNames;
		FIterator begin() const { return {this, SourceNames}; }
		FIterator end() const { return {nullptr}; }
	};

	FIterator& FIterator::operator++()
	{
		if (!Container)
		{
			CurrentNames.Reset();
			Indices.Reset();
			return *this;
		}

		auto SourceNum{Container->SourceNames.Num()};

		// ex. SourceNum = 5
		// SourceNum - IndicesNum(1) = 4
		// 11110 0
		// 11101 1
		// 11011 2
		// 10111 3
		// 01111 4
		// 
		// SourceNum - IndicesNum(2) = 3
		// 11100 0 0
		// 11010 0 1
		// 11001 1 1
		// 10110 0 2
		// 10101 1 2
		// 10011 2 2
		// 01110 0 3
		// 01101 1 3
		// 01011 2 3
		// 00111 3 3
		// 
		// SourceNum - IndicesNum(3) = 2
		// 11000 0 0 0
		// 10100 0 0 1
		// 10010 0 1 1
		// 10001 1 1 1
		// 01100 0 0 2
		// 01010 0 1 2
		// 01001 1 1 2
		// 00110 0 2 2
		// 00101 1 2 2
		// 00011 2 2 2
		// 
		// SourceNum - IndicesNum(4) = 1
		// 10000 0 0 0 0
		// 01000 0 0 0 1
		// 00100 0 0 1 1
		// 00010 0 1 1 1
		// 00001 1 1 1 1

		if (Indices.IsEmpty())
		{
			Indices.Push(0);
		}
		else
		{
			bool bNextDepth = false;
			auto IndicesNum{Indices.Num()};
			for (int Index = 0; Index < IndicesNum; ++Index)
			{
				if (Index + 1 < IndicesNum)
				{
					if (Indices[Index] < Indices[Index + 1])
					{
						Indices[Index]++;
						break;
					}
					else
					{
						Indices[Index] = 0;
					}
				}
				else
				{
					if (Indices[Index] < SourceNum - IndicesNum)
					{
						Indices[Index]++;
					}
					else
					{
						Indices[Index] = 0;
						bNextDepth = true;
					}
				}
			}
			if (bNextDepth)
			{
				if(IndicesNum + 1 < SourceNum)
				{
					Indices.Push(0);
				}
				else
				{
					Container = nullptr;
					CurrentNames.Reset();
					Indices.Reset();
					return *this;
				}
			}
		}

		CurrentNames = Container->SourceNames;

		if (SourceNum > 1)
		{
			for (auto Index : Indices)
			{
				CurrentNames.RemoveAt(CurrentNames.Num() - 1 - Index);
			}
		}

		return *this;
	}
}

void UGarPhysicalAnimationComponent::SelectProfile()
{
	using namespace GarPhysicalAnimationTagCombination;
	TArray<FName> NextProfileNames;
	FName NextBaseProfileName;
	TArray<FName> NextAdditiveProfileNames;
	TArray<FName> NextMultiplyProfileNames;
	TStringBuilder<256> StringBuilder;
	TStringBuilder<256> AdditionalStringBuilder;

	FName RagdollingModeName{CurrentRagdolling.IsValid() ? UGarUtility::GetSimpleTagName(CurrentRagdolling) : NAME_None};

	FContainer Container;

	for (auto& Mask : GameplayTagMasks)
	{
		auto Name{UGarUtility::GetSimpleTagName(CurrentGameplayTags.Filter(Mask).First())};
		if (Name.IsValid() && !Name.IsNone())
		{
			Container.SourceNames.Add(Name);
		}
	}

	for (auto& Names : Container)
	{
		if(bRagdolling && !Names.Contains(RagdollingModeName))
		{
			continue;
		}

		for(auto& Name : Names)
		{
			if (StringBuilder.Len() > 0)
			{
				StringBuilder << TEXT(":");
			}
			StringBuilder << Name;
		}

		UE_LOG(LogGar, VeryVerbose, TEXT("Try Physical Animation Profile '%s'"), *StringBuilder);

		// determin base profile

		if (NextBaseProfileName.IsNone())
		{
			FName ProfileName(StringBuilder, FNAME_Find);
			if (IsProfileExist(ProfileName))
			{
				NextBaseProfileName = ProfileName;
			}
		}

		// add additive profile if exists

		AdditionalStringBuilder << TEXT("+");
		AdditionalStringBuilder.Append(StringBuilder);
		FName AdditiveProfileName(AdditionalStringBuilder, FNAME_Find);
		AdditionalStringBuilder.Reset();
		if (IsProfileExist(AdditiveProfileName))
		{
			NextAdditiveProfileNames.Add(AdditiveProfileName);
		}

		// add multiply profile if exists

		AdditionalStringBuilder << TEXT("*");
		AdditionalStringBuilder.Append(StringBuilder);
		FName MultiplyProfileName(AdditionalStringBuilder, FNAME_Find);
		AdditionalStringBuilder.Reset();
		if (IsProfileExist(MultiplyProfileName))
		{
			NextMultiplyProfileNames.Add(MultiplyProfileName);
		}

		StringBuilder.Reset();
	}

	if (NextBaseProfileName.IsNone() && bRagdolling && IsProfileExist(RagdollingModeName))
	{
		NextBaseProfileName = RagdollingModeName;
	}
	if (NextBaseProfileName.IsNone() && !bRagdolling && IsProfileExist(DefaultProfileName))
	{
		NextBaseProfileName = DefaultProfileName;
	}

	if (NextBaseProfileName.IsNone())
	{
		NextAdditiveProfileNames.Reset();
		NextMultiplyProfileNames.Reset();
	}
	else
	{
		NextProfileNames.Add(NextBaseProfileName);
		NextProfileNames.Append(NextAdditiveProfileNames);
		NextMultiplyProfileNames.Append(MultiplyProfileNames);
	}

	if (NextProfileNames != CurrentProfileNames || NextMultiplyProfileNames != CurrentMultiplyProfileNames)
	{
		bool bFirst = true;
		for (const auto& NextProfileName : NextProfileNames)
		{
			ApplyPhysicalAnimationProfileBelow(NAME_None, NextProfileName);
			GetSkeletalMesh()->SetConstraintProfileForAll(NextProfileName, bFirst);
			bFirst = false;
		}
		CurrentProfileNames = NextProfileNames;

		for (const auto& NextMultiplyProfileName : NextMultiplyProfileNames)
		{
			ApplyPhysicalAnimationProfileBelow(NAME_None, NextMultiplyProfileName);
			GetSkeletalMesh()->SetConstraintProfileForAll(NextMultiplyProfileName);
		}
		CurrentMultiplyProfileNames = NextMultiplyProfileNames;
	}
}

void UGarPhysicalAnimationComponent::OnRegister()
{
	Super::OnRegister();
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	if (IsValid(Character))
	{
		Character->OnRefresh.AddUObject(this, &ThisClass::OnRefresh);
		RagdollingState.Character = Character;
	}
}

void UGarPhysicalAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
	auto* Character{Cast<AGarCharacter>(GetOwner())};
	if (IsValid(Character))
	{
		RagdollingState.RagdollingAnimInstance = Character->GetGarAnimationInstace()->GetRagdollingAnimInstance();
	}
}

void UGarPhysicalAnimationComponent::OnRefresh(float DeltaTime)
{
	auto* Character{Cast<AGarCharacter>(GetOwner())};

	// Apply special behaviour when changed Ragdolling state
	
	CurrentRagdolling = FGameplayTag::EmptyTag;
	for(auto& KeyValue : RagdollingSettingsMap)
	{
		if (Character->HasMatchingGameplayTag(KeyValue.Key))
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
			SetRagdollingTargetLocation(RagdollingState.TargetLocation);

			GetSkeletalMesh()->SetAllBodiesBelowSimulatePhysics(TopBoneName, true);
			GetSkeletalMesh()->SetAllBodiesPhysicsBlendWeight(1.0f);
			ApplyPhysicalAnimationProfileBelow(NAME_None, NAME_None, true, true);
		}

		RagdollingState.TargetLocation = RagdollingTargetLocation;
		RagdollingState.Tick(DeltaTime, this);
		SetRagdollingTargetLocation(RagdollingState.TargetLocation);

		if (RagdollingState.bGrounded)
		{
			Character->SetLocomotionMode(GarLocomotionModeTags::Grounded);
		}
		else
		{
			Character->SetLocomotionMode(GarLocomotionModeTags::InAir);
		}
	}
	else
	{
		if (bRagdolling)
		{
			bRagdolling = false;

			RagdollingState.End(this);
			SetRagdollingTargetLocation(FVector::ZeroVector);

			CurrentProfileNames.Reset();
			CurrentMultiplyProfileNames.Reset();
			ClearGameplayTags();
			GetSkeletalMesh()->SetAllBodiesSimulatePhysics(false);
			GetSkeletalMesh()->SetAllBodiesPhysicsBlendWeight(0.0f);
			GetSkeletalMesh()->SetConstraintProfileForAll(NAME_None, true);

			if (bActive)
			{
				GetSkeletalMesh()->SetCollisionObjectType(PrevCollisionObjectType);
				GetSkeletalMesh()->SetCollisionEnabled(PrevCollisionEnabled);
				bActive = false;
			}
		}
	}

	FGameplayTagContainer TempContainer;
	Character->GetOwnedGameplayTags(TempContainer);
	FGameplayTagContainer TempMaskContainer;
	for (auto& Container : GameplayTagMasks)
	{
		TempMaskContainer.AppendTags(Container);
	}
	CurrentGameplayTags.Reset();
	CurrentGameplayTags.AppendMatchingTags(TempContainer, TempMaskContainer);

	CurveValues.Refresh(Character, CurveBoneMappings);
}

void UGarPhysicalAnimationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Choose Physical Animation Profile

	if (OverrideProfileNames.Num() > 0)
	{
		if (CurrentProfileNames != OverrideProfileNames || CurrentMultiplyProfileNames != MultiplyProfileNames)
		{
			bool bFirst = true;
			for (const auto& CurrentProfileName : OverrideProfileNames)
			{
				ApplyPhysicalAnimationProfileBelow(NAME_None, CurrentProfileName);
				GetSkeletalMesh()->SetConstraintProfileForAll(CurrentProfileName, bFirst);
				bFirst = false;
			}
			CurrentProfileNames = OverrideProfileNames;
			ClearGameplayTags();

			for (const auto& MultiplyProfileName : MultiplyProfileNames)
			{
				ApplyPhysicalAnimationProfileBelow(NAME_None, MultiplyProfileName);
				GetSkeletalMesh()->SetConstraintProfileForAll(MultiplyProfileName);
			}
			CurrentMultiplyProfileNames = MultiplyProfileNames;
		}
	}
	else if(NeedsProfileChange())
	{
		SelectProfile();
	}

	// Update PhysicsBlendWeight and Collision settings

	if (!bRagdolling || !RagdollingState.bFreezing)
	{
		RefreshBodyState(DeltaTime);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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

	for (const auto& ProfileName : CurrentMultiplyProfileNames)
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

void UGarPhysicalAnimationComponent::SetRagdollingTargetLocation(const FVector& NewTargetLocation)
{
	if (RagdollingTargetLocation != NewTargetLocation)
	{
		RagdollingTargetLocation = NewTargetLocation;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RagdollingTargetLocation, this)

		auto* Character{Cast<AGarCharacter>(GetOwner())};
		if (Character->IsCharacterSelf())
		{
			ServerSetRagdollingTargetLocation(RagdollingTargetLocation);
		}
	}
}

void UGarPhysicalAnimationComponent::ServerSetRagdollingTargetLocation_Implementation(const FVector_NetQuantize& NewTargetLocation)
{
	SetRagdollingTargetLocation(NewTargetLocation);
}

// FGarPhysicalAnimationCurveValues

void FGarPhysicalAnimationCurveValues::Refresh(AGarCharacter *Character, const TArray<FGarPACurveBoneMapping>& Mappings)
{
	auto AnimInstance{Character->GetGarAnimationInstace()};
	if (!AnimInstance)
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

	if (!IsValid(RagdollingAnimInstance))
	{
		return;
	}

	// Ensure freeze flag is off.

	RagdollingAnimInstance->UnFreeze();

	RagdollingAnimInstance->SetStartBlendTime(Settings->StartBlendTime);

	PullForce = 0.0f;
	ElapsedTime = 0.0f;
	TimeAfterGrounded = TimeAfterGroundedAndStopped = 0.0f;
	bFacingUpward = bGrounded = false;
	bPreviousGrounded = true;
	bFreezing = false;
	PrevActorLocation = Character->GetActorLocation();

	auto* CharacterMovement{Character->GetGarCharacterMovement()};

	// Initialize bFacingUpward flag by current movement direction. If Velocity is Zero, it is chosen bFacingUpward is true.
	// And determine target yaw angle of the character.

	const auto PoleDirection = CharacterMovement->Velocity.GetSafeNormal2D();

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

	// Stop any active montages.

	auto* AnimInstance{Character->GetGarAnimationInstace()};
	AnimInstance->Montage_Stop(Settings->StartBlendTime);

	// Disable movement corrections and reset network smoothing.

	CharacterMovement->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
	CharacterMovement->bIgnoreClientMovementErrorChecksAndCorrection = true;

	// Disable capsule collision. other physics states will be changed by physical aniamtion process

	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (Character->IsLocallyControlled() || (Character->GetLocalRole() >= ROLE_Authority && !IsValid(Character->GetController())))
	{
		TargetLocation = Character->GetMesh()->GetBoneLocation(PhysicalAnimation->GetTopBoneName());
	}

	// Clear the character movement mode and set the locomotion action to ragdolling.

	CharacterMovement->SetMovementMode(MOVE_Custom);
	CharacterMovement->SetMovementModeLocked(true);

	RagdollingAnimInstance->Refresh(*this, true);
}

FVector FGarRagdollingState::TraceGround()
{
	auto* CharacterMovement{Character->GetGarCharacterMovement()};

	const auto Capsule{Character->GetCapsuleComponent()};
	const auto CapsuleHalfHeight{Capsule->GetScaledCapsuleHalfHeight()};

	const auto TraceStart{!TargetLocation.IsZero() ? FVector{TargetLocation}: Character->GetActorLocation()};
	const FVector TraceEnd{TraceStart.X, TraceStart.Y, TraceStart.Z - CapsuleHalfHeight};

	FHitResult Hit;

	Character->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, Capsule->GetCollisionObjectType(),
													{__FUNCTION__, false, Character},
													Capsule->GetCollisionResponseToChannel(Capsule->GetCollisionObjectType()));

	bGrounded = CharacterMovement->IsWalkable(Hit);

	return {
		TraceStart.X, TraceStart.Y,
		bGrounded ? Hit.ImpactPoint.Z + CapsuleHalfHeight + UCharacterMovementComponent::MIN_FLOOR_DIST : TraceStart.Z
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

	auto* CharacterMovement{Character->GetGarCharacterMovement()};

	auto NetMode{Character->GetWorld()->GetNetMode()};
	bool bCharacterSelf{Character->IsCharacterSelf()};

	if (bCharacterSelf)
	{
		TargetLocation = Character->GetMesh()->GetBoneLocation(PhysicalAnimation->GetTopBoneName());
	}

	// just for info.
	CharacterMovement->Velocity = FMath::VInterpTo(CharacterMovement->Velocity,
												   DeltaTime > 0.0f ? (Character->GetActorLocation() - PrevActorLocation) / DeltaTime : FVector::Zero(),
												   DeltaTime, Settings->VelocityInterpolationSpeed);
	PrevActorLocation = Character->GetActorLocation();

	// Prevent the capsule from going through the ground when the ragdoll is lying on the ground.

	// While we could get rid of the line trace here and just use TargetLocation
	// as the character's location, we don't do that because the camera depends on the
	// capsule's bottom location, so its removal will cause the camera to behave erratically.

	if (bCharacterSelf || NetMode == NM_DedicatedServer)
	{
		Character->SetActorLocation(TraceGround(), true);
	}
	else
	{
		Character->SetActorLocation(FMath::VInterpTo(Character->GetActorLocation(), TraceGround(), DeltaTime, Settings->SimulatedProxyInterpolationSpeed), true);
	}

	// Zero target location means that it hasn't been replicated yet, so we can't apply the logic below.

	if (!bCharacterSelf && !TargetLocation.IsZero())
	{
		// Apply ragdoll location corrections.

		auto TopLocation{Character->GetMesh()->GetBoneLocation(PhysicalAnimation->GetTopBoneName())};

		auto NewTopLocation{FMath::VInterpTo(TopLocation, TargetLocation, DeltaTime, Settings->SimulatedProxyMeshInterpolationSpeed)};

		auto Diff{NewTopLocation - TopLocation};

		for(auto& Body : Character->GetMesh()->Bodies)
		{
			auto Transform{Body->GetUnrealWorldTransform()};
			Transform.SetLocation(Transform.GetLocation() + Diff);
			Body->SetBodyTransform(Transform, ETeleportType::TeleportPhysics);
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

	RagdollingAnimInstance->Refresh(*this, true);

	if (Settings->bAllowFreeze)
	{
		RootBoneSpeed = CharacterMovement->Velocity.Size();

		RagdollingAnimInstance->UnFreeze();

		if (bGrounded)
		{
			TimeAfterGrounded += DeltaTime;

			if (Settings->TimeAfterGroundedForForceFreezing > 0.0f &&
				TimeAfterGrounded > Settings->TimeAfterGroundedForForceFreezing)
			{
				bFreezing = true;
			}
			else if (RootBoneSpeed < Settings->RootBoneSpeedConsideredAsStopped)
			{
				TimeAfterGroundedAndStopped += DeltaTime;

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
			else
			{
				TimeAfterGroundedAndStopped = 0.0f;
			}

			if (bFreezing)
			{
				RagdollingAnimInstance->Freeze();
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
		bFacingUpward = Character->GetActorForwardVector().Dot(CharacterMovement->Velocity.GetSafeNormal2D()) <= 0.0f;
	}

	if (bPreviousGrounded != bGrounded)
	{
		if (bGrounded)
		{
			Character->Crouch();
		}
		else
		{
			Character->UnCrouch();
		}
	}
	bPreviousGrounded = bGrounded;

	ElapsedTime += DeltaTime;
}

void FGarRagdollingState::End(const UGarPhysicalAnimationComponent* PhysicalAnimation)
{
	auto CharacterMovement{Character->GetGarCharacterMovement()};

	RagdollingAnimInstance->Freeze();
	RagdollingAnimInstance->Refresh(*this, false);

	// Re-enable capsule collision.

	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	CharacterMovement->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	CharacterMovement->bIgnoreClientMovementErrorChecksAndCorrection = false;

	if (RagdollingAnimInstance && ElapsedTime > Settings->StartBlendTime)
	{
		const auto TopTransform{Character->GetMesh()->GetBoneTransform(PhysicalAnimation->GetTopBoneName())};
		const auto TopRotation{TopTransform.Rotator()};

		// Determine yaw angle of the character.

		auto NewActorRotation{Character->GetActorRotation()};
		NewActorRotation.Yaw = UGarMath::DirectionToAngleXY(TopRotation.RotateVector(
			FMath::Abs(TopRotation.RotateVector(FVector::ForwardVector).GetSafeNormal2D().Dot(FVector::UpVector)) > 0.5f ?
			(bFacingUpward ? FVector::RightVector : FVector::LeftVector) :
			(bFacingUpward ? FVector::BackwardVector : FVector::ForwardVector)).GetSafeNormal2D());
		Character->SetActorRotation(NewActorRotation, ETeleportType::TeleportPhysics);

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

	TargetLocation = FVector::ZeroVector;

	// If the ragdoll is on the ground, set the movement mode to walking and play a get up montage. If not, set
	// the movement mode to falling and update the character movement velocity to match the last ragdoll velocity.

	CharacterMovement->SetMovementModeLocked(false);

	if (bGrounded)
	{
		CharacterMovement->SetMovementMode(MOVE_Walking);
	}
	else
	{
		CharacterMovement->SetMovementMode(MOVE_Falling);
	}
}
