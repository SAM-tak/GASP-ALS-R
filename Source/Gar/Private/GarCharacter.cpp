#include "GarCharacter.h"

#include "MotionWarpingComponent.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/GameNetworkManager.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Settings/GarCharacterSettings.h"
#include "GarAnimationInstance.h"
#include "GarCharacterMovementComponent.h"
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

FName AGarCharacter::PhysicalAnimationComponentName(TEXT("PhysicalAnimComp"));
FName AGarCharacter::AbilitySystemComponentName(TEXT("AbilitySystemComp"));
FName AGarCharacter::MotionWarpingComponentName(TEXT("MotionWarpComp"));

AGarCharacter::AGarCharacter(const FObjectInitializer& ObjectInitializer) : Super{
	ObjectInitializer.SetDefaultSubobjectClass<UGarCharacterMovementComponent>(CharacterMovementComponentName)
}
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;

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

	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(MotionWarpingComponentName);

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
	if (Perspective.IsValid())
	{
		TagContainer.AddLeafTag(Perspective);
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

void AGarCharacter::PostInitializeComponents()
{
	// Make sure the mesh and animation blueprint are ticking after the character so they can access the most up-to-date character state.

	GetMesh()->AddTickPrerequisiteActor(this);

	AnimationInstance = Cast<UGarAnimationInstance>(GetMesh()->GetAnimInstance());

	// workaround for crash since 5.6
	//PhysicalAnimation->SetSkeletalMeshComponent(GetMesh());

	if (IsValid(AbilitySystem))
	{
		AbilitySystem->Initialize(this);
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

	//if (GetLocalRole() >= ROLE_AutonomousProxy)
	//{
	//	// Teleportation of simulated proxies is detected differently, see
	//	// AGarCharacter::PostNetReceiveLocationAndRotation() and AGarCharacter::OnRep_ReplicatedBasedMovement().

	//	GetCapsuleComponent()->TransformUpdated.AddWeakLambda(
	//		this, [this](USceneComponent*, const EUpdateTransformFlags, const ETeleportType TeleportType)
	//		{
	//			if (TeleportType != ETeleportType::None && AnimationInstance.IsValid())
	//			{
	//				AnimationInstance->MarkTeleported();
	//			}
	//		});
	//}

	// workaround for crash since 5.6
	PhysicalAnimation->SetSkeletalMeshComponent(GetMesh());

	// Update states to use the initial desired values.

	RefreshRotationMode();
	ApplyDesiredStance();
	RefreshSprintState();
	RefreshGait();
}

void AGarCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//RefreshMeshProperties();

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

void AGarCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	OnSetupPlayerInputComponent.Broadcast(Input);
}

void AGarCharacter::PostNetReceiveLocationAndRotation()
{
	// AActor::PostNetReceiveLocationAndRotation() function is only called on simulated proxies, so there is no need to check roles here.

	const auto PreviousLocation{GetActorLocation()};

	Super::PostNetReceiveLocationAndRotation();

	// Detect teleportation of simulated proxies.

	//auto bTeleported{static_cast<bool>(bSimGravityDisabled)};

	//if (!bTeleported && !ReplicatedBasedMovement.HasRelativeLocation())
	//{
	//	const auto NewLocation{FRepMovement::RebaseOntoLocalOrigin(GetReplicatedMovement().Location, this)};

	//	bTeleported |= FVector::DistSquared(PreviousLocation, NewLocation) > GarCharacterConstants::TeleportDistanceThresholdSquared;
	//}

	//if (bTeleported && AnimationInstance.IsValid())
	//{
	//	AnimationInstance->MarkTeleported();
	//}
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

	//if (bTeleported && AnimationInstance.IsValid())
	//{
	//	AnimationInstance->MarkTeleported();
	//}
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

	RefreshInput(DeltaTime);

	RefreshRotationMode();
	RefreshSprintState();
	RefreshGait();

	OnRefresh.Broadcast(DeltaTime);

	Super::Tick(DeltaTime);
}

void AGarCharacter::Restart()
{
	Super::Restart();

	ApplyDesiredStance();
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

			GetCharacterMovement()->BrakingFrictionFactor = HasInput()
															? HasInputBrakingFrictionFactor
															: NoInputBrakingFrictionFactor;

			static constexpr auto ResetDelay{0.5f};

			GetWorldTimerManager().SetTimer(BrakingFrictionFactorResetTimer,
											FTimerDelegate::CreateWeakLambda(this, [this]
											{
												GetCharacterMovement()->BrakingFrictionFactor = 0.0f;
											}), ResetDelay, false);
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

	if (Perspective == GarPerspectiveTags::FirstPerson)
	{
		if (LocomotionMode == GarLocomotionModeTags::InAir)
		{
			if (bAiming && Settings->bAllowAimingWhenInAir)
			{
				SetRotationMode(GarRotationModeTags::Aiming);
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

void AGarCharacter::SetInputDirection(FVector NewInputDirection)
{
	InputDirection = NewInputDirection.GetSafeNormal();
}

void AGarCharacter::RefreshInput(const float DeltaTime)
{
	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		SetInputDirection(GetCharacterMovement()->GetCurrentAcceleration() / GetCharacterMovement()->GetMaxAcceleration());
	}

	if (HasInput())
	{
		InputYawAngle = UE_REAL_TO_FLOAT(UGarMath::DirectionToAngleXY(InputDirection));
	}
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
	return (HasInput() && Speed >= 1.0) || Speed > Settings->MovingSpeedThreshold;
}

void AGarCharacter::RefreshSprintState()
{
	if (Settings->bAutoTurnOffSprint
		&& (GetLocomotionAction().IsValid() || GetLocomotionMode() == GarLocomotionModeTags::Grounded)
		&& GetVelocity().Size2D() < GarCharacterMovement->GetGaitSettings().WalkSpeed && GetDesiredGait() == GarDesiredGaitTags::Sprinting)
	{
		SetDesiredGait(GarDesiredGaitTags::Running);
	}
}

void AGarCharacter::Jump()
{
	if (GetStance() == GarStanceTags::Standing && !GetLocomotionAction().IsValid() && LocomotionMode == GarLocomotionModeTags::Grounded)
	{
		Super::Jump();
	}
}

float AGarCharacter::GetAimAmount() const
{
	return AnimationInstance.IsValid() ? AnimationInstance->GetCurveValueClamped01(UGarConstants::AllowAimingCurveName()) : 0.0f;
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
	if (HasMatchingGameplayTag(GarStateFlagTags::BlockUpdateCapsuleSize))
	{
		return;
	}

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
