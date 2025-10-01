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
#include "Settings/GarCharacterSettings.h"
#include "State/GarCharacterMoverInputs.h"
#include "GarAnimationInstance.h"
#include "GarCharacterMoverComponent.h"
#include "GarPhysicalAnimationComponent.h"
#include "GarAbilitySystemComponent.h"
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
}

// IAbilitySystemInterface

UAbilitySystemComponent* AGarCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AGarCharacter::AppendOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (DesiredRotationMode.IsValid())
	{
		TagContainer.AddLeafTag(DesiredRotationMode);
	}
	if (DesiredStance.IsValid())
	{
		TagContainer.AddLeafTag(DesiredStance);
	}
	if (DesiredGait.IsValid())
	{
		TagContainer.AddLeafTag(DesiredGait);
	}
	if (Perspective.IsValid())
	{
		TagContainer.AddLeafTag(Perspective);
	}
	if (OverlayMode.IsValid())
	{
		TagContainer.AddLeafTag(OverlayMode);
	}
}

void AGarCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredStance, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredGait, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredRotationMode, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OverlayMode, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedControlRotation, Parameters)
}

void AGarCharacter::PreRegisterAllComponents()
{
	// Set some default values here so that the animation instance and the
	// camera component can read the most up-to-date values during initialization.

	if (IsValid(Settings))
	{
		InputRotationMode = DesiredToActual(DesiredRotationMode);
		InputStance = DesiredToActual(DesiredStance);
		InputGait = DesiredToActual(DesiredGait);
	}

	Super::PreRegisterAllComponents();
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
		CharacterMover->SetInitialGameplayTags(InputRotationMode, InputStance, InputGait);
		if (USceneComponent* UpdatedComponent = CharacterMover->GetUpdatedComponent())
		{
			UpdatedComponent->SetCanEverAffectNavigation(bCanAffectNavigationGeneration);
		}
	}

	if (PhysicalAnimation)
	{
		PhysicalAnimation->SetSkeletalMeshComponent(Mesh);
	}

	if (IsValid(AbilitySystem))
	{
		AbilitySystem->Initialize(this);
	}

	Super::PostInitializeComponents();

	Capsule->GetUnscaledCapsuleSize(InitialCapsuleRadius, InitialCapsuleHalfHeight);
	ProneCapsule->GetUnscaledCapsuleSize(InitialProneCapsuleRadius, InitialProneCapsuleHalfHeight);
	InitialEyeHeight = BaseEyeHeight;
	InitialMeshZ = Mesh->GetRelativeLocation().Z;
	InitialProneCapsuleX = ProneCapsule->GetRelativeLocation().X;
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

	// Enable view network smoothing on the listen server here because the remote role may not be valid yet during begin play.

	if (GetLocalRole() >= ROLE_Authority)
	{
		ClientPossessed(NewController);
	}
}

void AGarCharacter::UnPossessed()
{
	Super::UnPossessed();

	if (GetLocalRole() >= ROLE_Authority)
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
		GetOwnedGameplayTags(TempTagContainer);

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

	if (bHasAffirmativeMoveInput || InputRotationMode == GarRotationModeTags::Aiming || Perspective == GarPerspectiveTags::FirstPerson)
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

	OnRefresh.Broadcast(DeltaTime);

	Super::Tick(DeltaTime);
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

void AGarCharacter::SetOverlayMode(const FGameplayTag& NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode, true);
}

void AGarCharacter::SetOverlayMode(const FGameplayTag& NewOverlayMode, const bool bSendRpc)
{
	if (OverlayMode == NewOverlayMode || GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}

	const auto PreviousOverlayMode{OverlayMode};

	OverlayMode = NewOverlayMode;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OverlayMode, this)

	OnOverlayModeChanged.Broadcast(PreviousOverlayMode);

	if (bSendRpc)
	{
		if (GetLocalRole() >= ROLE_Authority)
		{
			ClientSetOverlayMode(OverlayMode);
		}
		else
		{
			ServerSetOverlayMode(OverlayMode);
		}
	}
}

void AGarCharacter::ClientSetOverlayMode_Implementation(const FGameplayTag& NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode, false);
}

void AGarCharacter::ServerSetOverlayMode_Implementation(const FGameplayTag& NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode, false);
}

void AGarCharacter::OnReplicated_OverlayMode(const FGameplayTag& PreviousOverlayMode) const
{
	OnOverlayModeChanged.Broadcast(PreviousOverlayMode);
}

FGameplayTag AGarCharacter::GetLocomotionAction() const
{
	if (IsValid(AbilitySystem))
	{
		FGameplayTagContainer TempTagContainer;
		AbilitySystem->GetOwnedGameplayTags_Super(TempTagContainer);
		return TempTagContainer.Filter(Settings->ActionTags).First();
	}
	return FGameplayTag::EmptyTag;
}

void AGarCharacter::SetPerspective(const FGameplayTag& NewPerspective)
{
	if (Perspective == NewPerspective)
	{
		return;
	}

	const auto PreviousPerspective{Perspective};

	Perspective = NewPerspective;

	OnPerspectiveChanged(PreviousPerspective);
}

void AGarCharacter::OnPerspectiveChanged_Implementation(const FGameplayTag& PreviousPerspective) {}

FGameplayTag AGarCharacter::GetLocomotionMode() const
{
	return CharacterMover->GetLocomotionMode();
}

void AGarCharacter::OnLocomotionModeChanged_Implementation(const FGameplayTag& PreviousLocomotionMode) {}

void AGarCharacter::SetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode)
{
	if (DesiredRotationMode == NewDesiredRotationMode || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	DesiredRotationMode = NewDesiredRotationMode;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredRotationMode, this)

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetDesiredRotationMode(DesiredRotationMode);
	}
}

void AGarCharacter::ServerSetDesiredRotationMode_Implementation(const FGameplayTag& NewDesiredRotationMode)
{
	SetDesiredRotationMode(NewDesiredRotationMode);
}

FGameplayTag AGarCharacter::GetRotationMode() const
{
	return CharacterMover->GetRotationMode();
}

void AGarCharacter::RefreshRotationMode()
{
	bool bSprinting{GetGait() == GarGaitTags::Sprinting};
	bool bAiming{HasMatchingGameplayTag(GarAimingModeTags::Root)};

	if (Perspective == GarPerspectiveTags::FirstPerson)
	{
		if (GetLocomotionMode() == GarLocomotionModeTags::InAir)
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

	if (GetLocomotionMode() == GarLocomotionModeTags::InAir)
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
			InputRotationMode = DesiredToActual(DesiredRotationMode);
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
			InputRotationMode = DesiredToActual(DesiredRotationMode);
		}
	}
}

void AGarCharacter::SetDesiredStance(const FGameplayTag& NewDesiredStance)
{
	if (DesiredStance == NewDesiredStance || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	DesiredStance = NewDesiredStance;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredStance, this)

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetDesiredStance(DesiredStance);
	}
}

void AGarCharacter::ServerSetDesiredStance_Implementation(const FGameplayTag& NewDesiredStance)
{
	SetDesiredStance(NewDesiredStance);
}

void AGarCharacter::ApplyDesiredStance()
{
	if (!GetLocomotionAction().IsValid())
	{
		auto LocomotionMode{GetLocomotionMode()};
		if (LocomotionMode == GarLocomotionModeTags::Grounded)
		{
			if (DesiredStance == GarDesiredStanceTags::Standing)
			{
				if(CanUnCrouch())
				{
					InputStance = GarStanceTags::Standing;
				}
			}
			else if (DesiredStance == GarDesiredStanceTags::Crouching)
			{
				if(CanCrouch())
				{
					InputStance = GarStanceTags::Crouching;
				}
			}
			else if (DesiredStance == GarDesiredStanceTags::Lying)
			{
				if(CanLie())
				{
					InputStance = GarStanceTags::Lying;
				}
			}
		}
		else if (LocomotionMode == GarLocomotionModeTags::InAir)
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
	if (DesiredStance != GarDesiredStanceTags::Standing || GetStance() == GarStanceTags::Standing)
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
	if (DesiredStance != GarDesiredStanceTags::Crouching || GetStance() == GarStanceTags::Crouching)
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
	if (DesiredStance != GarDesiredStanceTags::Lying || GetStance() == GarStanceTags::Lying)
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

void AGarCharacter::Lie()
{
	SetDesiredStance(GarDesiredStanceTags::Lying);
}

FGameplayTag AGarCharacter::GetStance() const
{
	return CharacterMover->GetStance();
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
	if (HasMatchingGameplayTag(GarStateFlagTags::BlockUpdateCapsuleSize))
	{
		return;
	}

	// Update capsule height and radius
	auto Stance{GetStance()};
	auto CapsuleUpdateSpeed{Settings->CapsuleUpdateSpeed};
	auto ProneHalfHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialProneCapsuleHalfHeight - LiedProneCapsuleHalfHeight) / CapsuleUpdateSpeed : .0f};
	auto OffsetSpeed{CapsuleUpdateSpeed > 0 ? LiedProneCapsuleZOffset / CapsuleUpdateSpeed : .0f};
	if (Stance == GarStanceTags::Lying)
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
	else if (Stance == GarStanceTags::Crouching)
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
	else
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

	auto Stance{GetStance()};
	auto CapsuleUpdateSpeed{Settings->CapsuleUpdateSpeed};
	if (Stance == GarStanceTags::Lying)
	{
		auto EyeHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(CrouchedEyeHeight - LiedEyeHeight) / CapsuleUpdateSpeed : .0f};
		BaseEyeHeight = FMath::FInterpConstantTo(BaseEyeHeight, LiedEyeHeight, DeltaTime, EyeHeightSpeed);
	}
	else if (Stance == GarStanceTags::Crouching)
	{
		auto EyeHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialEyeHeight - CrouchedEyeHeight) / CapsuleUpdateSpeed : .0f};
		BaseEyeHeight = FMath::FInterpConstantTo(BaseEyeHeight, CrouchedEyeHeight, DeltaTime, EyeHeightSpeed);
	}
	else
	{
		auto EyeHeightSpeed{CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialEyeHeight - CrouchedEyeHeight) / CapsuleUpdateSpeed : .0f};
		BaseEyeHeight = FMath::FInterpConstantTo(BaseEyeHeight, InitialEyeHeight, DeltaTime, EyeHeightSpeed);
	}
}

void AGarCharacter::SetDesiredGait(const FGameplayTag& NewDesiredGait)
{
	if (DesiredGait == NewDesiredGait || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	DesiredGait = NewDesiredGait;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredGait, this)

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetDesiredGait(DesiredGait);
	}
}

void AGarCharacter::ServerSetDesiredGait_Implementation(const FGameplayTag& NewDesiredGait)
{
	SetDesiredGait(NewDesiredGait);
}

FGameplayTag AGarCharacter::GetGait() const
{
	return CharacterMover->GetGait();
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

	return MovementInputVector.Size2D() < (GetGait() == GarGaitTags::Running ? 0.5 : 0.75) ? GarGaitTags::Walking : GarGaitTags::Running;
}

void AGarCharacter::RefreshGait()
{
	if (GetLocomotionMode() != GarLocomotionModeTags::Grounded)
	{
		return;
	}

	InputGait = LimitGaitIfNeeded(DesiredToActual(DesiredGait));
}

bool AGarCharacter::CanSprint_Implementation() const
{
	// Determine if the character can sprint based on the rotation mode and input direction.
	// If the character is in view direction rotation mode, only allow sprinting if there is
	// input and if the input direction is aligned with the view direction within 50 degrees.

	if (!HasSpeed() || GetStance() != GarStanceTags::Standing || (GetRotationMode() == GarRotationModeTags::Aiming && !Settings->bSprintHasPriorityOverAiming))
	{
		return false;
	}

	if (Perspective != GarPerspectiveTags::FirstPerson && (DesiredRotationMode == GarDesiredRotationModeTags::VelocityDirection || Settings->bRotateToVelocityWhenSprinting))
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
	MovementInputVector = ConsumeMovementInputVector();

	if (HasMovementInput())
	{
		InputDirection = MovementInputVector.GetUnsafeNormal2D();
		InputYawAngle = UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(InputDirection));
	}

	if (IsLocallyControlled())
	{
		SetReplicatedControlRotation(GetControlRotation());
	}
}

void AGarCharacter::SetReplicatedControlRotation(const FRotator& NewControlRotation)
{
	if (ReplicatedControlRotation == NewControlRotation || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	ReplicatedControlRotation = NewControlRotation;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedControlRotation, this)

	if(GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetReplicatedControlRotation(ReplicatedControlRotation);
	}
}

void AGarCharacter::ServerSetReplicatedControlRotation_Implementation(const FRotator& NewControlRotation)
{
	SetReplicatedControlRotation(NewControlRotation);
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
	if (IsLocallyControlled() && IsValid(GetController()) && !PendingFocalRotationRelativeAdjustment.IsNearlyZero(0.01))
	{
		const auto ControlRotation{Controller->GetControlRotation()};
		const auto PreviousPendingFocalRotationRelativeAdjustment{PendingFocalRotationRelativeAdjustment};
		Controller->SetControlRotation(FMath::RInterpTo(ControlRotation,
														ControlRotation + PendingFocalRotationRelativeAdjustment,
														DeltaTime,
														Settings->AdjustControllRotationSpeed));
		PendingFocalRotationRelativeAdjustment -= Controller->GetControlRotation() - ControlRotation;
		PendingFocalRotationRelativeAdjustment.Normalize();
		UE_LOG(LogGar, Verbose, TEXT("Applay PendingFocalRotationRelativeAdjustment %s %s"),
			   *(PendingFocalRotationRelativeAdjustment - PreviousPendingFocalRotationRelativeAdjustment).ToString(),
			   *PendingFocalRotationRelativeAdjustment.ToString());
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
		&& (GetLocomotionAction().IsValid() || GetLocomotionMode() == GarLocomotionModeTags::Grounded)
		&& (GetVelocity().Size2D() < Settings->SprintOffSpeed || MovementInputVector.Size2D() < 0.75f)
		&& GetDesiredGait() == GarDesiredGaitTags::Sprinting)
	{
		SetDesiredGait(GarDesiredGaitTags::Running);
	}
}

bool AGarCharacter::CanJump_Implementation() const
{
	return GetStance() == GarStanceTags::Standing && !GetLocomotionAction().IsValid() && GetLocomotionMode() == GarLocomotionModeTags::Grounded;
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

bool AGarCharacter::IsCharacterSelf() const
{
	auto NetMode{GetWorld()->GetNetMode()};
	return NetMode == NM_Standalone
		|| (NetMode == NM_ListenServer && GetLocalRole() == ROLE_Authority)
		|| (NetMode == NM_Client && GetLocalRole() == ROLE_AutonomousProxy);
}

bool AGarCharacter::HasServerRole() const
{
	auto NetMode{GetWorld()->GetNetMode()};
	return (NetMode == NM_DedicatedServer || NetMode == NM_ListenServer) && GetLocalRole() == ROLE_Authority;
}
