#include "GarCharacterMoverComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveVector.h"
#include "Engine/World.h"
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
	//PrimaryComponentTick.TickGroup = TG_PostPhysics;

	// https://unrealengine.hatenablog.com/entry/2019/01/16/231404

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
		auto& OldStance = GetStance();

		ControlRotation = CharacterInputs->ControlRotation;

		if (CharacterInputs->RotationMode.IsValid())
		{
			RotationMode = CharacterInputs->RotationMode;
		}

		if (CharacterInputs->Stance.IsValid())
		{
			Stance = CharacterInputs->Stance;
		}

		if (CharacterInputs->Gait.IsValid())
		{
			Gait = CharacterInputs->Gait;
		}

		if (CharacterInputs->bIsJumpJustPressed)
		{
			Jump();
		}

		if(CharacterInputs->Stance != OldStance)
		{
			OnStanceChanged.Broadcast(OldStance, CharacterInputs->Stance);
		}

		Character->RefreshCapsuleSize(TimeStep.StepMs * 0.001f);
	}
}

void UGarCharacterMoverComponent::OnMoverMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName)
{
	auto MovementMode{MovementModes[NewMovementModeName]};
	if (MovementMode)
	{
		auto OldLocomotionMode{LocomotionMode};
		LocomotionMode = MovementMode->GameplayTags.Filter(LocomotionModeTags).First();

		if (OldLocomotionMode != LocomotionMode)
		{
			Character->OnLocomotionModeChanged(OldLocomotionMode);
		}
	}
}

bool UGarCharacterMoverComponent::Jump()
{
	if (IsValid(Settings))
	{
		TSharedPtr<FJumpImpulseEffect> JumpMove = MakeShared<FJumpImpulseEffect>();
		JumpMove->UpwardsSpeed = Settings->JumpUpwardsSpeed;
		
 		QueueInstantMovementEffect(JumpMove);

		return true;
	}

	return false;
}

void UGarCharacterMoverComponent::AppendOwnedGameplayTags(FGameplayTagContainer& TagContainer)
{
	const FMoverSyncState& SyncState{MoverSyncStateDoubleBuffer.GetReadable()};

	// Append loose / external tags
	TagContainer.AppendTags(ExternalGameplayTags);

	// Append active Movement Mode
	if (const UBaseMovementMode* ActiveMovementMode = FindMovementModeByName(SyncState.MovementMode))
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
	auto MovementMode{FindMovementModeByName(StartingMovementMode)};
	if (MovementMode)
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
