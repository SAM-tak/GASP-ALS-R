#include "GarCharacter.h"

#include "GarAnimationInstance.h"
#include "GarCharacterMovementComponent.h"
#include "GarPhysicalAnimationComponent.h"
#include "GarAbilitySystemComponent.h"
#include "GarMotionWarpingComponent.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/GameNetworkManager.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Settings/GarCharacterSettings.h"
#include "GarConstants.h"
#include "Utility/GarUtility.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarCharacter)

namespace GarCharacterConstants
{
	constexpr auto TeleportDistanceThresholdSquared{FMath::Square(50.0f)};
}

FName AGarCharacter::PhysicalAnimationComponentName(TEXT("PhysicalAnimComp"));
FName AGarCharacter::AbilitySystemComponentName(TEXT("AbilitySystemComp"));
FName AGarCharacter::MotionWarpingComponentName(TEXT("MotionWarpComp"));

AGarCharacter::AGarCharacter(const FObjectInitializer& ObjectInitializer) : Super{
	ObjectInitializer.SetDefaultSubobjectClass<UGarCharacterMovementComponent>(CharacterMovementComponentName)
}
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bClientCheckEncroachmentOnNetUpdate = true; // Required for bSimGravityDisabled to be updated.

	CapsuleUpdateSpeed = 0.3f;
	bIsLied = false;

	GetCapsuleComponent()->InitCapsuleSize(30.0f, 90.0f);

	if (IsValid(GetMesh()))
	{
		GetMesh()->SetRelativeLocation_Direct({0.0f, 0.0f, -92.0f});
		GetMesh()->SetRelativeRotation_Direct({0.0f, -90.0f, 0.0f});

		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
		GetMesh()->bEnableUpdateRateOptimizations = false;
	}

	GarCharacterMovement = Cast<UGarCharacterMovementComponent>(GetCharacterMovement());

	PhysicalAnimation = CreateDefaultSubobject<UGarPhysicalAnimationComponent>(PhysicalAnimationComponentName);

	AbilitySystem = CreateOptionalDefaultSubobject<UGarAbilitySystemComponent>(AbilitySystemComponentName);

	MotionWarping = CreateDefaultSubobject<UGarMotionWarpingComponent>(MotionWarpingComponentName);

	// This will prevent the editor from combining component details with actor details.
	// Component details can still be accessed from the actor's component hierarchy.

#if WITH_EDITOR
	StaticClass()->FindPropertyByName(FName{TEXTVIEW("Mesh")})->SetPropertyFlags(CPF_DisableEditOnInstance);
	StaticClass()->FindPropertyByName(FName{TEXTVIEW("CapsuleComponent")})->SetPropertyFlags(CPF_DisableEditOnInstance);
	StaticClass()->FindPropertyByName(FName{TEXTVIEW("CharacterMovement")})->SetPropertyFlags(CPF_DisableEditOnInstance);
#endif
}

#if WITH_EDITOR
bool AGarCharacter::CanEditChange(const FProperty* Property) const
{
	return Super::CanEditChange(Property) &&
		   Property->GetFName() != GET_MEMBER_NAME_CHECKED(ThisClass, bUseControllerRotationPitch) &&
		   Property->GetFName() != GET_MEMBER_NAME_CHECKED(ThisClass, bUseControllerRotationYaw) &&
		   Property->GetFName() != GET_MEMBER_NAME_CHECKED(ThisClass, bUseControllerRotationRoll);
}
#endif

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
	if (LocomotionMode.IsValid())
	{
		TagContainer.AddLeafTag(LocomotionMode);
	}
	if (GetRotationMode().IsValid())
	{
		TagContainer.AddLeafTag(GetRotationMode());
	}
	if (GetStance().IsValid())
	{
		TagContainer.AddLeafTag(GetStance());
	}
	if (GetGait().IsValid())
	{
		TagContainer.AddLeafTag(GetGait());
	}
	if (ViewMode.IsValid())
	{
		TagContainer.AddLeafTag(ViewMode);
	}
	if (OverlayMode.IsValid())
	{
		TagContainer.AddLeafTag(OverlayMode);
	}
}

bool AGarCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	GetOwnedGameplayTags(TempTagContainer);
	return TempTagContainer.HasTag(TagToCheck);
}

bool AGarCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	GetOwnedGameplayTags(TempTagContainer);
	return TempTagContainer.HasAll(TagContainer);
}

bool AGarCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	GetOwnedGameplayTags(TempTagContainer);
	return TempTagContainer.HasAny(TagContainer);
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
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedViewRotation, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, InputDirection, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredVelocityYawAngle, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, bIsLied, Parameters)
}

void AGarCharacter::PreRegisterAllComponents()
{
	// Set some default values here so that the animation instance and the
	// camera component can read the most up-to-date values during initialization.

	if (IsValid(Settings))
	{
		SetRotationMode(DesiredToActual(DesiredRotationMode));
		SetStance(DesiredToActual(DesiredStance));
		SetGait(DesiredToActual(DesiredGait));
	}

	Super::PreRegisterAllComponents();
}

void AGarCharacter::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	SetReplicatedViewRotation(Super::GetViewRotation().GetNormalized(), false);

	ViewState.NetworkSmoothing.InitialRotation = ReplicatedViewRotation;
	ViewState.NetworkSmoothing.TargetRotation = ReplicatedViewRotation;
	ViewState.NetworkSmoothing.CurrentRotation = ReplicatedViewRotation;

	ViewState.Rotation = ViewState.LookRotation = ReplicatedViewRotation;
	ViewState.PreviousYawAngle = UE_REAL_TO_FLOAT(ReplicatedViewRotation.Yaw);

	const auto& ActorTransform{GetActorTransform()};

	LocomotionState.Rotation = GetActorRotation();

	RefreshTargetYawAngleUsingLocomotionRotation();

	LocomotionState.InputYawAngle = UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw);
	LocomotionState.VelocityYawAngle = UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw);
}

void AGarCharacter::PostInitializeComponents()
{
	// Make sure the mesh and animation blueprint are ticking after the character so they can access the most up-to-date character state.

	GetMesh()->AddTickPrerequisiteActor(this);

	AnimationInstance = Cast<UGarAnimationInstance>(GetMesh()->GetAnimInstance());

	// workaround for crash since 5.6
	//PhysicalAnimation->SetSkeletalMeshComponent(GetMesh());

	if (IsValid(AbilitySystem))
	{
		AbilitySystem->InitAbilityActorInfo(this, this);

		if (HasAuthority() && IsValid(AbilitySet))
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystem, this);
		}
	}

	Super::PostInitializeComponents();
}

void AGarCharacter::BeginPlay()
{
	if(!ensure(IsValid(Settings))) return;
	if(!ensure(IsValid(PhysicalAnimation))) return;
	if(!ensure(IsValid(MotionWarping))) return;
	if(!ensure(GarCharacterMovement.IsValid())) return;
	if(!ensure(AnimationInstance.IsValid())) return;

	if(!ensureMsgf(!bUseControllerRotationPitch && !bUseControllerRotationYaw && !bUseControllerRotationRoll,
					   TEXT("These settings are not allowed and must be turned off!"))) return;

	Super::BeginPlay();

	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		// Teleportation of simulated proxies is detected differently, see
		// AGarCharacter::PostNetReceiveLocationAndRotation() and AGarCharacter::OnRep_ReplicatedBasedMovement().

		GetCapsuleComponent()->TransformUpdated.AddWeakLambda(
			this, [this](USceneComponent*, const EUpdateTransformFlags, const ETeleportType TeleportType)
			{
				if (TeleportType != ETeleportType::None && AnimationInstance.IsValid())
				{
					AnimationInstance->MarkTeleported();
				}
			});
	}

	RefreshMeshProperties();

	// workaround for crash since 5.6
	PhysicalAnimation->SetSkeletalMeshComponent(GetMesh());

	ViewState.NetworkSmoothing.bEnabled |= IsValid(Settings) && Settings->View.bEnableNetworkSmoothing && GetLocalRole() == ROLE_SimulatedProxy;

	// Update states to use the initial desired values.

	RefreshRotationMode();
	ApplyDesiredStance();
	RefreshGait();
}

void AGarCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	RefreshMeshProperties();

	// Enable view network smoothing on the listen server here because the remote role may not be valid yet during begin play.

	ViewState.NetworkSmoothing.bEnabled |= IsValid(Settings) && Settings->View.bEnableListenServerNetworkSmoothing &&
		IsNetMode(NM_ListenServer) && GetRemoteRole() == ROLE_AutonomousProxy;

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

void AGarCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	OnSetupPlayerInputComponent.Broadcast(Input);
}

void AGarCharacter::PostNetReceiveLocationAndRotation()
{
	// AActor::PostNetReceiveLocationAndRotation() function is only called on simulated proxies, so there is no need to check roles here.

	const auto PreviousLocation{GetActorLocation()};

	// Ignore server-replicated rotation on simulated proxies because GAR itself has full control over character rotation.

	GetReplicatedMovement_Mutable().Rotation = GetActorRotation();

	Super::PostNetReceiveLocationAndRotation();

	// Detect teleportation of simulated proxies.

	auto bTeleported{static_cast<bool>(bSimGravityDisabled)};

	if (!bTeleported && !ReplicatedBasedMovement.HasRelativeLocation())
	{
		const auto NewLocation{FRepMovement::RebaseOntoLocalOrigin(GetReplicatedMovement().Location, this)};

		bTeleported |= FVector::DistSquared(PreviousLocation, NewLocation) > GarCharacterConstants::TeleportDistanceThresholdSquared;
	}

	if (bTeleported && AnimationInstance.IsValid())
	{
		AnimationInstance->MarkTeleported();
	}
}

void AGarCharacter::OnRep_ReplicatedBasedMovement()
{
	// ACharacter::OnRep_ReplicatedBasedMovement() is only called on simulated proxies, so there is no need to check roles here.

	const auto PreviousLocation{GetActorLocation()};

	// Ignore server-replicated rotation on simulated proxies because GAR itself has full control over character rotation.

	if (ReplicatedBasedMovement.HasRelativeRotation())
	{
		FVector MovementBaseLocation;
		FQuat MovementBaseRotation;

		MovementBaseUtility::GetMovementBaseTransform(ReplicatedBasedMovement.MovementBase, ReplicatedBasedMovement.BoneName,
													  MovementBaseLocation, MovementBaseRotation);

		ReplicatedBasedMovement.Rotation = (MovementBaseRotation.Inverse() * GetActorQuat()).Rotator();
	}
	else
	{
		ReplicatedBasedMovement.Rotation = GetActorRotation();
	}

	Super::OnRep_ReplicatedBasedMovement();

	// Detect teleportation of simulated proxies.

	auto bTeleported{static_cast<bool>(bSimGravityDisabled)};

	if (!bTeleported && BasedMovement.HasRelativeLocation())
	{
		const auto NewLocation{
			GetCharacterMovement()->OldBaseLocation + GetCharacterMovement()->OldBaseQuat.RotateVector(BasedMovement.Location)
		};

		bTeleported |= FVector::DistSquared(PreviousLocation, NewLocation) > GarCharacterConstants::TeleportDistanceThresholdSquared;
	}

	if (bTeleported && AnimationInstance.IsValid())
	{
		AnimationInstance->MarkTeleported();
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

	RefreshCapsuleSize(DeltaTime);

	RefreshMovementBase();

	RefreshMeshProperties();

	RefreshInput(DeltaTime);

	RefreshLocomotionEarly();

	RefreshView(DeltaTime);
	RefreshRotationMode();
	RefreshLocomotion(DeltaTime);
	RefreshGait();

	RefreshGroundedRotation(DeltaTime);
	RefreshInAirRotation(DeltaTime);

	OnRefresh.Broadcast(DeltaTime);

	Super::Tick(DeltaTime);

	RefreshLocomotionLate(DeltaTime);
}

void AGarCharacter::Restart()
{
	Super::Restart();

	ApplyDesiredStance();
}

void AGarCharacter::RefreshMeshProperties() const
{
	bool bStandalone{IsNetMode(NM_Standalone)};
	bool bDedicatedServer{IsNetMode(NM_DedicatedServer)};
	bool bListenServer{IsNetMode(NM_ListenServer)};

	bool bAuthority{GetLocalRole() >= ROLE_Authority};
	bool bRemoteAutonomousProxy{GetRemoteRole() == ROLE_AutonomousProxy};
	bool bLocallyControlled{IsLocallyControlled()};

	// Make sure that the pose is always ticked on the server when the character is controlled
	// by a remote client, otherwise some problems may arise (such as jitter when rolling).

	const auto DefaultTickOption{GetClass()->GetDefaultObject<ThisClass>()->GetMesh()->VisibilityBasedAnimTickOption};

	const auto TargetTickOption{
		!bStandalone && bAuthority && bRemoteAutonomousProxy
			? EVisibilityBasedAnimTickOption::AlwaysTickPose
			: EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered
	};

	// Keep the default tick option, at least if the target tick option is not required by the plugin to work properly.

	GetMesh()->VisibilityBasedAnimTickOption = FMath::Min(TargetTickOption, DefaultTickOption);

	bool bMeshIsTicking{GetMesh()->bRecentlyRendered || GetMesh()->VisibilityBasedAnimTickOption <= EVisibilityBasedAnimTickOption::AlwaysTickPose};

	// Use absolute mesh rotation to be able to precisely synchronize character rotation
	// with animations by manually updating the mesh rotation from the animation instance.

	// This is necessary in cases where the character and the animation instance are ticking
	// at different frequencies, which leads to desynchronization of rotation animations
	// with the character rotation, as well as foot sliding when the foot lock is active.

	// To save performance, use this only when really necessary, such as
	// when URO is enabled, or for autonomous proxies on the listen server.

	bool bUROActive{GetMesh()->AnimUpdateRateParams != nullptr && GetMesh()->AnimUpdateRateParams->UpdateRate > 1};
	bool bAutonomousProxyOnListenServer{bListenServer && bRemoteAutonomousProxy};

	// Can't use absolute mesh rotation when the character is standing on a rotating object, as it
	// causes constant rotation jitter. Be careful: although it eliminates jitter in this case, not
	// using absolute mesh rotation can cause jitter when rotating in place or turning in place.

	const auto bStandingOnRotatingObject{MovementBase.bHasRelativeRotation};

	bool bUseAbsoluteRotation{
		bMeshIsTicking && !bDedicatedServer && !bLocallyControlled && !bStandingOnRotatingObject &&
		(bUROActive || bAutonomousProxyOnListenServer)
	};

	if (GetMesh()->IsUsingAbsoluteRotation() != bUseAbsoluteRotation)
	{
		GetMesh()->SetUsingAbsoluteRotation(bUseAbsoluteRotation);

		// Instantly update the relative mesh rotation, otherwise it will be incorrect during this tick.

		if (bUseAbsoluteRotation || !IsValid(GetMesh()->GetAttachParent()))
		{
			GetMesh()->SetRelativeRotation_Direct(
				GetMesh()->GetRelativeRotationCache().QuatToRotator(GetMesh()->GetComponentQuat()));
		}
		else
		{
			GetMesh()->SetRelativeRotation_Direct(
				GetMesh()->GetRelativeRotationCache().QuatToRotator(GetActorQuat().Inverse() * GetMesh()->GetComponentQuat()));
		}
	}

	if (!bMeshIsTicking)
	{
		AnimationInstance->MarkPendingUpdate();
	}
}

void AGarCharacter::RefreshMovementBase()
{
	if (BasedMovement.MovementBase != MovementBase.Primitive || BasedMovement.BoneName != MovementBase.BoneName)
	{
		MovementBase.Primitive = BasedMovement.MovementBase;
		MovementBase.BoneName = BasedMovement.BoneName;
		MovementBase.bBaseChanged = true;
	}
	else
	{
		MovementBase.bBaseChanged = false;
	}

	MovementBase.bHasRelativeLocation = BasedMovement.HasRelativeLocation();
	MovementBase.bHasRelativeRotation = MovementBase.bHasRelativeLocation & BasedMovement.bRelativeRotation;

	const auto PreviousRotation{MovementBase.Rotation};

	MovementBaseUtility::GetMovementBaseTransform(BasedMovement.MovementBase, BasedMovement.BoneName,
												  MovementBase.Location, MovementBase.Rotation);

	MovementBase.DeltaRotation = MovementBase.bHasRelativeLocation && !MovementBase.bBaseChanged
								 ? (MovementBase.Rotation * PreviousRotation.Inverse()).Rotator()
								 : FRotator::ZeroRotator;
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
		AbilitySystem->GetOwnedGameplayTags(TempTagContainer);
	}
	return TempTagContainer.Filter(Settings->ActionTags).First();
}

void AGarCharacter::SetViewMode(const FGameplayTag& NewViewMode)
{
	if (ViewMode == NewViewMode)
	{
		return;
	}

	const auto PreviousViewMode{ViewMode};

	ViewMode = NewViewMode;

	OnViewModeChanged(PreviousViewMode);
}

void AGarCharacter::OnViewModeChanged_Implementation(const FGameplayTag& PreviousRotationMode) {}

void AGarCharacter::OnMovementModeChanged(const EMovementMode PreviousMovementMode, const uint8 PreviousCustomMode)
{
	// Use the character movement mode to set the locomotion mode to the right value. This allows you to have a
	// custom set of movement modes but still use the functionality of the default character movement component.

	switch (GetCharacterMovement()->MovementMode)
	{
		case MOVE_Walking:
		case MOVE_NavWalking:
			SetLocomotionMode(GarLocomotionModeTags::Grounded);
			break;

		case MOVE_Falling:
		case MOVE_Flying:
			SetLocomotionMode(GarLocomotionModeTags::InAir);
			break;
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void AGarCharacter::SetLocomotionMode(const FGameplayTag& NewLocomotionMode)
{	
	if (LocomotionMode == NewLocomotionMode)
	{
		return;
	}

	const auto PreviousLocomotionMode{LocomotionMode};

	LocomotionMode = NewLocomotionMode;

	NotifyLocomotionModeChanged(PreviousLocomotionMode);
}

void AGarCharacter::NotifyLocomotionModeChanged(const FGameplayTag& PreviousLocomotionMode)
{
	ApplyDesiredStance();

	if (LocomotionMode == GarLocomotionModeTags::Grounded && PreviousLocomotionMode == GarLocomotionModeTags::InAir && IsValid(AbilitySystem))
	{
		if (!AbilitySystem->TryActivateAbilitiesBySingleTag(GarLocomotionActionTags::Landing))
		{
			static constexpr auto HasInputBrakingFrictionFactor{0.5f};
			static constexpr auto NoInputBrakingFrictionFactor{3.0f};

			GetCharacterMovement()->BrakingFrictionFactor = LocomotionState.bHasInput
															? HasInputBrakingFrictionFactor
															: NoInputBrakingFrictionFactor;

			static constexpr auto ResetDelay{0.5f};

			GetWorldTimerManager().SetTimer(BrakingFrictionFactorResetTimer,
											FTimerDelegate::CreateWeakLambda(this, [this]
											{
												GetCharacterMovement()->BrakingFrictionFactor = 0.0f;
											}), ResetDelay, false);

			// Block character rotation towards the last input direction after landing to
			// prevent legs from twisting into a spiral while the landing animation is playing.

			LocomotionState.bRotationTowardsLastInputDirectionBlocked = true;
		}
	}

	OnLocomotionModeChanged(PreviousLocomotionMode);
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
}

void AGarCharacter::SetRotationMode(const FGameplayTag& NewRotationMode)
{
	const FGameplayTag PreviousRotationMode{GetRotationMode()};
	if (PreviousRotationMode == NewRotationMode)
	{
		return;
	}
	GarCharacterMovement->SetRotationMode(NewRotationMode);
	OnRotationModeChanged(PreviousRotationMode);
}

void AGarCharacter::OnRotationModeChanged_Implementation(const FGameplayTag& PreviousRotationMode) {}

const FGameplayTag& AGarCharacter::GetRotationMode() const
{
	return GarCharacterMovement->GetRotationMode();
}

void AGarCharacter::RefreshRotationMode()
{
	bool bSprinting{GetGait() == GarGaitTags::Sprinting};
	bool bAiming{HasMatchingGameplayTag(GarAimingModeTags::Root)};

	if (ViewMode == GarViewModeTags::FirstPerson)
	{
		if (LocomotionMode == GarLocomotionModeTags::InAir)
		{
			if (bAiming && Settings->bAllowAimingWhenInAir)
			{
				SetRotationMode(GarRotationModeTags::Aiming);
			}
			else
			{
				SetRotationMode(GarRotationModeTags::ViewDirection);
			}

			return;
		}

		// Grounded and other locomotion modes.

		if (bAiming && (!bSprinting || !Settings->bSprintHasPriorityOverAiming))
		{
			SetRotationMode(GarRotationModeTags::Aiming);
		}
		else
		{
			SetRotationMode(GarRotationModeTags::ViewDirection);
		}

		return;
	}

	// Third person and other view modes.

	if (LocomotionMode == GarLocomotionModeTags::InAir)
	{
		if (bAiming && Settings->bAllowAimingWhenInAir)
		{
			SetRotationMode(GarRotationModeTags::Aiming);
		}
		else if (bAiming)
		{
			SetRotationMode(GarRotationModeTags::ViewDirection);
		}
		else
		{
			SetRotationMode(DesiredToActual(DesiredRotationMode));
		}

		return;
	}

	// Grounded and other locomotion modes.

	if (bSprinting)
	{
		if (bAiming && !Settings->bSprintHasPriorityOverAiming)
		{
			SetRotationMode(GarRotationModeTags::Aiming);
		}
		else if (Settings->bRotateToVelocityWhenSprinting)
		{
			SetRotationMode(GarRotationModeTags::VelocityDirection);
		}
		else if (bAiming)
		{
			SetRotationMode(GarRotationModeTags::ViewDirection);
		}
		else
		{
			SetRotationMode(DesiredToActual(DesiredRotationMode));
		}
	}
	else // Not sprinting.
	{
		if (bAiming)
		{
			SetRotationMode(GarRotationModeTags::Aiming);
		}
		else
		{
			SetRotationMode(DesiredToActual(DesiredRotationMode));
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

	ApplyDesiredStance();
}

void AGarCharacter::ApplyDesiredStance()
{
	if (!GetLocomotionAction().IsValid())
	{
		if (LocomotionMode == GarLocomotionModeTags::Grounded)
		{
			if (DesiredStance == GarDesiredStanceTags::Standing)
			{
				UnCrouch();
			}
			else if (DesiredStance == GarDesiredStanceTags::Crouching)
			{
				Crouch();
			}
			else if (DesiredStance == GarDesiredStanceTags::Lying)
			{
				Lie();
			}
		}
		else if (LocomotionMode == GarLocomotionModeTags::InAir)
		{
			UnCrouch();
		}
	}
}

bool AGarCharacter::CanCrouch() const
{
	// This allows the ACharacter::Crouch() function to execute properly when bIsCrouched is true.
	// TODO Wait for https://github.com/EpicGames/UnrealEngine/pull/9558 to be merged into the engine.

	return bIsCrouched || Super::CanCrouch();
}

void AGarCharacter::Crouch(bool bClientSimulation)
{
	Super::Crouch(bClientSimulation);
	if (GarCharacterMovement.IsValid())
	{
		GarCharacterMovement->bWantsToLie = false;
	}
}

void AGarCharacter::UnCrouch(bool bClientSimulation)
{
	Super::UnCrouch(bClientSimulation);
	if (GarCharacterMovement.IsValid())
	{
		GarCharacterMovement->bWantsToLie = false;
	}
}

void AGarCharacter::OnStartCrouch(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust)
{
	auto* PredictionData{GetCharacterMovement()->GetPredictionData_Client_Character()};

	if (PredictionData != nullptr && GetLocalRole() <= ROLE_SimulatedProxy &&
	    ScaledHalfHeightAdjust > 0.0f && IsPlayingNetworkedRootMotionMontage())
	{
		// The code below essentially undoes the changes that will be made later at the end of the UCharacterMovementComponent::Crouch()
		// function because they literally break network smoothing when crouching while the root motion montage is playing, causing the
		// mesh to take an incorrect location for a while.

		// TODO Check the need for this fix in future engine versions.

		PredictionData->MeshTranslationOffset.Z += ScaledHalfHeightAdjust;
		PredictionData->OriginalMeshTranslationOffset = PredictionData->MeshTranslationOffset;
	}

	K2_OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	if (!bIsLied)
	{
		SetStance(GarStanceTags::Crouching);
	}
}

void AGarCharacter::OnEndCrouch(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust)
{
	auto* PredictionData{GetCharacterMovement()->GetPredictionData_Client_Character()};

	if (PredictionData != nullptr && GetLocalRole() <= ROLE_SimulatedProxy &&
		ScaledHalfHeightAdjust > 0.0f && IsPlayingNetworkedRootMotionMontage())
	{
		// Same fix as in AGarCharacter::OnStartCrouch().

		PredictionData->MeshTranslationOffset.Z -= ScaledHalfHeightAdjust;
		PredictionData->OriginalMeshTranslationOffset = PredictionData->MeshTranslationOffset;
	}

	K2_OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SetStance(GarStanceTags::Standing);
}

void AGarCharacter::SetStance(const FGameplayTag& NewStance)
{
	const FGameplayTag PreviousStance{GetStance()};

	if (PreviousStance == NewStance)
	{
		return;
	}

	GarCharacterMovement->SetStance(NewStance);
	OnStanceChanged(PreviousStance);
}

void AGarCharacter::OnStanceChanged_Implementation(const FGameplayTag& PreviousStance) {}

const FGameplayTag& AGarCharacter::GetStance() const
{
	return GarCharacterMovement->GetStance();
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

void AGarCharacter::SetGait(const FGameplayTag& NewGait)
{
	const FGameplayTag PreviousGait{GetGait()};
	auto ActualNewGait{LimitGaitIfNeeded(NewGait)};
	if (PreviousGait == ActualNewGait)
	{
		return;
	}
	GarCharacterMovement->SetGait(ActualNewGait);
	OnGaitChanged(PreviousGait);
}

void AGarCharacter::OnGaitChanged_Implementation(const FGameplayTag& PreviousGait) {}

const FGameplayTag& AGarCharacter::GetGait() const
{
	return GarCharacterMovement->GetGait();
}

void AGarCharacter::RefreshGait()
{
	if (LocomotionMode != GarLocomotionModeTags::Grounded)
	{
		return;
	}

	SetGait(DesiredToActual(DesiredGait));
}

FGameplayTag AGarCharacter::LimitGaitIfNeeded_Implementation(const FGameplayTag& NewGait) const
{
	// Calculate the max allowed gait. This represents the maximum gait the character is currently allowed
	// to be in and can be determined by the desired gait, the rotation mode, the stance, etc. For example,
	// if you wanted to force the character into a walking state while indoors, this could be done here.

	if (NewGait == GarGaitTags::Sprinting && !CanSprint())
	{
		return GarGaitTags::Running;
	}

	return NewGait;
}

bool AGarCharacter::CanSprint() const
{
	// Determine if the character can sprint based on the rotation mode and input direction.
	// If the character is in view direction rotation mode, only allow sprinting if there is
	// input and if the input direction is aligned with the view direction within 50 degrees.

	if (!LocomotionState.bHasInput || GetStance() != GarStanceTags::Standing
		|| (GetRotationMode() == GarRotationModeTags::Aiming && !Settings->bSprintHasPriorityOverAiming))
	{
		return false;
	}

	if (ViewMode != GarViewModeTags::FirstPerson && (DesiredRotationMode == GarDesiredRotationModeTags::VelocityDirection || Settings->bRotateToVelocityWhenSprinting))
	{
		return true;
	}

	auto ViewRelativeAngle = FMath::Abs(FMath::UnwindDegrees(UE_REAL_TO_FLOAT(LocomotionState.InputYawAngle - ViewState.Rotation.Yaw)));
	if (ViewRelativeAngle < Settings->ViewRelativeAngleThresholdForSprint)
	{
		return true;
	}

	return false;
}

FRotator AGarCharacter::GetViewRotation() const
{
	return ViewState.Rotation;
}

void AGarCharacter::SetInputDirection(FVector NewInputDirection)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, InputDirection, NewInputDirection.GetSafeNormal(), this);
}

void AGarCharacter::RefreshInput(const float DeltaTime)
{
	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		SetInputDirection(GetCharacterMovement()->GetCurrentAcceleration() / GetCharacterMovement()->GetMaxAcceleration());
	}

	LocomotionState.bHasInput = InputDirection.SizeSquared() > UE_KINDA_SMALL_NUMBER;

	if (LocomotionState.bHasInput)
	{
		LocomotionState.InputYawAngle = UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(InputDirection));
	}
}

void AGarCharacter::SetReplicatedViewRotation(const FRotator& NewViewRotation, const bool bSendRpc)
{
	if (!ReplicatedViewRotation.Equals(NewViewRotation))
	{
		ReplicatedViewRotation = NewViewRotation;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedViewRotation, this)

		if (bSendRpc && GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerSetReplicatedViewRotation(ReplicatedViewRotation);
		}
	}
}

void AGarCharacter::ServerSetReplicatedViewRotation_Implementation(const FRotator& NewViewRotation)
{
	SetReplicatedViewRotation(NewViewRotation, false);
}

void AGarCharacter::OnReplicated_ReplicatedViewRotation()
{
	CorrectViewNetworkSmoothing(ReplicatedViewRotation, MovementBase.bHasRelativeRotation);
}

void AGarCharacter::CorrectViewNetworkSmoothing(const FRotator& NewTargetRotation, const bool bRelativeTargetRotation)
{
	// Based on UCharacterMovementComponent::SmoothCorrection().

	auto& NetworkSmoothing{ViewState.NetworkSmoothing};

	NetworkSmoothing.TargetRotation = bRelativeTargetRotation
									  ? (MovementBase.Rotation * NewTargetRotation.Quaternion()).Rotator()
									  : NewTargetRotation.GetNormalized();

	if (!NetworkSmoothing.bEnabled)
	{
		NetworkSmoothing.InitialRotation = NetworkSmoothing.TargetRotation;
		NetworkSmoothing.CurrentRotation = NetworkSmoothing.TargetRotation;
		return;
	}

	bool bListenServer{IsNetMode(NM_ListenServer)};

	const auto NewNetworkSmoothingServerTime{
		bListenServer
			? GetCharacterMovement()->GetServerLastTransformUpdateTimeStamp()
			: GetReplicatedServerLastTransformUpdateTimeStamp()
	};

	if (NewNetworkSmoothingServerTime <= 0.0f)
	{
		return;
	}

	NetworkSmoothing.InitialRotation = NetworkSmoothing.CurrentRotation;

	// Using server time lets us know how much time elapsed, regardless of packet lag variance.

	const auto ServerDeltaTime{NewNetworkSmoothingServerTime - NetworkSmoothing.ServerTime};

	NetworkSmoothing.ServerTime = NewNetworkSmoothingServerTime;

	// Don't let the client fall too far behind or run ahead of new server time.

	const auto MaxServerDeltaTime{GetDefault<AGameNetworkManager>()->MaxClientSmoothingDeltaTime};

	const auto MinServerDeltaTime{
		FMath::Min(MaxServerDeltaTime, bListenServer
									   ? GetCharacterMovement()->ListenServerNetworkSimulatedSmoothLocationTime
									   : GetCharacterMovement()->NetworkSimulatedSmoothLocationTime)
	};

	// Calculate how far behind we can be after receiving a new server time.

	const auto MinClientDeltaTime{FMath::Clamp(ServerDeltaTime * 1.25f, MinServerDeltaTime, MaxServerDeltaTime)};

	NetworkSmoothing.ClientTime = FMath::Clamp(NetworkSmoothing.ClientTime,
											   NetworkSmoothing.ServerTime - MinClientDeltaTime,
											   NetworkSmoothing.ServerTime);

	// Compute actual delta between new server time and client simulation.

	NetworkSmoothing.Duration = NetworkSmoothing.ServerTime - NetworkSmoothing.ClientTime;
}

void AGarCharacter::SetLookRotation(const FRotator& NewLookRotation)
{
	TargetLookRotation = NewLookRotation;
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
														Settings->View.AdjustControllRotationSpeed));
		PendingFocalRotationRelativeAdjustment -= Controller->GetControlRotation() - ControlRotation;
		PendingFocalRotationRelativeAdjustment.Normalize();
		UE_LOG(LogGar, Verbose, TEXT("Applay PendingFocalRotationRelativeAdjustment %s %s"),
			   *(PendingFocalRotationRelativeAdjustment - PreviousPendingFocalRotationRelativeAdjustment).ToString(),
			   *PendingFocalRotationRelativeAdjustment.ToString());
	}
}

void AGarCharacter::RefreshView(const float DeltaTime)
{
	if (MovementBase.bHasRelativeRotation)
	{
		// Offset the rotations to keep them relative to the movement base.

		ViewState.Rotation.Pitch += MovementBase.DeltaRotation.Pitch;
		ViewState.Rotation.Yaw += MovementBase.DeltaRotation.Yaw;
		ViewState.Rotation.Normalize();
	}

	ViewState.PreviousYawAngle = UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw);

	if (IsLocallyControlled())
	{
		if (MovementBase.bHasRelativeRotation)
		{
			// We can't depend on the view rotation sent by the character movement component
			// since it's in world space, so in this case we always send it ourselves.

			SetReplicatedViewRotation((MovementBase.Rotation.Inverse() * Super::GetViewRotation().Quaternion()).Rotator(), true);
		}
		else
		{
			SetReplicatedViewRotation(Super::GetViewRotation().GetNormalized(), true);
		}
	}

	RefreshViewNetworkSmoothing(DeltaTime);

	if (IsLocallyControlled())
	{
		if (!TargetLookRotation.ContainsNaN())
		{
			ViewState.LookRotation = FMath::RInterpTo(ViewState.LookRotation, TargetLookRotation, DeltaTime, Settings->View.LookRotationInterpSpeed);
		}
		else
		{
			ViewState.LookRotation = FMath::RInterpTo(ViewState.LookRotation, ViewState.Rotation, DeltaTime, Settings->View.LookRotationInterpSpeed);
		}
	}
	else
	{
		ViewState.LookRotation = ViewState.Rotation;
	}

	// Set the yaw speed by comparing the current and previous view yaw angle, divided by
	// delta seconds. This represents the speed the camera is rotating from left to right.

	if (DeltaTime > UE_SMALL_NUMBER)
	{
		ViewState.YawSpeed = FMath::Abs(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw - ViewState.PreviousYawAngle)) / DeltaTime;
	}
}

void AGarCharacter::RefreshViewNetworkSmoothing(const float DeltaTime)
{
	// Based on UCharacterMovementComponent::SmoothClientPosition_Interpolate()
	// and UCharacterMovementComponent::SmoothClientPosition_UpdateVisuals().

	auto& NetworkSmoothing{ViewState.NetworkSmoothing};

	if (!NetworkSmoothing.bEnabled ||
		NetworkSmoothing.ClientTime >= NetworkSmoothing.ServerTime ||
		NetworkSmoothing.Duration <= UE_SMALL_NUMBER ||
		(MovementBase.bHasRelativeRotation && IsNetMode(NM_ListenServer)))
	{
		// Can't use network smoothing on the listen server when the character
		// is standing on a rotating object, as it causes constant rotation jitter.

		ViewState.Rotation = MovementBase.bHasRelativeRotation
						   ? (MovementBase.Rotation * ReplicatedViewRotation.Quaternion()).Rotator()
						   : ReplicatedViewRotation;

		NetworkSmoothing.InitialRotation = NetworkSmoothing.TargetRotation = NetworkSmoothing.CurrentRotation = ViewState.Rotation;

		return;
	}

	if (MovementBase.bHasRelativeRotation)
	{
		// Offset the rotations to keep them relative to the movement base.

		NetworkSmoothing.InitialRotation.Pitch += MovementBase.DeltaRotation.Pitch;
		NetworkSmoothing.InitialRotation.Yaw += MovementBase.DeltaRotation.Yaw;
		NetworkSmoothing.InitialRotation.Normalize();

		NetworkSmoothing.TargetRotation.Pitch += MovementBase.DeltaRotation.Pitch;
		NetworkSmoothing.TargetRotation.Yaw += MovementBase.DeltaRotation.Yaw;
		NetworkSmoothing.TargetRotation.Normalize();

		NetworkSmoothing.CurrentRotation.Pitch += MovementBase.DeltaRotation.Pitch;
		NetworkSmoothing.CurrentRotation.Yaw += MovementBase.DeltaRotation.Yaw;
		NetworkSmoothing.CurrentRotation.Normalize();
	}

	NetworkSmoothing.ClientTime += DeltaTime;

	const auto InterpolationAmount{
		UGarMath::Clamp01(1.0f - (NetworkSmoothing.ServerTime - NetworkSmoothing.ClientTime) / NetworkSmoothing.Duration)
	};

	if (!FAnimWeight::IsFullWeight(InterpolationAmount))
	{
		NetworkSmoothing.CurrentRotation = UGarMath::LerpRotator(NetworkSmoothing.InitialRotation,
																 NetworkSmoothing.TargetRotation,
																 InterpolationAmount);
	}
	else
	{
		NetworkSmoothing.ClientTime = NetworkSmoothing.ServerTime;
		NetworkSmoothing.CurrentRotation = NetworkSmoothing.TargetRotation;
	}

	ViewState.Rotation = NetworkSmoothing.CurrentRotation;
}

void AGarCharacter::SetDesiredVelocityYawAngle(const float NewDesiredVelocityYawAngle)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, DesiredVelocityYawAngle, NewDesiredVelocityYawAngle, this);
}

void AGarCharacter::RefreshLocomotionLocationAndRotation()
{
	const auto& ActorTransform{GetActorTransform()};

	// If network smoothing is disabled, then return regular actor transform.

	if (GetCharacterMovement()->NetworkSmoothingMode == ENetworkSmoothingMode::Disabled)
	{
		LocomotionState.Location = ActorTransform.GetLocation();
		LocomotionState.RotationQuaternion = ActorTransform.GetRotation();
		LocomotionState.Rotation = GetActorRotation();
	}
	else if (GetMesh()->IsUsingAbsoluteRotation())
	{
		LocomotionState.Location = ActorTransform.TransformPosition(GetMesh()->GetRelativeLocation() - GetBaseTranslationOffset());
		LocomotionState.RotationQuaternion = ActorTransform.GetRotation();
		LocomotionState.Rotation = GetActorRotation();
	}
	else
	{
		const auto SmoothTransform{
			ActorTransform * FTransform{
				GetMesh()->GetRelativeRotationCache().RotatorToQuat_ReadOnly(
					GetMesh()->GetRelativeRotation()) * GetBaseRotationOffset().Inverse(),
				GetMesh()->GetRelativeLocation() - GetBaseTranslationOffset()
			}
		};

		LocomotionState.Location = SmoothTransform.GetLocation();
		LocomotionState.RotationQuaternion = SmoothTransform.GetRotation();
		LocomotionState.Rotation = LocomotionState.RotationQuaternion.Rotator();
	}
}

void AGarCharacter::RefreshLocomotionEarly()
{
	if (MovementBase.bHasRelativeRotation)
	{
		// Offset the rotations (the actor's rotation too) to keep them relative to the movement base.

		LocomotionState.TargetYawAngle = FMath::UnwindDegrees(UE_REAL_TO_FLOAT(
			LocomotionState.TargetYawAngle + MovementBase.DeltaRotation.Yaw));

		LocomotionState.ViewRelativeTargetYawAngle = FMath::UnwindDegrees(UE_REAL_TO_FLOAT(
			LocomotionState.ViewRelativeTargetYawAngle + MovementBase.DeltaRotation.Yaw));

		LocomotionState.SmoothTargetYawAngle = FMath::UnwindDegrees(UE_REAL_TO_FLOAT(
			LocomotionState.SmoothTargetYawAngle + MovementBase.DeltaRotation.Yaw));

		auto NewRotation{GetActorRotation()};
		NewRotation.Pitch += MovementBase.DeltaRotation.Pitch;
		NewRotation.Yaw += MovementBase.DeltaRotation.Yaw;
		NewRotation.Normalize();

		SetActorRotation(NewRotation);
	}

	RefreshLocomotionLocationAndRotation();

	LocomotionState.PreviousVelocity = LocomotionState.Velocity;
	LocomotionState.PreviousYawAngle = UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw);
}

void AGarCharacter::RefreshLocomotion(const float DeltaTime)
{
	LocomotionState.Velocity = GetVelocity();

	// Determine if the character is moving by getting its speed. The speed equals the length
	// of the horizontal velocity, so it does not take vertical movement into account. If the
	// character is moving, update the last velocity rotation. This value is saved because it might
	// be useful to know the last orientation of a movement even after the character has stopped.

	LocomotionState.Speed = UE_REAL_TO_FLOAT(LocomotionState.Velocity.Size2D());

	static constexpr auto HasSpeedThreshold{1.0f};

	LocomotionState.bHasSpeed = LocomotionState.Speed >= HasSpeedThreshold;

	if (LocomotionState.bHasSpeed)
	{
		LocomotionState.VelocityYawAngle = UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(LocomotionState.Velocity));
	}

	if (Settings->bRotateTowardsDesiredVelocityInVelocityDirectionRotationMode && GetLocalRole() >= ROLE_AutonomousProxy)
	{
		FVector DesiredVelocity;

		SetDesiredVelocityYawAngle(GarCharacterMovement->TryConsumePrePenetrationAdjustmentVelocity(DesiredVelocity) &&
								   DesiredVelocity.Size2D() >= HasSpeedThreshold
								   ? UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(DesiredVelocity))
								   : LocomotionState.VelocityYawAngle);
	}

	if (DeltaTime > UE_SMALL_NUMBER)
	{
		LocomotionState.Acceleration = (LocomotionState.Velocity - LocomotionState.PreviousVelocity) / DeltaTime;
	}

	// Character is moving if has speed and current acceleration, or if the speed is greater than the moving speed threshold.

	LocomotionState.bMoving = (LocomotionState.bHasInput && LocomotionState.bHasSpeed) ||
							  LocomotionState.Speed > Settings->MovingSpeedThreshold;

	if (Settings->bAutoTurnOffSprint
		&& (GetLocomotionAction().IsValid() || GetLocomotionMode() == GarLocomotionModeTags::Grounded)
		&& LocomotionState.Speed < GarCharacterMovement->GetGaitSettings().WalkSpeed && GetDesiredGait() == GarDesiredGaitTags::Sprinting)
	{
		SetDesiredGait(GarDesiredGaitTags::Running);
	}
}

void AGarCharacter::RefreshLocomotionLate(const float DeltaTime)
{
	if (!LocomotionMode.IsValid() || GetLocomotionAction().IsValid())
	{
		RefreshLocomotionLocationAndRotation();
		RefreshTargetYawAngleUsingLocomotionRotation();
	}

	if (DeltaTime > UE_SMALL_NUMBER)
	{
		LocomotionState.YawSpeed = FMath::UnwindDegrees(UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw - LocomotionState.PreviousYawAngle)) / DeltaTime;
	}
}

void AGarCharacter::Jump()
{
	if (GetStance() == GarStanceTags::Standing && !GetLocomotionAction().IsValid() && LocomotionMode == GarLocomotionModeTags::Grounded)
	{
		Super::Jump();
	}
}

void AGarCharacter::FaceRotation(const FRotator Rotation, const float DeltaTime)
{
	// Left empty intentionally. We are ignoring rotation changes from external
	// sources because GAR itself has full control over character rotation.
}

void AGarCharacter::RefreshGroundedRotation(const float DeltaTime)
{
	if (GetLocomotionAction().IsValid() || LocomotionMode != GarLocomotionModeTags::Grounded)
	{
		return;
	}

	if (HasAnyRootMotion())
	{
		RefreshTargetYawAngleUsingLocomotionRotation();
		return;
	}

	if (!LocomotionState.bMoving)
	{
		// Not moving.

		ApplyRotationYawSpeedAnimationCurve(DeltaTime);

		if (RefreshCustomGroundedNotMovingRotation(DeltaTime))
		{
			return;
		}

		if (GetRotationMode() == GarRotationModeTags::VelocityDirection)
		{
			// Rotate to the last target yaw angle when not moving (relative to the movement base or not).

			const auto TargetYawAngle{
				MovementBase.bHasRelativeLocation && !MovementBase.bHasRelativeRotation &&
				Settings->bInheritMovementBaseRotationInVelocityDirectionRotationMode
					? UE_REAL_TO_FLOAT(LocomotionState.TargetYawAngle + MovementBase.DeltaRotation.Yaw)
					: LocomotionState.TargetYawAngle
			};

			static constexpr auto RotationInterpolationSpeed{12.0f};
			static constexpr auto TargetYawAngleRotationSpeed{800.0f};

			RefreshRotationExtraSmooth(TargetYawAngle, DeltaTime, RotationInterpolationSpeed, TargetYawAngleRotationSpeed);
			return;
		}

		if (GetRotationMode() == GarRotationModeTags::Aiming || ViewMode == GarViewModeTags::FirstPerson)
		{
			RefreshGroundedAimingRotation(DeltaTime);
			return;
		}

		RefreshTargetYawAngleUsingLocomotionRotation();
		return;
	}

	// Moving.

	if (RefreshCustomGroundedMovingRotation(DeltaTime))
	{
		return;
	}

	if (GetRotationMode() == GarRotationModeTags::VelocityDirection &&
	    (LocomotionState.bHasInput || !LocomotionState.bRotationTowardsLastInputDirectionBlocked))
	{
		LocomotionState.bRotationTowardsLastInputDirectionBlocked = false;

		static constexpr auto TargetYawAngleRotationSpeed{800.0f};

		RefreshRotationExtraSmooth(
			Settings->bRotateTowardsDesiredVelocityInVelocityDirectionRotationMode
				? DesiredVelocityYawAngle
				: LocomotionState.VelocityYawAngle,
			DeltaTime, CalculateGroundedMovingRotationInterpolationSpeed(), TargetYawAngleRotationSpeed);
		return;
	}

	if (GetRotationMode() == GarRotationModeTags::ViewDirection)
	{
		const auto TargetYawAngle{
			GetGait() == GarGaitTags::Sprinting
				? LocomotionState.VelocityYawAngle
				: UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw +
					GetMesh()->GetAnimInstance()->GetCurveValue(UGarConstants::RotationYawOffsetCurveName()))
		};

		static constexpr auto TargetYawAngleRotationSpeed{500.0f};

		RefreshRotationExtraSmooth(TargetYawAngle, DeltaTime, CalculateGroundedMovingRotationInterpolationSpeed(),
								   TargetYawAngleRotationSpeed);
		return;
	}

	if (GetRotationMode() == GarRotationModeTags::Aiming)
	{
		RefreshGroundedAimingRotation(DeltaTime);
		return;
	}

	RefreshTargetYawAngleUsingLocomotionRotation();
}

bool AGarCharacter::RefreshCustomGroundedMovingRotation(const float DeltaTime)
{
	return false;
}

bool AGarCharacter::RefreshCustomGroundedNotMovingRotation(const float DeltaTime)
{
	return false;
}

void AGarCharacter::RefreshGroundedAimingRotation(const float DeltaTime)
{
	if (!LocomotionState.bHasInput && !LocomotionState.bMoving)
	{
		// Not moving.

		if (RefreshConstrainedAimingRotation(DeltaTime, true))
		{
			return;
		}

		RefreshTargetYawAngleUsingLocomotionRotation();
		return;
	}

	// Moving.

	if (RefreshConstrainedAimingRotation(DeltaTime))
	{
		return;
	}

	static constexpr auto RotationInterpolationSpeed{20.0f};
	static constexpr auto TargetYawAngleRotationSpeed{1000.0f};

	RefreshRotationExtraSmooth(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw), DeltaTime,
							   RotationInterpolationSpeed, TargetYawAngleRotationSpeed);
}

bool AGarCharacter::RefreshConstrainedAimingRotation(const float DeltaTime, const bool bApplySecondaryConstraint)
{
	// Limit the character's rotation when aiming to prevent situations where the lower body noticeably
	// fails to keep up with the rotation of the upper body when the camera is rotating very fast.

	static constexpr auto ViewYawSpeedThreshold{620.0f};

	bool bApplyPrimaryConstraint{ViewState.YawSpeed > ViewYawSpeedThreshold};

	if (!bApplyPrimaryConstraint && !bApplySecondaryConstraint)
	{
		return false;
	}

	auto ViewRelativeYawAngle{FMath::UnwindDegrees(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw - LocomotionState.Rotation.Yaw))};
	static constexpr auto ViewRelativeYawAngleThreshold{70.0f};

	if (FMath::Abs(ViewRelativeYawAngle) <= ViewRelativeYawAngleThreshold + UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}

	ViewRelativeYawAngle = UGarMath::RemapAngleForCounterClockwiseRotation(ViewRelativeYawAngle);

	const auto TargetYawAngle{
		UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw +
			(ViewRelativeYawAngle >= 0.0f ? -ViewRelativeYawAngleThreshold : ViewRelativeYawAngleThreshold))
	};

	// Primary constraint. Prevents the character from rotating past a certain angle when the camera rotation speed is very high.

	if (bApplyPrimaryConstraint)
	{
		RefreshRotationInstant(TargetYawAngle);
		return true;
	}

	// Secondary constraint. Simply increases the character's rotation speed. Typically only used when the character is standing still.

	if (bApplySecondaryConstraint)
	{
		static constexpr auto RotationInterpolationSpeed{20.0f};

		RefreshRotation(TargetYawAngle, DeltaTime, RotationInterpolationSpeed);
		return true;
	}

	return false;
}

float AGarCharacter::CalculateGroundedMovingRotationInterpolationSpeed() const
{
	// Calculate the rotation speed by using the rotation speed curve in the movement gait settings. Using
	// the curve in conjunction with the gait amount gives you a high level of control over the rotation
	// rates for each speed. Increase the speed if the camera is rotating quickly for more responsive rotation.

	const auto* InterpolationSpeedCurve{GarCharacterMovement->GetGaitSettings().RotationInterpolationSpeedCurve.Get()};

	static constexpr auto DefaultInterpolationSpeed{5.0f};

	const auto InterpolationSpeed{
		ensure(IsValid(InterpolationSpeedCurve))
			? InterpolationSpeedCurve->GetFloatValue(GarCharacterMovement->CalculateGaitAmount())
			: DefaultInterpolationSpeed
	};

	static constexpr auto MaxInterpolationSpeedMultiplier{3.0f};
	static constexpr auto ReferenceViewYawSpeed{300.0f};

	return InterpolationSpeed * UGarMath::LerpClamped(1.0f, MaxInterpolationSpeedMultiplier, ViewState.YawSpeed / ReferenceViewYawSpeed);
}

void AGarCharacter::ApplyRotationYawSpeedAnimationCurve(const float DeltaTime)
{
	const auto DeltaYawAngle{GetMesh()->GetAnimInstance()->GetCurveValue(UGarConstants::RotationYawSpeedCurveName()) * DeltaTime};
	if (FMath::Abs(DeltaYawAngle) > UE_SMALL_NUMBER)
	{
		auto NewRotation{GetActorRotation()};
		NewRotation.Yaw += DeltaYawAngle;

		SetActorRotation(NewRotation);

		RefreshLocomotionLocationAndRotation();
		RefreshTargetYawAngleUsingLocomotionRotation();
	}
}

void AGarCharacter::RefreshInAirRotation(const float DeltaTime)
{
	if (GetLocomotionAction().IsValid() || LocomotionMode != GarLocomotionModeTags::InAir)
	{
		return;
	}

	if (RefreshCustomInAirRotation(DeltaTime))
	{
		return;
	}

	static constexpr auto RotationInterpolationSpeed{5.0f};
	const FGameplayTag RotationMode{GetRotationMode()};
	if (RotationMode == GarRotationModeTags::VelocityDirection || RotationMode == GarRotationModeTags::ViewDirection)
	{
		switch (Settings->InAirRotationMode)
		{
			case EGarInAirRotationMode::RotateToVelocityOnJump:
				if (LocomotionState.bMoving)
				{
					RefreshRotation(LocomotionState.VelocityYawAngle, DeltaTime, RotationInterpolationSpeed);
				}
				else
				{
					RefreshTargetYawAngleUsingLocomotionRotation();
				}
				break;

			case EGarInAirRotationMode::KeepRelativeRotation:
				RefreshRotation(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw - LocomotionState.ViewRelativeTargetYawAngle),
								DeltaTime, RotationInterpolationSpeed);
				break;

			default:
				RefreshTargetYawAngleUsingLocomotionRotation();
				break;
		}
	}
	else if (RotationMode == GarRotationModeTags::Aiming)
	{
		RefreshInAirAimingRotation(DeltaTime);
	}
	else
	{
		RefreshTargetYawAngleUsingLocomotionRotation();
	}
}

bool AGarCharacter::RefreshCustomInAirRotation(const float DeltaTime)
{
	return false;
}

void AGarCharacter::RefreshInAirAimingRotation(const float DeltaTime)
{
	if (RefreshConstrainedAimingRotation(DeltaTime))
	{
		return;
	}

	static constexpr auto RotationInterpolationSpeed{15.0f};

	RefreshRotation(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw), DeltaTime, RotationInterpolationSpeed);
}

void AGarCharacter::RefreshRotation(const float TargetYawAngle, const float DeltaTime, const float RotationInterpolationSpeed)
{
	RefreshTargetYawAngle(TargetYawAngle);

	auto NewRotation{GetActorRotation()};
	NewRotation.Yaw = UGarMath::ExponentialDecayAngle(UE_REAL_TO_FLOAT(FMath::UnwindDegrees(NewRotation.Yaw)),
													  TargetYawAngle, DeltaTime, RotationInterpolationSpeed);

	SetActorRotation(NewRotation);

	RefreshLocomotionLocationAndRotation();
}

void AGarCharacter::RefreshRotationExtraSmooth(const float TargetYawAngle, const float DeltaTime,
											   const float RotationInterpolationSpeed, const float TargetYawAngleRotationSpeed)
{
	LocomotionState.TargetYawAngle = FMath::UnwindDegrees(TargetYawAngle);

	RefreshViewRelativeTargetYawAngle();

	// Interpolate target yaw angle for extra smooth rotation.

	LocomotionState.SmoothTargetYawAngle = UGarMath::InterpolateAngleConstant(LocomotionState.SmoothTargetYawAngle,
																			  LocomotionState.TargetYawAngle,
																			  DeltaTime, TargetYawAngleRotationSpeed);

	auto NewRotation{GetActorRotation()};
	NewRotation.Yaw = UGarMath::ExponentialDecayAngle(UE_REAL_TO_FLOAT(FMath::UnwindDegrees(NewRotation.Yaw)),
													  LocomotionState.SmoothTargetYawAngle, DeltaTime, RotationInterpolationSpeed);

	SetActorRotation(NewRotation);

	RefreshLocomotionLocationAndRotation();
}

void AGarCharacter::RefreshRotationInstant(const float TargetYawAngle, const ETeleportType Teleport)
{
	RefreshTargetYawAngle(TargetYawAngle);

	auto NewRotation{GetActorRotation()};
	NewRotation.Yaw = TargetYawAngle;

	SetActorRotation(NewRotation, Teleport);

	RefreshLocomotionLocationAndRotation();
}

void AGarCharacter::RefreshTargetYawAngleUsingLocomotionRotation()
{
	RefreshTargetYawAngle(UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw));
}

void AGarCharacter::RefreshTargetYawAngle(const float TargetYawAngle)
{
	LocomotionState.TargetYawAngle = FMath::UnwindDegrees(TargetYawAngle);

	RefreshViewRelativeTargetYawAngle();

	LocomotionState.SmoothTargetYawAngle = LocomotionState.TargetYawAngle;
}

void AGarCharacter::RefreshViewRelativeTargetYawAngle()
{
	LocomotionState.ViewRelativeTargetYawAngle = FMath::UnwindDegrees(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw - LocomotionState.TargetYawAngle));
}

float AGarCharacter::GetAimAmount() const
{
	return AnimationInstance.IsValid() ? AnimationInstance->GetCurveValueClamped01(UGarConstants::AllowAimingCurveName()) : 0.0f;
}

bool AGarCharacter::HasSight_Implementation() const
{
	return false;
}

void AGarCharacter::GetSightLocAndRot_Implementation(FVector& Loc, FRotator& Rot) const
{
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

bool AGarCharacter::CanLie() const
{
	return true;
}

void AGarCharacter::Lie()
{
	if (GarCharacterMovement.IsValid())
	{
		if (CanLie())
		{
			GarCharacterMovement->bWantsToLie = true;
		}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		else if (!GarCharacterMovement->CanEverCrouch())
		{
			UE_LOG(LogGar, Log, TEXT("%s is trying to lie, but lying is disabled on this character! (check CharacterMovement NavAgentSettings)"), *GetName());
		}
#endif
	}
}

void AGarCharacter::OnStartLie(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust)
{
	auto* PredictionData{GetCharacterMovement()->GetPredictionData_Client_Character()};

	if (PredictionData != nullptr && GetLocalRole() <= ROLE_SimulatedProxy &&
		ScaledHalfHeightAdjust > 0.0f && IsPlayingNetworkedRootMotionMontage())
	{
		// The code below essentially undoes the changes that will be made later at the end of the UCharacterMovementComponent::Crouch()
		// function because they literally break network smoothing when crouching while the root motion montage is playing, causing the
		// mesh to take an incorrect location for a while.

		// TODO Check the need for this fix in future engine versions.

		PredictionData->MeshTranslationOffset.Z += ScaledHalfHeightAdjust;
		PredictionData->OriginalMeshTranslationOffset = PredictionData->MeshTranslationOffset;
	}

	K2_OnStartLie(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SetStance(GarStanceTags::Lying);
}

void AGarCharacter::OnEndLie(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust)
{
	auto* PredictionData{GetCharacterMovement()->GetPredictionData_Client_Character()};

	if (PredictionData != nullptr && GetLocalRole() <= ROLE_SimulatedProxy &&
		ScaledHalfHeightAdjust > 0.0f && IsPlayingNetworkedRootMotionMontage())
	{
		// Same fix as in AGarCharacter::OnStartCrouch().

		PredictionData->MeshTranslationOffset.Z -= ScaledHalfHeightAdjust;
		PredictionData->OriginalMeshTranslationOffset = PredictionData->MeshTranslationOffset;
	}

	K2_OnEndLie(HalfHeightAdjust, ScaledHalfHeightAdjust);

	if (bIsCrouched)
	{
		SetStance(GarStanceTags::Crouching);
	}
	else
	{
		SetStance(GarStanceTags::Standing);
	}
}

void AGarCharacter::RefreshCapsuleSize(float DeltaTime)
{
	// Update capsule height and radius
	auto DefaultCharacter = GetDefault<AGarCharacter>(GetClass());
	auto InitialEyeHeight = DefaultCharacter->BaseEyeHeight;
	auto InitialHalfHeight = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	auto InitialRadius = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	auto CrouchedHalfHeight = GarCharacterMovement->GetCrouchedHalfHeight();
	auto EyeHeightSpeed = CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialEyeHeight - CrouchedEyeHeight) / CapsuleUpdateSpeed : .0f;
	auto HalfHeightSpeed = CapsuleUpdateSpeed > 0 ? FMath::Abs(InitialHalfHeight - CrouchedHalfHeight) / CapsuleUpdateSpeed : .0f;
	if (bIsLied)
	{
		UpdateCapsule(DeltaTime, CrouchedEyeHeight, EyeHeightSpeed, CrouchedHalfHeight, HalfHeightSpeed, InitialRadius, 0.0f);
	}
	else if (bIsCrouched)
	{
		UpdateCapsule(DeltaTime, CrouchedEyeHeight, EyeHeightSpeed, CrouchedHalfHeight, HalfHeightSpeed, InitialRadius, 0.0f);
	}
	else
	{
		UpdateCapsule(DeltaTime, InitialEyeHeight, EyeHeightSpeed, InitialHalfHeight, HalfHeightSpeed, InitialRadius, 0.0f);
	}
}

void AGarCharacter::UpdateCapsule(float DeltaTime, float EyeHeight, float EyeHeightSpeed, float HalfHeight, float HalfHeightSpeed, float Radius, float RadiusSpeed)
{
	BaseEyeHeight = FMath::FInterpConstantTo(BaseEyeHeight, EyeHeight, DeltaTime, EyeHeightSpeed);
	BaseTranslationOffset.Z = FMath::FInterpConstantTo(BaseTranslationOffset.Z, -HalfHeight, DeltaTime, HalfHeightSpeed);

	GarCharacterMovement->UpdateCapsuleSize(DeltaTime, HalfHeight, HalfHeightSpeed, Radius, RadiusSpeed);
}

void AGarCharacter::SetIsLied(bool bNewIsLied)
{
	if (bIsLied != bNewIsLied)
	{
		bIsLied = bNewIsLied;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bIsLied, this)
	}
}

void AGarCharacter::OnRep_IsLied()
{
	if (GarCharacterMovement.IsValid())
	{
		if (bIsLied)
		{
			GarCharacterMovement->bWantsToLie = true;
			GarCharacterMovement->Lie(true);
		}
		else
		{
			GarCharacterMovement->bWantsToLie = false;
			GarCharacterMovement->UnLie(true);
		}
		GarCharacterMovement->bNetworkUpdateReceived = true;
	}
}
