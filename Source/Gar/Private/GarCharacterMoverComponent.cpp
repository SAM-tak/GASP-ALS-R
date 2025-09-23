#include "GarCharacterMoverComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveVector.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "MoveLibrary/MovementUtils.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "MoverPoseSearchTrajectoryPredictor.h"
#include "MotionWarpingMoverAdapter.h"
#include "GarCharacter.h"
#include "MoverModes/GarMoverFallingMode.h"
#include "MoverModes/GarMoverWalkingMode.h"
#include "GarPhysicalAnimationComponent.h"
#include "Settings/GarMovementSettings.h"
#include "State/GarCharacterMoverInputs.h"
#include "Utility/GarUtility.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacterMoverComponent)

UGarCharacterMoverComponent::UGarCharacterMoverComponent()
{
	SetIsReplicatedByDefault(true);

	// Default movement modes
	MovementModes.Add(DefaultModeNames::Walking, CreateDefaultSubobject<UGarMoverWalkingMode>(TEXT("DefaultWalkingMode")));
	MovementModes.Add(DefaultModeNames::Falling, CreateDefaultSubobject<UGarMoverFallingMode>(TEXT("DefaultFallingMode")));

	StartingMovementMode = DefaultModeNames::Walking;
}

void UGarCharacterMoverComponent::InitializeComponent()
{
	Super::InitializeComponent();

	Character = Cast<AGarCharacter>(GetOwner());

	TrajectoryPredictor = NewObject<UMoverTrajectoryPredictor>();
	TrajectoryPredictor->Setup(this);
	MotionWarpingMoverAdapter = NewObject<UMotionWarpingMoverAdapter>();
	MotionWarpingMoverAdapter->SetMoverComp(this);
}

void UGarCharacterMoverComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LocomotionMode, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RotationMode, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Stance, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Gait, Parameters)
}

void UGarCharacterMoverComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ensure(Character.IsValid())) return;

	Settings = FindSharedSettings<UGarMovementSettings>();

	if(!ensureMsgf(Settings, TEXT("Failed to find instance of GarMovementSettings on %s. Movement may not function properly."), *GetPathNameSafe(this))) return;

	OnPreSimulationTick.AddUniqueDynamic(this, &UGarCharacterMoverComponent::OnMoverPreSimulationTick);
	OnMovementModeChanged.AddUniqueDynamic(this, &UGarCharacterMoverComponent::OnMoverMovementModeChanged);
}

void UGarCharacterMoverComponent::OnMoverPreSimulationTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd)
{
	auto CharacterInputs = InputCmd.InputCollection.FindDataByType<FGarCharacterMoverInputs>();
	if (CharacterInputs)
	{
		if (CharacterInputs->RotationMode.IsValid() && RotationMode == CharacterInputs->RotationMode)
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

		if (CharacterInputs->bIsJumpJustPressed && IsValid(Settings))
		{
			auto JumpMove = MakeShared<FJumpImpulseEffect>();
			JumpMove->UpwardsSpeed = Settings->JumpUpwardsSpeed;
 			QueueInstantMovementEffect(JumpMove);
		}
	}
}

void UGarCharacterMoverComponent::OnMoverMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName)
{
	if (auto& MovementMode = MovementModes[NewMovementModeName])
	{
		auto OldLocomotionMode{LocomotionMode};
		LocomotionMode = MovementMode->GameplayTags.Filter(LocomotionModeTags).First();
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LocomotionMode, this)

		if (OldLocomotionMode != LocomotionMode)
		{
			Character->OnLocomotionModeChanged(OldLocomotionMode);
		}
	}
}

void UGarCharacterMoverComponent::OnReplicated_LocomotionMode(const FGameplayTag& PreviousMovementMode) const
{
	Character->OnLocomotionModeChanged(PreviousMovementMode);
}

void UGarCharacterMoverComponent::AppendOwnedGameplayTags(FGameplayTagContainer& TagContainer)
{
	auto SyncState{MoverSyncStateDoubleBuffer.GetReadable()};

	// Append loose / external tags
	TagContainer.AppendTags(ExternalGameplayTags);

	// Append active Movement Mode
	if (auto ActiveMovementMode = FindMovementModeByName(SyncState.MovementMode))
	{
		TagContainer.AppendTags(ActiveMovementMode->GameplayTags);
	}

	TagContainer.AddTag(LocomotionMode);
	TagContainer.AddTag(RotationMode);
	TagContainer.AddTag(Stance);
	TagContainer.AddTag(Gait);
}

void UGarCharacterMoverComponent::SetInitialGameplayTags(const FGameplayTag& InRotationMode, const FGameplayTag& InStance, const FGameplayTag& InGait)
{
	if (auto MovementMode{FindMovementModeByName(StartingMovementMode)})
	{
		LocomotionMode = MovementMode->GameplayTags.Filter(LocomotionModeTags).First();
	}
	RotationMode = InRotationMode;
	Stance = InStance;
	Gait = InGait;
}

bool UGarCharacterMoverComponent::IsWalkable(const FHitResult& Hit) const
{
	return Hit.IsValidBlockingHit() && UFloorQueryUtils::IsHitSurfaceWalkable(Hit, GetUpDirection(), Settings->MaxWalkSlopeCosine);
}
