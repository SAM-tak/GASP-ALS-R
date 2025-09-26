#include "GarCharacterMoverComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "MoveLibrary/MovementUtils.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "MoverPoseSearchTrajectoryPredictor.h"
#include "MotionWarpingComponent.h"
#include "MotionWarpingMoverAdapter.h"
#include "GarCharacter.h"
#include "MoverModes/GarMoverFallingMode.h"
#include "MoverModes/GarMoverWalkingMode.h"
#include "MoverModes/GarMoverRagdollingMode.h"
#include "GarPhysicalAnimationComponent.h"
#include "Settings/GarMovementSettings.h"
#include "State/GarCharacterMoverInputs.h"
#include "Utility/GarUtility.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacterMoverComponent)

// -------------------------------------------------------------------
// FGarLayeredMove_TurnTo
// -------------------------------------------------------------------

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

	FRotator AngularVelocity = (CurrentTargetRotation - CurrentRotation).GetNormalized() * (1.0f / DeltaSeconds);

	OutProposedMove.AngularVelocity = AngularVelocity;

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

// -------------------------------------------------------------------
// UGarCharacterMoverComponent
// -------------------------------------------------------------------

UGarCharacterMoverComponent::UGarCharacterMoverComponent()
{
	SetIsReplicatedByDefault(true);

	// Default movement modes
	MovementModes.Add(DefaultModeNames::Walking, CreateDefaultSubobject<UGarMoverWalkingMode>(TEXT("DefaultWalkingMode")));
	MovementModes.Add(DefaultModeNames::Falling, CreateDefaultSubobject<UGarMoverFallingMode>(TEXT("DefaultFallingMode")));
	MovementModes.Add(TEXT("Ragdolling"), CreateDefaultSubobject<UGarMoverRagdollingMode>(TEXT("DefaultRagdollingMode")));

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

		if (CharacterInputs->bIsJumpJustPressed && IsValid(Settings))
		{
			auto JumpMove = MakeShared<FJumpImpulseEffect>();
			JumpMove->UpwardsSpeed = Settings->JumpUpwardsSpeed;
 			QueueInstantMovementEffect(JumpMove);
		}
	}
}

void UGarCharacterMoverComponent::SetLocomotionMode(const FGameplayTag& NewLocomotionMode)
{
	if (LocomotionMode != NewLocomotionMode)
	{
		auto OldLocomotionMode{LocomotionMode};
		LocomotionMode = NewLocomotionMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LocomotionMode, this)

		if (OldLocomotionMode != LocomotionMode)
		{
			Character->OnLocomotionModeChanged(OldLocomotionMode);
		}
	}
}

void UGarCharacterMoverComponent::OnMoverMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName)
{
	if (auto& MovementMode = MovementModes[NewMovementModeName])
	{
		SetLocomotionMode(MovementMode->GameplayTags.Filter(LocomotionModeTags).First());
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
