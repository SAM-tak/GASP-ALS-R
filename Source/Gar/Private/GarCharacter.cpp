#include "GarCharacter.h"

#include "MotionWarpingComponent.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameNetworkManager.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "MoveLibrary/BasedMovementUtils.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "Components/GarOverlayModeComponent.h"
#include "Components/GarDeltaOverlayModeComponent.h"
#include "Components/GarOverrideModeComponent.h"
#include "Settings/GarCharacterSettings.h"
#include "State/GarCharacterMoverInputs.h"
#include "GarAnimationInstance.h"
#include "GarCharacterMoverComponent.h"
#include "GarPhysicalAnimationComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarGameplayTags.h"
#include "GarConstants.h"
#include "Utility/GarUtility.h"
#include "Utility/GarMath.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacter)

namespace GarCharacterConstants
{
	constexpr auto TeleportDistanceThresholdSquared{FMath::Square(50.0f)};
}

FName AGarCharacter::SkeletalMeshComponentName(TEXT("CharacterMesh"));
FName AGarCharacter::CapsuleComponentName(TEXT("CharacterCollider"));
FName AGarCharacter::ProneCapsuleComponentName(TEXT("HorizontalCollider"));
FName AGarCharacter::CharacterMoverComponentName(TEXT("CharacterMoverComp"));
FName AGarCharacter::MotionWarpingComponentName(TEXT("MotionWarpComp"));
FName AGarCharacter::PhysicalAnimationComponentName(TEXT("PhysicalAnimComp"));
FName AGarCharacter::AbilitySystemComponentName(TEXT("AbilitySystemComp"));
FName AGarCharacter::OverlayModeComponentName(TEXT("OverlayComp"));
FName AGarCharacter::DeltaOverlayModeComponentName(TEXT("DeltaOverlayComp"));
FName AGarCharacter::OverrideModeComponentName(TEXT("OverrideComp"));

AGarCharacter::AGarCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	SetReplicatingMovement(false);	// disable Actor-level movement replication, since our Mover component will handle it

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(CapsuleComponentName);
	Capsule->InitCapsuleSize(30.0f, 90.0f);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	Capsule->CanCharacterStepUpOn = ECB_No;
	Capsule->SetShouldUpdatePhysicsVolume(true);
	Capsule->SetCanEverAffectNavigation(false);
	Capsule->bDynamicObstacle = true;
	RootComponent = Capsule;

	ProneCapsule = CreateDefaultSubobject<UCapsuleComponent>(ProneCapsuleComponentName);
	ProneCapsule->InitCapsuleSize(30.0f, 30.0f);
	ProneCapsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	ProneCapsule->CanCharacterStepUpOn = ECB_No;
	ProneCapsule->SetShouldUpdatePhysicsVolume(true);
	ProneCapsule->SetCanEverAffectNavigation(false);
	ProneCapsule->bDynamicObstacle = true;
	ProneCapsule->SetupAttachment(Capsule);
	ProneCapsule->SetRelativeRotation_Direct({-90.0f, 0.0f, 0.0f});

	Mesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(SkeletalMeshComponentName);
	if (Mesh)
	{
		Mesh->AlwaysLoadOnClient = true;
		Mesh->AlwaysLoadOnServer = true;
		Mesh->bOwnerNoSee = false;
		//Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		Mesh->bCastDynamicShadow = true;
		Mesh->bAffectDynamicIndirectLighting = true;
		Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Mesh->SetupAttachment(Capsule);
		static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
		Mesh->SetCollisionProfileName(MeshCollisionProfileName);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetRelativeLocation_Direct({0.0f, 0.0f, -92.0f});
		Mesh->SetRelativeRotation_Direct({0.0f, -90.0f, 0.0f});
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
		Mesh->bEnableUpdateRateOptimizations = false;
	}

	CharacterMover = CreateDefaultSubobject<UGarCharacterMoverComponent>(CharacterMoverComponentName);
	if (CharacterMover && Mesh)
	{
		CharacterMover->SetPrimaryVisualComponent(Mesh);
	}

	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(MotionWarpingComponentName);

	PhysicalAnimation = CreateDefaultSubobject<UGarPhysicalAnimationComponent>(PhysicalAnimationComponentName);

	AbilitySystem = CreateDefaultSubobject<UGarAbilitySystemComponent>(AbilitySystemComponentName);

	OverlayModeComponent = CreateDefaultSubobject<UGarOverlayModeComponent>(OverlayModeComponentName);

	DeltaOverlayModeComponent = CreateDefaultSubobject<UGarDeltaOverlayModeComponent>(DeltaOverlayModeComponentName);

	OverrideModeComponent = CreateDefaultSubobject<UGarOverrideModeComponent>(OverrideModeComponentName);
}

// IAbilitySystemInterface

UAbilitySystemComponent* AGarCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

// IGameplayTagAssetInterface

void AGarCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (IsValid(AbilitySystem))
	{
		AbilitySystem->GetOwnedGameplayTags(TagContainer);
	}
	else
	{
		TagContainer.Reset();
	}
}

bool AGarCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	return AbilitySystem->HasMatchingGameplayTag(TagToCheck);
}

bool AGarCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return AbilitySystem->HasAllMatchingGameplayTags(TagContainer);
}

bool AGarCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return AbilitySystem->HasAnyMatchingGameplayTags(TagContainer);
}

void AGarCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedControlRotation, Parameters)
}

void AGarCharacter::PostInitializeComponents()
{
	// Make sure the mesh and animation blueprint are ticking after the character so they can access the most up-to-date character state.

	if (Mesh)
	{
		AnimationInstance = Cast<UGarAnimationInstance>(Mesh->GetAnimInstance());
	}

	if (CharacterMover)
	{
		if (USceneComponent* UpdatedComponent = CharacterMover->GetUpdatedComponent())
		{
			UpdatedComponent->SetCanEverAffectNavigation(bCanAffectNavigationGeneration);
		}

		// ACharacter と同様に、メッシュのアニメーションティックが Mover の
		// FinalizeFrame / FinalizeSmoothingFrame より後に実行されるよう依存を設定。
		// これにより SimProxy の VisualComponentOffset スムージング後の位置を
		// OffsetRootBone が正しく参照できる。
		if (Mesh && Mesh->PrimaryComponentTick.bCanEverTick)
		{
			Mesh->PrimaryComponentTick.AddPrerequisite(CharacterMover, CharacterMover->PrimaryComponentTick);
		}
	}

	if (PhysicalAnimation)
	{
		PhysicalAnimation->SetSkeletalMeshComponent(Mesh);
		if (PhysicalAnimation->PrimaryComponentTick.bCanEverTick)
		{
			PhysicalAnimation->PrimaryComponentTick.AddPrerequisite(Mesh, Mesh->PrimaryComponentTick);
		}
	}

	if (IsValid(AbilitySystem))
	{
		AbilitySystem->Initialize(this);
		AbilitySystem->TryActivateAbilitiesBySingleTag(InitialOverlay);
		AbilitySystem->TryActivateAbilitiesBySingleTag(InitialDeltaOverlay);
	}

	Super::PostInitializeComponents();

	Capsule->GetUnscaledCapsuleSize(InitialCapsuleRadius, InitialCapsuleHalfHeight);
	ProneCapsule->GetUnscaledCapsuleSize(InitialProneCapsuleRadius, InitialProneCapsuleHalfHeight);
	InitialEyeHeight = BaseEyeHeight;
	InitialMeshZ = Mesh->GetRelativeLocation().Z;
	InitialProneCapsuleX = ProneCapsule->GetRelativeLocation().X;

	SetDesiredRotationMode(InitialDesiredRotationMode);
	SetDesiredStance(InitialDesiredStance);
	SetDesiredGait(InitialDesiredGait);
	SetPerspective(InitialPerspective);
}

bool AGarCharacter::TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest, bool bNoCheck)
{
	// PAC は常にオンのためメッシュボディが常に QueryAndPhysics + WorldStatic Block 状態にある。
	// そのため FindTeleportSpot の EncroachingBlockingGeometry がメッシュボディを拾い、
	// 通常のテレポート先でも常に失敗する。
	// bIsATest=false（実テレポート）では bNoCheck=true を強制して FindTeleportSpot をスキップし、
	// SetWorldLocationAndRotation（常に成功）で移動させる。
	// bIsATest=true（スポット探索の仮チェック）は元の bNoCheck を維持する。
	if (!bIsATest)
	{
		bNoCheck = true;
	}
	return Super::TeleportTo(DestLocation, DestRotation, bIsATest, bNoCheck);
}

void AGarCharacter::TeleportSucceeded(bool bIsATest)
{
	Super::TeleportSucceeded(bIsATest);

	// Actor.TeleportTo(bNoCheck=true) は SetWorldLocationAndRotation(ETeleportType::None) を使うため、
	// メッシュの OnUpdateTransform が None で呼ばれ、シミュレーション中ボディはスキップされる。
	// → PAC コンストレイントがスプリングで引っ張り、遅れてついてくる挙動が発生。
	// TeleportPhysics() でシミュレーション中ボディと TargetActors を即座にスナップして解決する。
	if (!bIsATest && IsValid(PhysicalAnimation))
	{
		PhysicalAnimation->TeleportPhysics(true);
	}
}

void AGarCharacter::BeginPlay()
{
	if(!ensure(IsValid(Settings))) return;
	if(!ensure(IsValid(CharacterMover))) return;
	if(!ensure(IsValid(PhysicalAnimation))) return;
	if(!ensure(IsValid(MotionWarping))) return;
	if(!ensure(AnimationInstance.IsValid())) return;

	Super::BeginPlay();

	// Update states to use the initial desired values.

	RefreshRotationMode();
	ApplyDesiredStance();
	RefreshSprintState();
	RefreshGait();
}

void AGarCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority())
	{
		ClientPossessed(NewController);
	}
}

void AGarCharacter::UnPossessed()
{
	Super::UnPossessed();

	if (HasAuthority())
	{
		ClientUnPossessed();
	}
}

void AGarCharacter::ClientPossessed_Implementation(AController* NewContoller)
{
	OnPossessed_Client.Broadcast(NewContoller);
}

void AGarCharacter::ClientUnPossessed_Implementation()
{
	OnUnPossessed_Client.Broadcast(GetController());
}

FVector AGarCharacter::GetVelocity() const
{
	return GetMover()->GetVelocity();
}

void AGarCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	OnSetupPlayerInputComponent.Broadcast(Input);
}

void AGarCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmd)
{
	// Generate user commands. Called right before the Character movement simulation will tick (for a locally controlled pawn)
	// This isn't meant to be the best way of doing a camera system. It is just meant to show a couple of ways it may be done
	// and to make sure we can keep distinct the movement, rotation, and view angles.
	// Styles 1-3 are really meant to be used with a gamepad.
	//
	// Its worth calling out: the code that happens here is happening *outside* of the Character movement simulation. All we are doing
	// is generating the input being fed into that simulation. That said, this means that A) the code below does not run on the server
	// (and non controlling clients) and B) the code is not rerun during reconcile/resimulates. Use this information guide any
	// decisions about where something should go (such as aim assist, lock on targeting systems, etc): it is hard to give absolute
	// answers and will depend on the game and its specific needs. In general, at this time, I'd recommend aim assist and lock on 
	// targeting systems to happen /outside/ of the system, i.e, here. But I can think of scenarios where that may not be ideal too.

	auto& CharacterInputs{InputCmd.InputCollection.FindOrAddMutableDataByType<FGarCharacterMoverInputs>()};

	if (GetController() == nullptr)
	{
		if (GetLocalRole() == ENetRole::ROLE_Authority && GetRemoteRole() == ENetRole::ROLE_SimulatedProxy)
		{
			static const FGarCharacterMoverInputs DoNothingInput;
			// If we get here, that means this pawn is not currently possessed and we're choosing to provide default do-nothing input
			CharacterInputs = DoNothingInput;
		}

		// We don't have a local controller so we can't run the code below. This is ok. Simulated proxies will just use previous input when extrapolating
		return;
	}

	CharacterInputs.SuggestedMovementMode = NAME_None;
	CharacterInputs.bIsJumpPressed = bIsJumpPressed;
	CharacterInputs.bIsJumpJustPressed = bIsJumpJustPressed;
	CharacterInputs.RotationMode = InputRotationMode;
	CharacterInputs.Stance = InputStance;
	CharacterInputs.Gait = InputGait;

	// Clear/consume temporal movement inputs. We are not consuming others in the event that the game world is ticking at a lower rate than the Mover simulation. 
	// In that case, we want most input to carry over between simulation frames.
	bIsJumpJustPressed = false;

	if (Settings)
	{
		FGameplayTagContainer TempTagContainer;
		AbilitySystem->GetOwnedGameplayTags(TempTagContainer);

		for(auto& KeyValue : Settings->TagToMovementModeMap)
		{
			if (TempTagContainer.HasTagExact(KeyValue.Key) && !PrevTagContainer.HasTagExact(KeyValue.Key))
			{
				CharacterInputs.SuggestedMovementMode = KeyValue.Value;
				break;
			}
			else if (CharacterInputs.SuggestedMovementMode == NAME_None
				&& !TempTagContainer.HasTagExact(KeyValue.Key) && PrevTagContainer.HasTagExact(KeyValue.Key))
			{
				CharacterInputs.SuggestedMovementMode = CharacterMover->StartingMovementMode;
			}
		}

		PrevTagContainer = TempTagContainer;

		if (!TempTagContainer.Filter(Settings->BlockMoveInputTags).IsEmpty())
		{
			return;
		}
	}

	CharacterInputs.ControlRotation = GetControlRotation();

	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MovementInputVector);

	static float RotationMagMin(1e-3);

	const bool bHasAffirmativeMoveInput = (CharacterInputs.GetMoveInput().Size() >= RotationMagMin);
	
	// Figure out intended orientation
	CharacterInputs.OrientationIntent = FVector::ZeroVector;

	if (bHasAffirmativeMoveInput || InputRotationMode == GarRotationModeTags::Aiming
		|| AbilitySystem->HasMatchingGameplayTag(GarPerspectiveTags::FirstPerson))
	{
		if (InputRotationMode == GarRotationModeTags::VelocityDirection)
		{
			// set the intent to the actors movement direction
			CharacterInputs.OrientationIntent = CharacterInputs.GetMoveInput().GetSafeNormal();
		}
		else
		{
			// set intent to the the control rotation - often a player's camera rotation
			CharacterInputs.OrientationIntent = CharacterInputs.ControlRotation.Vector().GetSafeNormal();
		}
	}

	// Convert inputs to be relative to the current movement base (depending on options and state)
	CharacterInputs.bUsingMovementBase = false;

	if (bUseBaseRelativeMovement)
	{
		if (UPrimitiveComponent* MovementBase = CharacterMover->GetMovementBase())
		{
			FName MovementBaseBoneName = CharacterMover->GetMovementBaseBoneName();

			FVector RelativeMoveInput, RelativeOrientDir;

			UBasedMovementUtils::TransformWorldDirectionToBased(MovementBase, MovementBaseBoneName, CharacterInputs.GetMoveInput(), RelativeMoveInput);
			UBasedMovementUtils::TransformWorldDirectionToBased(MovementBase, MovementBaseBoneName, CharacterInputs.OrientationIntent, RelativeOrientDir);

			CharacterInputs.SetMoveInput(CharacterInputs.GetMoveInputType(), RelativeMoveInput);
			CharacterInputs.OrientationIntent = RelativeOrientDir;

			CharacterInputs.bUsingMovementBase = true;
			CharacterInputs.MovementBase = MovementBase;
			CharacterInputs.MovementBaseBoneName = MovementBaseBoneName;
		}
	}
}

void AGarCharacter::Tick(const float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AGarCharacter::Tick()"), STAT_AGarCharacter_Tick, STATGROUP_Gar)

	if (!IsValid(Settings) || !AnimationInstance.IsValid())
	{
		Super::Tick(DeltaTime);
		return;
	}

	TryAdjustControllRotation(DeltaTime);

	RefreshEyeHeight(DeltaTime);
	RefreshCapsuleSize(DeltaTime);
	CheckCanUnCrouchIfNeeded();
	CheckCanCrouchIfNeeded();

	RefreshInput();

	RefreshRotationMode();
	ApplyDesiredStance();
	RefreshSprintState();
	RefreshGait();

	auto LocomotionMode{CharacterMover->GetLocomotionMode()};
	if (LocomotionMode.IsValid())
	{
		for(const auto& Tag : CharacterMover->GetLocomotionModeTags())
		{
			AbilitySystem->SetLooseGameplayTagCount(Tag, LocomotionMode == Tag ? 1 : 0);
		}
	}

	auto& RotationMode{CharacterMover->GetRotationMode()};
	if (RotationMode.IsValid())
	{
		AbilitySystem->SetLooseGameplayTagCount(GarRotationModeTags::ViewDirection, RotationMode == GarRotationModeTags::ViewDirection ? 1 : 0);
		AbilitySystem->SetLooseGameplayTagCount(GarRotationModeTags::VelocityDirection, RotationMode == GarRotationModeTags::VelocityDirection ? 1 : 0);
		AbilitySystem->SetLooseGameplayTagCount(GarRotationModeTags::Aiming, RotationMode == GarRotationModeTags::Aiming ? 1 : 0);
	}

	auto& Stance{CharacterMover->GetStance()};
	if (Stance.IsValid())
	{
		AbilitySystem->SetLooseGameplayTagCount(GarStanceTags::Standing, Stance == GarStanceTags::Standing ? 1 : 0);
		AbilitySystem->SetLooseGameplayTagCount(GarStanceTags::Crouching, Stance == GarStanceTags::Crouching ? 1 : 0);
		AbilitySystem->SetLooseGameplayTagCount(GarStanceTags::LyingFront, Stance == GarStanceTags::LyingFront ? 1 : 0);
		AbilitySystem->SetLooseGameplayTagCount(GarStanceTags::LyingBack, Stance == GarStanceTags::LyingBack ? 1 : 0);
	}

	auto& Gait{CharacterMover->GetGait()};
	if (Gait.IsValid())
	{
		AbilitySystem->SetLooseGameplayTagCount(GarGaitTags::Walking, Gait == GarGaitTags::Walking ? 1 : 0);
		AbilitySystem->SetLooseGameplayTagCount(GarGaitTags::Running, Gait == GarGaitTags::Running ? 1 : 0);
		AbilitySystem->SetLooseGameplayTagCount(GarGaitTags::Sprinting, Gait == GarGaitTags::Sprinting ? 1 : 0);
	}

	Super::Tick(DeltaTime);

	OnTick.Broadcast(DeltaTime);
}

void AGarCharacter::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
	Internal_AddMovementInput(WorldDirection * ScaleValue, bForce);
}

FVector AGarCharacter::ConsumeMovementInputVector()
{
	return Internal_ConsumeMovementInputVector();
}

FRotator AGarCharacter::GetViewRotation() const
{
	return IsLocallyControlled() ? Super::GetViewRotation() : ReplicatedControlRotation;
}

FGameplayTag AGarCharacter::GetLocomotionAction() const
{
	if (IsValid(AbilitySystem))
	{
		FGameplayTagContainer TempTagContainer;
		AbilitySystem->GetOwnedGameplayTags(TempTagContainer);
		return TempTagContainer.Filter(Settings->ActionTags).First();
	}
	return FGameplayTag::EmptyTag;
}

FGameplayTag AGarCharacter::GetPerspective() const
{
	if (IsValid(AbilitySystem))
	{
		FGameplayTagContainer TempTagContainer;
		AbilitySystem->GetOwnedGameplayTags(TempTagContainer);
		return TempTagContainer.Filter(FGameplayTagContainer{GarPerspectiveTags::Root}).First();
	}
	return FGameplayTag::EmptyTag;
}

void AGarCharacter::SetPerspective(const FGameplayTag& NewPerspective)
{
	if (AbilitySystem->HasMatchingGameplayTag(NewPerspective) || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	const auto PreviousPerspective{GetPerspective()};

	AbilitySystem->SetLooseGameplayTagCount(GarPerspectiveTags::ThirdPerson, NewPerspective == GarPerspectiveTags::ThirdPerson ? 1 : 0);
	AbilitySystem->SetLooseGameplayTagCount(GarPerspectiveTags::FirstPerson, NewPerspective == GarPerspectiveTags::FirstPerson ? 1 : 0);

	OnPerspectiveChanged(PreviousPerspective);
}

void AGarCharacter::OnPerspectiveChanged_Implementation(const FGameplayTag& PreviousPerspective) {}

void AGarCharacter::OnLocomotionModeChanged_Implementation(const FGameplayTag& PreviousLocomotionMode) {}

FGameplayTag AGarCharacter::GetDesiredRotationMode() const
{
	if (IsValid(AbilitySystem))
	{
		FGameplayTagContainer TempTagContainer;
		AbilitySystem->GetOwnedGameplayTags(TempTagContainer);
		return TempTagContainer.Filter(FGameplayTagContainer{GarDesiredRotationModeTags::Root}).First();
	}
	return FGameplayTag::EmptyTag;
}

void AGarCharacter::SetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode)
{
	if (AbilitySystem->HasMatchingGameplayTag(NewDesiredRotationMode) || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	AbilitySystem->SetLooseGameplayTagCount(GarDesiredRotationModeTags::ViewDirection,
		NewDesiredRotationMode == GarDesiredRotationModeTags::ViewDirection ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
	AbilitySystem->SetLooseGameplayTagCount(GarDesiredRotationModeTags::VelocityDirection,
		NewDesiredRotationMode == GarDesiredRotationModeTags::VelocityDirection ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
}

void AGarCharacter::RefreshRotationMode()
{
	bool bSprinting{AbilitySystem->HasMatchingGameplayTag(GarGaitTags::Sprinting)};
	bool bAiming{AbilitySystem->HasMatchingGameplayTag(GarAimingModeTags::Root)};

	if (AbilitySystem->HasMatchingGameplayTag(GarPerspectiveTags::FirstPerson))
	{
		if (AbilitySystem->HasMatchingGameplayTag(GarLocomotionModeTags::InAir))
		{
			if (bAiming && Settings->bAllowAimingWhenInAir)
			{
				InputRotationMode = GarRotationModeTags::Aiming;
			}

			return;
		}

		// Grounded and other locomotion modes.

		if (bAiming && (!bSprinting || !Settings->bSprintHasPriorityOverAiming))
		{
			InputRotationMode = GarRotationModeTags::Aiming;
		}
		else
		{
			InputRotationMode = GarRotationModeTags::ViewDirection;
		}

		return;
	}

	// Third person and other view modes.

	if (AbilitySystem->HasMatchingGameplayTag(GarLocomotionModeTags::InAir))
	{
		if (bAiming && Settings->bAllowAimingWhenInAir)
		{
			InputRotationMode = GarRotationModeTags::Aiming;
		}
		else if (bAiming)
		{
			InputRotationMode = GarRotationModeTags::ViewDirection;
		}

		return;
	}

	// Grounded and other locomotion modes.

	if (bSprinting)
	{
		if (bAiming && !Settings->bSprintHasPriorityOverAiming)
		{
			InputRotationMode = GarRotationModeTags::Aiming;
		}
		else if (Settings->bRotateToVelocityWhenSprinting)
		{
			InputRotationMode = GarRotationModeTags::VelocityDirection;
		}
		else if (bAiming)
		{
			InputRotationMode = GarRotationModeTags::ViewDirection;
		}
		else
		{
			InputRotationMode = DesiredToActual(GetDesiredRotationMode());
		}
	}
	else // Not sprinting.
	{
		if (bAiming)
		{
			InputRotationMode = GarRotationModeTags::Aiming;
		}
		else
		{
			InputRotationMode = DesiredToActual(GetDesiredRotationMode());
		}
	}
}

void AGarCharacter::SetDesiredStance(const FGameplayTag& NewDesiredStance)
{
	if (AbilitySystem->HasMatchingGameplayTag(NewDesiredStance) || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	AbilitySystem->SetLooseGameplayTagCount(GarDesiredStanceTags::Standing, NewDesiredStance == GarDesiredStanceTags::Standing ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
	AbilitySystem->SetLooseGameplayTagCount(GarDesiredStanceTags::Crouching, NewDesiredStance == GarDesiredStanceTags::Crouching ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
	AbilitySystem->SetLooseGameplayTagCount(GarDesiredStanceTags::LyingFront, NewDesiredStance == GarDesiredStanceTags::LyingFront ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
	AbilitySystem->SetLooseGameplayTagCount(GarDesiredStanceTags::LyingBack, NewDesiredStance == GarDesiredStanceTags::LyingBack ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
}

void AGarCharacter::ApplyDesiredStance()
{
	if (!GetLocomotionAction().IsValid())
	{
		if (AbilitySystem->HasMatchingGameplayTag(GarLocomotionModeTags::Grounded))
		{
			if (AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::Standing))
			{
				if(CanUnCrouch())
				{
					InputStance = GarStanceTags::Standing;
				}
			}
			else if (AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::Crouching))
			{
				if(CanCrouch())
				{
					InputStance = GarStanceTags::Crouching;
				}
			}
			else if (AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::LyingFront))
			{
				if(CanLie())
				{
					InputStance = GarStanceTags::LyingFront;
				}
			}
			else if (AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::LyingBack))
			{
				if(CanLie())
				{
					InputStance = GarStanceTags::LyingBack;
				}
			}
		}
		else if (AbilitySystem->HasMatchingGameplayTag(GarLocomotionModeTags::InAir))
		{
			if(CanUnCrouch())
			{
				InputStance = GarStanceTags::Standing;
			}
		}
	}
}

void AGarCharacter::CheckCanUnCrouchIfNeeded()
{
	if (!AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::Standing) || AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Standing))
	{
		bUnCrouchBlocked = false;
		return;
	}

	const UWorld* MyWorld = GetWorld();
	const float UpOffset = 5.f;
	FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(UnCrouchTrace), false, this);
	FCollisionResponseParams ResponseParam;
	Capsule->InitSweepCollisionParams(CapsuleParams, ResponseParam);
	float Radius, HalfHeight;
	Capsule->GetScaledCapsuleSize(Radius, HalfHeight);

	// Compensate for the difference between current capsule size and standing size
	const FCollisionShape StandingCapsuleShape = FCollisionShape::MakeCapsule({InitialCapsuleRadius, InitialCapsuleRadius, InitialCapsuleHalfHeight - 0.5f * UpOffset});
	const ECollisionChannel CollisionChannel = Capsule->GetCollisionObjectType();

	auto Location{GetActorLocation() + GetActorUpVector() * (InitialCapsuleHalfHeight + UpOffset - HalfHeight)};

	bUnCrouchBlocked = MyWorld->OverlapBlockingTestByChannel(Location, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
}

void AGarCharacter::CheckCanCrouchIfNeeded()
{
	if (!AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::Crouching) || AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Crouching))
	{
		bCrouchBlocked = false;
		return;
	}

	const UWorld* MyWorld = GetWorld();
	const float UpOffset = 5.f;
	FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(UnCrouchTrace), false, this);
	FCollisionResponseParams ResponseParam;
	Capsule->InitSweepCollisionParams(CapsuleParams, ResponseParam);
	float Radius, HalfHeight;
	Capsule->GetScaledCapsuleSize(Radius, HalfHeight);

	// Compensate for the difference between current capsule size and standing size
	const FCollisionShape StandingCapsuleShape = FCollisionShape::MakeCapsule({InitialCapsuleRadius, InitialCapsuleRadius, CrouchedCapsuleHalfHeight - 0.5f * UpOffset});
	const ECollisionChannel CollisionChannel = Capsule->GetCollisionObjectType();

	auto Location{GetActorLocation() + GetActorUpVector() * (CrouchedCapsuleHalfHeight + UpOffset - HalfHeight)};

	bCrouchBlocked = MyWorld->OverlapBlockingTestByChannel(Location, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
}

void AGarCharacter::CheckCanLieIfNeeded()
{
	if (!AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::Lying) || AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Lying))
	{
		bLieBlocked = false;
		return;
	}
}

bool AGarCharacter::CanCrouch_Implementation() const
{
	return !bCrouchBlocked;
}

bool AGarCharacter::CanUnCrouch_Implementation() const
{
	return !bUnCrouchBlocked;
}

bool AGarCharacter::CanLie_Implementation() const
{
	return !bLieBlocked;
}

void AGarCharacter::Crouch()
{
	SetDesiredStance(GarDesiredStanceTags::Crouching);
}

void AGarCharacter::UnCrouch()
{
	SetDesiredStance(GarDesiredStanceTags::Standing);
}

void AGarCharacter::Prone()
{
	SetDesiredStance(GarDesiredStanceTags::LyingFront);
}

void AGarCharacter::Supine()
{
	SetDesiredStance(GarDesiredStanceTags::LyingBack);
}

bool AGarCharacter::IsCrouching() const
{
	return AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::Crouching);
}

bool AGarCharacter::IsLying() const
{
	return AbilitySystem->HasMatchingGameplayTag(GarDesiredStanceTags::Lying);
}

void AGarCharacter::SetInputStance(const FGameplayTag & NewInputStance)
{
	InputStance = NewInputStance;
}

bool AGarCharacter::UpdateMainCapsule(float DeltaTime, float TargetHalfHeight, float HeightSpeed, float TargetRadius, float RadiusSpeed)
{
	TargetRadius = FMath::Max(0.f, TargetRadius);
	TargetHalfHeight = FMath::Max3(0.f, TargetRadius, TargetHalfHeight);

	const float OldHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	const float OldRadius = Capsule->GetUnscaledCapsuleRadius();
	const float HalfHeight = FMath::FInterpConstantTo(OldHalfHeight, TargetHalfHeight, DeltaTime, HeightSpeed);
	const float Radius = FMath::FInterpConstantTo(OldRadius, TargetRadius, DeltaTime, RadiusSpeed);
	
	if (OldHalfHeight != HalfHeight || OldRadius != Radius)
	{
		double Scale{Capsule->GetComponentTransform().GetScale3D().Z};
		// Now call SetCapsuleSize() to cause touch/untouch events and actually grow the capsule
		Capsule->SetCapsuleSize(Radius, HalfHeight, false);

		// Add offset to visual component as the base location has changed

		if (GetLocalRole() <= ROLE_SimulatedProxy)
		{
			if (Mesh)
			{
				Mesh->GetRelativeLocation_DirectMutable().Z = InitialMeshZ
					+ (HalfHeight < Radius ? InitialCapsuleRadius - Radius : InitialCapsuleHalfHeight - HalfHeight) * Scale;
			}
		}
		else
		{
			auto TeleportEffect = MakeShared<FTeleportEffect>();
			TeleportEffect->TargetLocation = CharacterMover->GetUpdatedComponentTransform().GetLocation()
				+ CharacterMover->GetUpDirection() * (HalfHeight < Radius ? Radius - OldRadius : HalfHeight - OldHalfHeight) * Scale;
			CharacterMover->QueueInstantMovementEffect(TeleportEffect);

			if (Mesh)
			{
				auto MoverVisualComponentOffset = CharacterMover->GetBaseVisualComponentTransform();
				auto Location{MoverVisualComponentOffset.GetLocation()};
				Location.Z = InitialMeshZ + (HalfHeight < Radius ? InitialCapsuleRadius - Radius : InitialCapsuleHalfHeight - HalfHeight) * Scale;
				MoverVisualComponentOffset.SetLocation(Location);
				CharacterMover->SetBaseVisualComponentTransform(MoverVisualComponentOffset);
			}
		}
		return true;
	}
	return false;
}

bool AGarCharacter::UpdateProneCapsule(float DeltaTime, float TargetHalfHeight, float HeightSpeed, float TargetRadius, float RadiusSpeed,
	float TargetOffset, float OffsetSpeed)
{
	TargetRadius = FMath::Max(0.f, TargetRadius);
	TargetHalfHeight = FMath::Max3(0.f, TargetRadius, TargetHalfHeight);

	const float OldHalfHeight = ProneCapsule->GetUnscaledCapsuleHalfHeight();
	const float OldRadius = ProneCapsule->GetUnscaledCapsuleRadius();
	const float OldOffsetX = ProneCapsule->GetRelativeLocation().X;
	const float HalfHeight = FMath::FInterpConstantTo(OldHalfHeight, TargetHalfHeight, DeltaTime, HeightSpeed);
	const float Radius = FMath::FInterpConstantTo(OldRadius, TargetRadius, DeltaTime, RadiusSpeed);
	const float OffsetX = FMath::FInterpConstantTo(OldOffsetX, TargetOffset, DeltaTime, OffsetSpeed);

	if (OldHalfHeight != HalfHeight || OldRadius != Radius || OldOffsetX != OffsetX)
	{
		// Now call SetCapsuleSize() to cause touch/untouch events and actually grow the capsule
		ProneCapsule->SetCapsuleSize(Radius, HalfHeight, false);
		ProneCapsule->GetRelativeLocation_DirectMutable().X = OffsetX;
		return true;
	}
	return false;
}

void AGarCharacter::RefreshCapsuleSize(float DeltaTime)
{
	if (AbilitySystem->HasMatchingGameplayTag(GarStateFlagTags::BlockUpdateCapsuleSize))
	{
		return;
	}

	// Update capsule height and radius
	auto CapsuleUpdateSpeed{Settings->CapsuleUpdateSpeed};
	auto ProneHalfHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialProneCapsuleHalfHeight - LiedProneCapsuleHalfHeight) / CapsuleUpdateSpeed : .0f};
	auto OffsetSpeed{CapsuleUpdateSpeed > 0 ? LiedProneCapsuleZOffset / CapsuleUpdateSpeed : .0f};
	if (AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Lying))
	{
		auto HalfHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(CrouchedCapsuleHalfHeight - LiedCapsuleHalfHeight) / CapsuleUpdateSpeed : .0f};
		UpdateMainCapsule(DeltaTime, LiedCapsuleHalfHeight, HalfHeightSpeed, InitialCapsuleRadius, 0.f);
		UpdateProneCapsule(DeltaTime, LiedProneCapsuleHalfHeight, ProneHalfHeightSpeed, InitialProneCapsuleRadius, 0.0f,
			InitialProneCapsuleX + LiedProneCapsuleZOffset, OffsetSpeed);
		if (!ProneCapsule->IsWelded())
		{
			ProneCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ProneCapsule->WeldTo(Capsule, NAME_None, true);
		}
	}
	else if (AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Crouching))
	{
		auto HalfHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialCapsuleHalfHeight - CrouchedCapsuleHalfHeight) / CapsuleUpdateSpeed : .0f};
		UpdateMainCapsule(DeltaTime, CrouchedCapsuleHalfHeight, HalfHeightSpeed, InitialCapsuleRadius, 0.f);
		if (!UpdateProneCapsule(DeltaTime, InitialProneCapsuleHalfHeight, ProneHalfHeightSpeed, InitialProneCapsuleRadius, 0.0f,
			InitialProneCapsuleX, OffsetSpeed))
		{
			if (ProneCapsule->IsWelded())
			{
				ProneCapsule->UnWeldFromParent();
				ProneCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
	else if (AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Standing))
	{
		auto HalfHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialCapsuleHalfHeight - CrouchedCapsuleHalfHeight) / CapsuleUpdateSpeed : .0f};
		UpdateMainCapsule(DeltaTime, InitialCapsuleHalfHeight, HalfHeightSpeed, InitialCapsuleRadius, 0.f);
		if (!UpdateProneCapsule(DeltaTime, InitialProneCapsuleHalfHeight, ProneHalfHeightSpeed, InitialProneCapsuleRadius, 0.0f,
			InitialProneCapsuleX, OffsetSpeed))
		{
			if (ProneCapsule->IsWelded())
			{
				ProneCapsule->UnWeldFromParent();
				ProneCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}

void AGarCharacter::RefreshEyeHeight(float DeltaTime)
{
	// Update eye height

	auto CapsuleUpdateSpeed{Settings->CapsuleUpdateSpeed};
	if (AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Lying))
	{
		auto EyeHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(CrouchedEyeHeight - LiedEyeHeight) / CapsuleUpdateSpeed : .0f};
		BaseEyeHeight = FMath::FInterpConstantTo(BaseEyeHeight, LiedEyeHeight, DeltaTime, EyeHeightSpeed);
	}
	else if (AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Crouching))
	{
		auto EyeHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialEyeHeight - CrouchedEyeHeight) / CapsuleUpdateSpeed : .0f};
		BaseEyeHeight = FMath::FInterpConstantTo(BaseEyeHeight, CrouchedEyeHeight, DeltaTime, EyeHeightSpeed);
	}
	else if(AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Standing))
	{
		auto EyeHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialEyeHeight - CrouchedEyeHeight) / CapsuleUpdateSpeed : .0f};
		BaseEyeHeight = FMath::FInterpConstantTo(BaseEyeHeight, InitialEyeHeight, DeltaTime, EyeHeightSpeed);
	}
}

FGameplayTag AGarCharacter::GetDesiredGait() const
{
	if (IsValid(AbilitySystem))
	{
		FGameplayTagContainer TempTagContainer;
		AbilitySystem->GetOwnedGameplayTags(TempTagContainer);
		return TempTagContainer.Filter(FGameplayTagContainer{GarDesiredGaitTags::Root}).First();
	}
	return FGameplayTag::EmptyTag;
}

void AGarCharacter::SetDesiredGait(const FGameplayTag& NewDesiredGait)
{
	if (AbilitySystem->HasMatchingGameplayTag(NewDesiredGait) || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	AbilitySystem->SetLooseGameplayTagCount(GarDesiredGaitTags::Walking, NewDesiredGait == GarDesiredGaitTags::Walking ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
	AbilitySystem->SetLooseGameplayTagCount(GarDesiredGaitTags::Running, NewDesiredGait == GarDesiredGaitTags::Running ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
	AbilitySystem->SetLooseGameplayTagCount(GarDesiredGaitTags::Sprinting, NewDesiredGait == GarDesiredGaitTags::Sprinting ? 1 : 0,
		EGameplayTagReplicationState::TagAndCountToAll);
}

FGameplayTag AGarCharacter::LimitGaitIfNeeded_Implementation(const FGameplayTag& NewGait) const
{
	// Calculate the max allowed gait. This represents the maximum gait the character is currently allowed
	// to be in and can be determined by the desired gait, the rotation mode, the stance, etc. For example,
	// if you wanted to force the character into a walking state while indoors, this could be done here.

	if (NewGait == GarGaitTags::Sprinting && CanSprint())
	{
		return GarGaitTags::Sprinting;
	}

	if (NewGait == GarGaitTags::Walking)
	{
		return GarGaitTags::Walking;
	}

	return MovementInputVector.Size2D() < (AbilitySystem->HasMatchingGameplayTag(GarGaitTags::Running) ? 0.5 : 0.75)
		? GarGaitTags::Walking : GarGaitTags::Running;
}

void AGarCharacter::RefreshGait()
{
	if (!AbilitySystem->HasMatchingGameplayTag(GarLocomotionModeTags::Grounded))
	{
		return;
	}

	InputGait = LimitGaitIfNeeded(DesiredToActual(GetDesiredGait()));
}

bool AGarCharacter::CanSprint_Implementation() const
{
	// Determine if the character can sprint based on the rotation mode and input direction.
	// If the character is in view direction rotation mode, only allow sprinting if there is
	// input and if the input direction is aligned with the view direction within 50 degrees.

	if (!HasSpeed() || !AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Standing)
		|| (AbilitySystem->HasMatchingGameplayTag(GarRotationModeTags::Aiming) && !Settings->bSprintHasPriorityOverAiming))
	{
		return false;
	}

	if (!AbilitySystem->HasMatchingGameplayTag(GarPerspectiveTags::FirstPerson)
		&& (AbilitySystem->HasMatchingGameplayTag(GarDesiredRotationModeTags::VelocityDirection) || Settings->bRotateToVelocityWhenSprinting))
	{
		return true;
	}

	auto ViewRelativeAngle = FMath::Abs(FMath::UnwindDegrees(UE_REAL_TO_FLOAT(InputYawAngle - GetActorRotation().Yaw)));
	if (ViewRelativeAngle < Settings->ViewRelativeAngleThresholdForSprint)
	{
		return true;
	}

	return false;
}

void AGarCharacter::RefreshInput()
{
	if (IsLocallyControlled())
	{
		MovementInputVector = ConsumeMovementInputVector();

		if (HasMovementInput())
		{
			InputDirection = MovementInputVector.GetUnsafeNormal2D();
			InputYawAngle = UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(InputDirection));
		}
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		auto ControlRotation{Controller->GetControlRotation()};
		if (!ReplicatedControlRotation.Equals(ControlRotation, 0.001))
		{
			ReplicatedControlRotation = ControlRotation;
			ServerSetControlRotation(ReplicatedControlRotation);
		}
	}
	else if(HasAuthority())
	{
		auto ControlRotation{Controller->GetControlRotation()};
		if (!ReplicatedControlRotation.Equals(ControlRotation, 0.001))
		{
			ReplicatedControlRotation = ControlRotation;
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedControlRotation, this)
		}
	}
}

void AGarCharacter::ServerSetControlRotation_Implementation(const FRotator& NewControlRotation)
{
	Controller->SetControlRotation(NewControlRotation);
}

void AGarCharacter::SetFocalRotation(const FRotator& NewFocalRotation)
{
	if (IsLocallyControlled())
	{
		PendingFocalRotationRelativeAdjustment = (NewFocalRotation - GetViewRotation()).GetNormalized();
		PendingFocalRotationRelativeAdjustment.Yaw = FMath::Clamp(PendingFocalRotationRelativeAdjustment.Yaw, -90.0, 90.0);
		PendingFocalRotationRelativeAdjustment.Pitch = FMath::Clamp(PendingFocalRotationRelativeAdjustment.Pitch, -45.0, 45.0);
		PendingFocalRotationRelativeAdjustment.Roll = 0.0;
		UE_LOG(LogGar, Verbose, TEXT("SetFocalRotation PendingFocalRotationRelativeAdjustment %s"), *PendingFocalRotationRelativeAdjustment.ToString());
	}
}

void AGarCharacter::TryAdjustControllRotation(float DeltaTime)
{
	if (!IsLocallyControlled() || !IsValid(GetController()))
	{
		return;
	}
	if (!PendingFocalRotationRelativeAdjustment.IsNearlyZero(0.01))
	{
		const auto ControlRotation{Controller->GetControlRotation()};
		const auto PreviousPendingFocalRotationRelativeAdjustment{PendingFocalRotationRelativeAdjustment};
		auto NewControlRotation{FMath::RInterpTo(ControlRotation,
			ControlRotation + PendingFocalRotationRelativeAdjustment,
			DeltaTime,
			Settings->AdjustControllRotationSpeed)};
		NewControlRotation.Pitch = FMath::ClampAngle(NewControlRotation.Pitch, -89.0f, 89.0f);
		Controller->SetControlRotation(NewControlRotation);
		PendingFocalRotationRelativeAdjustment -= Controller->GetControlRotation() - ControlRotation;
		PendingFocalRotationRelativeAdjustment.Normalize();
		UE_LOG(LogGar, Verbose, TEXT("Applay PendingFocalRotationRelativeAdjustment %s %s"),
			*(PendingFocalRotationRelativeAdjustment - PreviousPendingFocalRotationRelativeAdjustment).ToString(),
			*PendingFocalRotationRelativeAdjustment.ToString());
	}
	else
	{
		auto ControlRotation{Controller->GetControlRotation()};
		ControlRotation.Pitch = FMath::ClampAngle(ControlRotation.Pitch, -89.0f, 89.0f);
		Controller->SetControlRotation(ControlRotation);
	}
}

bool AGarCharacter::IsMoving() const
{
	auto Speed{GetVelocity().Size2D()};
	return (HasMovementInput() && Speed >= 1.0) || Speed > Settings->MovingSpeedThreshold;
}

void AGarCharacter::RefreshSprintState()
{
	if (Settings->bAutoTurnOffSprint
		&& (GetLocomotionAction().IsValid() || AbilitySystem->HasMatchingGameplayTag(GarLocomotionModeTags::Grounded))
		&& (GetVelocity().Size2D() < Settings->SprintOffSpeed || MovementInputVector.Size2D() < 0.75f)
		&& GetDesiredGait() == GarDesiredGaitTags::Sprinting)
	{
		SetDesiredGait(GarDesiredGaitTags::Running);
	}
}

bool AGarCharacter::CanJump_Implementation() const
{
	return AbilitySystem->HasMatchingGameplayTag(GarStanceTags::Standing)
		&& !GetLocomotionAction().IsValid() && AbilitySystem->HasMatchingGameplayTag(GarLocomotionModeTags::Grounded);
}

void AGarCharacter::Jump()
{
	if (CanJump())
	{
		bIsJumpJustPressed = true;
		bIsJumpPressed = true;
	}
}

void AGarCharacter::StopJumping()
{
	bIsJumpPressed = false;
}

float AGarCharacter::GetAimAmount() const
{
	return AnimationInstance.IsValid() ? AnimationInstance->GetCurveValueClamped01(UGarConstants::PoseAimingCurveName()) : 0.0f;
}

const FGameplayTag& AGarCharacter::DesiredToActual(const FGameplayTag& SourceTag) const
{
	if (SourceTag.IsValid() && IsValid(Settings))
	{
		auto* Value{Settings->DesiredToActualMap.Find(SourceTag)};
		if (Value)
		{
			return *Value;
		}
	}
	return SourceTag;
}
