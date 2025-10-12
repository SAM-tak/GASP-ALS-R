#include "GarCharacterMoverComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "MoverPoseSearchTrajectoryPredictor.h"
#include "MotionWarpingComponent.h"
#include "MotionWarpingMoverAdapter.h"
#include "GarCharacter.h"
#include "MoverModes/GarMoverFallingMode.h"
#include "MoverModes/GarMoverWalkingMode.h"
#include "MoverModes/GarMoverRagdollingMode.h"
#if GAR_USE_GE_FOR_MOVEMENTSTATE
#include "MoverModifiers/GarMoverRotationModifier.h"
#include "MoverModifiers/GarMoverStanceModifier.h"
#include "MoverModifiers/GarMoverGaitModifier.h"
#endif
#include "GarPhysicalAnimationComponent.h"
#include "Settings/GarMovementSettings.h"
#include "State/GarCharacterMoverInputs.h"
#include "State/GarCharacterMoverSyncState.h"
#include "Utility/GarUtility.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacterMoverComponent)

UGarCharacterMoverComponent::UGarCharacterMoverComponent()
{
#if !GAR_USE_GE_FOR_MOVEMENTSTATE
	SetIsReplicatedByDefault(true);
#endif
	//PersistentSyncStateDataTypes.Add(FMoverDataPersistence(FGarCharacterMoverSyncState::StaticStruct(), true));

	LocomotionModeTags.AddTag(GarLocomotionModeTags::Grounded);
	LocomotionModeTags.AddTag(GarLocomotionModeTags::InAir);

	// Default movement modes
	MovementModes.Add(DefaultModeNames::Walking, CreateDefaultSubobject<UGarMoverWalkingMode>(TEXT("DefaultWalkingMode")));
	MovementModes.Add(DefaultModeNames::Falling, CreateDefaultSubobject<UGarMoverFallingMode>(TEXT("DefaultFallingMode")));
	MovementModes.Add(TEXT("Ragdolling"), CreateDefaultSubobject<UGarMoverRagdollingMode>(TEXT("DefaultRagdollingMode")));
	MovementModes.Add(TEXT("Ragdolling In Air"), CreateDefaultSubobject<UGarMoverRagdollingMode>(TEXT("DefaultAirborneRagdollingMode")));
	MovementModes.Add(DefaultModeNames::Flying, CreateDefaultSubobject<UFlyingMode>(TEXT("DefaultFlyingMode")));

	auto AirborneRagdolling{MovementModes[TEXT("Ragdolling In Air")]};
	AirborneRagdolling->GameplayTags.Reset();
	AirborneRagdolling->GameplayTags.AddTag(GarLocomotionModeTags::InAir);

	auto FlyingMode{MovementModes[DefaultModeNames::Flying]};
	FlyingMode->GameplayTags.Reset();
	FlyingMode->GameplayTags.AddTag(GarLocomotionModeTags::InAir);

	StartingMovementMode = DefaultModeNames::Walking;
}

void UGarCharacterMoverComponent::InitializeComponent()
{
	Super::InitializeComponent();

	Character = Cast<AGarCharacter>(GetOwner());

	TrajectoryPredictor = NewObject<UMoverTrajectoryPredictor>();
	TrajectoryPredictor->Setup(this);
	MotionWarpingMoverAdapter = Character->GetMotionWarping()->CreateOwnerAdapter<UMotionWarpingMoverAdapter>();
	MotionWarpingMoverAdapter->SetMoverComp(this);
}

#if !GAR_USE_MOVEMENTMODIFIER
void UGarCharacterMoverComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RotationMode, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Stance, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Gait, Parameters)
}
#endif

void UGarCharacterMoverComponent::BeginPlay()
{
	if (!ensure(Character.IsValid()))
	{
		return;
	}

	Settings = FindSharedSettings<UGarMovementSettings>();

	if(!ensureMsgf(Settings, TEXT("Failed to find instance of GarMovementSettings on %s. Movement may not function properly."), *GetPathNameSafe(this)))
	{
		return;
	}

	CommonSettings = FindSharedSettings<UCommonLegacyMovementSettings>();

	if(!ensureMsgf(CommonSettings, TEXT("Failed to find instance of CommonLegacyMovementSettings on %s. Movement may not function properly."),
		*GetPathNameSafe(this)))
	{
		return;
	}

	Super::BeginPlay();

	OnPreSimulationTick.AddUniqueDynamic(this, &UGarCharacterMoverComponent::OnMoverPreSimulationTick);
	//OnPostMovement.AddUniqueDynamic(this, &UGarCharacterMoverComponent::OnMoverPostMovement);
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		OnPostFinalize.AddUniqueDynamic(this, &UGarCharacterMoverComponent::OnMoverPostFinalize);
	}
}

void UGarCharacterMoverComponent::OnMoverPreSimulationTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd)
{
	auto CharacterInputs = InputCmd.InputCollection.FindDataByType<FGarCharacterMoverInputs>();
	if (CharacterInputs)
	{
#if GAR_USE_MOVEMENTMODIFIER
		if (CharacterInputs->RotationMode.IsValid() && GetRotationMode() != CharacterInputs->RotationMode)
		{
			if (RotationModifierHandle.IsValid())
			{
				CancelModifierFromHandle(RotationModifierHandle);
				RotationModifierHandle.Invalidate();
			}
			if (CharacterInputs->RotationMode == GarRotationModeTags::ViewDirection)
			{
				RotationModifierHandle = QueueMovementModifier(MakeShared<FGarMoverViewDirectionModifier>());
			}
			else if (CharacterInputs->RotationMode == GarRotationModeTags::VelocityDirection)
			{
				RotationModifierHandle = QueueMovementModifier(MakeShared<FGarMoverVelocityDirectionModifier>());
			}
			else if (CharacterInputs->RotationMode == GarRotationModeTags::Aiming)
			{
				RotationModifierHandle = QueueMovementModifier(MakeShared<FGarMoverAimingModifier>());
			}
		}

		if (CharacterInputs->Stance.IsValid() && GetStance() != CharacterInputs->Stance)
		{
			if (StanceModifierHandle.IsValid())
			{
				CancelModifierFromHandle(StanceModifierHandle);
				StanceModifierHandle.Invalidate();
			}
			if (CharacterInputs->Stance == GarStanceTags::Standing)
			{
				StanceModifierHandle = QueueMovementModifier(MakeShared<FGarMoverStandingModifier>());
			}
			else if (CharacterInputs->Stance == GarStanceTags::Crouching)
			{
				StanceModifierHandle = QueueMovementModifier(MakeShared<FGarMoverCrouchingModifier>());
			}
			else if (CharacterInputs->Stance == GarStanceTags::Lying)
			{
				StanceModifierHandle = QueueMovementModifier(MakeShared<FGarMoverLyingModifier>());
			}
		}

		if (CharacterInputs->Gait.IsValid() && GetGait() != CharacterInputs->Gait)
		{
			if (GaitModifierHandle.IsValid())
			{
				CancelModifierFromHandle(GaitModifierHandle);
				GaitModifierHandle.Invalidate();
			}
			if (CharacterInputs->Gait == GarGaitTags::Walking)
			{
				GaitModifierHandle = QueueMovementModifier(MakeShared<FGarMoverWalkingModifier>());
			}
			else if (CharacterInputs->Gait == GarGaitTags::Running)
			{
				GaitModifierHandle = QueueMovementModifier(MakeShared<FGarMoverRunningModifier>());
			}
			else if (CharacterInputs->Gait == GarGaitTags::Sprinting)
			{
				GaitModifierHandle = QueueMovementModifier(MakeShared<FGarMoverSprintingModifier>());
			}
		}
#else
		if (CharacterInputs->RotationMode.IsValid() && RotationMode != CharacterInputs->RotationMode)
		{
			RotationMode = CharacterInputs->RotationMode;
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RotationMode, this)
		}

		if (CharacterInputs->Stance.IsValid() && Stance != CharacterInputs->Stance)
		{
			Stance = CharacterInputs->Stance;
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Stance, this)
		}

		if (CharacterInputs->Gait.IsValid() && Gait != CharacterInputs->Gait)
		{
			Gait = CharacterInputs->Gait;
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Gait, this)
		}
#endif
		if (CharacterInputs->bIsJumpJustPressed && IsValid(Settings))
		{
			auto JumpMove = MakeShared<FJumpImpulseEffect>();
			JumpMove->UpwardsSpeed = CommonSettings->JumpUpwardsSpeed;
 			QueueInstantMovementEffect(JumpMove);
		}
	}
}

void UGarCharacterMoverComponent::OnMoverPostMovement(const FMoverTimeStep& TimeStep, FMoverSyncState& SyncState, FMoverAuxStateContext& AuxState)
{
	auto MySyncState = SyncState.SyncStateCollection.FindMutableDataByType<FGarCharacterMoverSyncState>();
	if (MySyncState)
	{
		MySyncState->RotationMode = RotationMode;
		MySyncState->Stance = Stance;
		MySyncState->Gait = Gait;
	}
}

void UGarCharacterMoverComponent::OnMoverPostFinalize(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState)
{
	auto MySyncState = SyncState.SyncStateCollection.FindMutableDataByType<FGarCharacterMoverSyncState>();
	if (MySyncState)
	{
		RotationMode = MySyncState->RotationMode;
		Stance = MySyncState->Stance;
		Gait = MySyncState->Gait;
	}
}

FGameplayTag UGarCharacterMoverComponent::GetLocomotionMode() const
{
	for(auto& Tag : LocomotionModeTags)
	{
		if (HasGameplayTag(Tag, true))
		{
			return Tag;
		}
	}
	return FGameplayTag::EmptyTag;
}

#if GAR_USE_MOVEMENTMODIFIER
FGameplayTag UGarCharacterMoverComponent::GetRotationMode() const
{
	if (RotationModifierHandle.IsValid())
	{
		const auto* Modifier{static_cast<const FGarMoverModifier*>(FindMovementModifier(RotationModifierHandle))};
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		// This is a fail safe in case our handle was bad - try finding the modifier by type if we can
		Modifier = FindMovementModifierByType<FGarMoverViewDirectionModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		Modifier = FindMovementModifierByType<FGarMoverVelocityDirectionModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		Modifier = FindMovementModifierByType<FGarMoverAimingModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
	}
	return FGameplayTag::EmptyTag;
}

FGameplayTag UGarCharacterMoverComponent::GetStance() const
{
	if (StanceModifierHandle.IsValid())
	{
		const auto* Modifier{static_cast<const FGarMoverModifier*>(FindMovementModifier(StanceModifierHandle))};
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		// This is a fail safe in case our handle was bad - try finding the modifier by type if we can
		Modifier = FindMovementModifierByType<FGarMoverStandingModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		Modifier = FindMovementModifierByType<FGarMoverCrouchingModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		Modifier = FindMovementModifierByType<FGarMoverLyingModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
	}
	return FGameplayTag::EmptyTag;
}

FGameplayTag UGarCharacterMoverComponent::GetGait() const
{
	if (GaitModifierHandle.IsValid())
	{
		const auto* Modifier{static_cast<const FGarMoverModifier*>(FindMovementModifier(GaitModifierHandle))};
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		// This is a fail safe in case our handle was bad - try finding the modifier by type if we can
		Modifier = FindMovementModifierByType<FGarMoverWalkingModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		Modifier = FindMovementModifierByType<FGarMoverRunningModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
		Modifier = FindMovementModifierByType<FGarMoverSprintingModifier>();
		if (Modifier)
		{
			return Modifier->ActiveTag;
		}
	}
	return FGameplayTag::EmptyTag;
}
#endif

bool UGarCharacterMoverComponent::IsWalkable(const FHitResult& Hit) const
{
	if (Settings)
	{
		return Hit.IsValidBlockingHit() && UFloorQueryUtils::IsHitSurfaceWalkable(Hit, GetUpDirection(), Settings->MaxWalkSlopeCosine);
	}
	if (CommonSettings)
	{
		return Hit.IsValidBlockingHit() && UFloorQueryUtils::IsHitSurfaceWalkable(Hit, GetUpDirection(), CommonSettings->MaxWalkSlopeCosine);
	}
	return Hit.IsValidBlockingHit() && UFloorQueryUtils::IsHitSurfaceWalkable(Hit, GetUpDirection(), 0.0f);
}
