#pragma once

#include "GameFramework/Character.h"
#include "State/GarLocomotionState.h"
#include "State/GarMovementBaseState.h"
#include "State/GarViewState.h"
#include "GarGameplayTags.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Abilities/GarAbilitySet.h"
#include "GarCharacter.generated.h"

class UGarCharacterSettings;
class UGarAnimationInstance;
class UGarCharacterMovementComponent;
class UGarPhysicalAnimationComponent;
class UGarAbilitySystemComponent;
class UGarMotionWarpingComponent;

DECLARE_EVENT_OneParam(AGarCharacter, FGarCharacter_OnControllerChanged, AController*);

DECLARE_EVENT_OneParam(AGarCharacter, FGarCharacter_OnSetupPlayerInputComponent, UInputComponent*);

DECLARE_EVENT_FourParams(AGarCharacter, FGarCharacter_OnDebugDisplayDelegate, UCanvas*, const FDebugDisplayInfo&, float&, float&);

DECLARE_EVENT_OneParam(AGarCharacter, FGarCharacter_OnRefresh, float);

DECLARE_EVENT_OneParam(AGarCharacter, FGarCharacter_OnChangeGameplayTag, const FGameplayTag &);

UCLASS(Abstract, AutoExpandCategories = ("GarCharacter|Settings"))
class GAR_API AGarCharacter : public ACharacter, public IAbilitySystemInterface, public IGameplayCueInterface, public IGameplayTagAssetInterface
{
	GENERATED_UCLASS_BODY()

	friend UGarPhysicalAnimationComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter")
	TObjectPtr<UGarPhysicalAnimationComponent> PhysicalAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter")
	TObjectPtr<UGarAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter")
	TObjectPtr<UGarMotionWarpingComponent> MotionWarping;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings")
	TObjectPtr<UGarCharacterSettings> Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbilitySystem|Settings")
	TObjectPtr<UGarAbilitySet> AbilitySet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State", Replicated)
	FGameplayTag DesiredRotationMode{GarDesiredRotationModeTags::ViewDirection};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State", Replicated)
	FGameplayTag DesiredStance{GarDesiredStanceTags::Standing};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State", Replicated)
	FGameplayTag DesiredGait{GarDesiredGaitTags::Running};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State", ReplicatedUsing = OnReplicated_OverlayMode)
	FGameplayTag OverlayMode{GarOverlayModeTags::Default};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	TWeakObjectPtr<UGarCharacterMovementComponent> GarCharacterMovement;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	TWeakObjectPtr<UGarAnimationInstance> AnimationInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	FGameplayTag LocomotionMode{GarLocomotionModeTags::Grounded};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	FGameplayTag ViewMode{GarViewModeTags::ThirdPerson};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	FGarMovementBaseState MovementBase;

	// Replicated raw view rotation. Depending on the context, this rotation can be in world space, or in movement
	// base space. In most cases, it is better to use FGarViewState::Rotation to take advantage of network smoothing.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient, ReplicatedUsing = OnReplicated_ReplicatedViewRotation)
	FRotator ReplicatedViewRotation{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	FGarViewState ViewState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient, Replicated)
	FVector_NetQuantizeNormal InputDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient, Replicated,
			  Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float DesiredVelocityYawAngle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	FGarLocomotionState LocomotionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	FRotator PendingFocalRotationRelativeAdjustment{ForceInit};

	FTimerHandle BrakingFrictionFactorResetTimer;

public:
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* Property) const override;
#endif

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreRegisterAllComponents() override;

	virtual void PostRegisterAllComponents() override;

	virtual void PostInitializeComponents() override;

	FORCEINLINE UGarCharacterMovementComponent* GetGarCharacterMovement() const { return GarCharacterMovement.Get(); }

	FORCEINLINE UGarAnimationInstance* GetGarAnimationInstace() const { return AnimationInstance.Get(); }

	/** Name of the PhysicalAnimationComponent. */
	static FName PhysicalAnimationComponentName;

	/** Returns PhysicalAnimation subobject **/
	template <class T>
	FORCEINLINE_DEBUGGABLE T* GetPhysicalAnimation() const
	{
		return CastChecked<T>(PhysicalAnimation, ECastCheckedType::NullAllowed);
	}
	FORCEINLINE UGarPhysicalAnimationComponent* GetPhysicalAnimation() const { return PhysicalAnimation; }

	/** Name of the PhysicalAnimationComponent. */
	static FName AbilitySystemComponentName;

	FORCEINLINE UGarAbilitySystemComponent* GetGarAbilitySystem() const { return AbilitySystem; }

	/** Name of the MotionWarpingComponent. */
	static FName MotionWarpingComponentName;

	FORCEINLINE UGarMotionWarpingComponent* GetMotionWarping() const { return MotionWarping; }

protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

public:
	FGarCharacter_OnControllerChanged OnPossessed_Client;

	FGarCharacter_OnControllerChanged OnUnPossessed_Client;

	FGarCharacter_OnSetupPlayerInputComponent OnSetupPlayerInputComponent;

	FGarCharacter_OnRefresh OnRefresh;

	virtual void PostNetReceiveLocationAndRotation() override;

	virtual void OnRep_ReplicatedBasedMovement() override;

	virtual void Tick(float DeltaTime) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void UnPossessed() override;

	virtual void Restart() override;

	// IAbilitySystemInterface

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// IGameplayTagAssetInterface

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void ReplaceGarAbilitySystem(UGarAbilitySystemComponent *NewAbilitySystem);

private:
	UFUNCTION(Client, Reliable)
	void ClientPossessed(AController* NewContoller);

	UFUNCTION(Client, Reliable)
	void ClientUnPossessed();

	mutable FGameplayTagContainer TempTagContainer;

	//void RefreshMeshProperties() const;

	void RefreshMovementBase();

	// View Mode

public:
	const FGameplayTag& GetViewMode() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void SetViewMode(const FGameplayTag& NewViewMode);

protected:

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	void OnViewModeChanged(const FGameplayTag& PreviousViewMode);

	// Locomotion Mode

public:
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode = 0) override;

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	const FGameplayTag& GetLocomotionMode() const;

protected:
	void SetLocomotionMode(const FGameplayTag& NewLocomotionMode);

	UFUNCTION()
	virtual void NotifyLocomotionModeChanged(const FGameplayTag& PreviousLocomotionMode);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	void OnLocomotionModeChanged(const FGameplayTag& PreviousLocomotionMode);

	// Desired Rotation Mode

public:
	const FGameplayTag& GetDesiredRotationMode() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character", Meta = (AutoCreateRefTerm = "NewDesiredRotationMode"))
	void SetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode);

	// Rotation Mode

public:
	const FGameplayTag& GetRotationMode() const;

protected:
	void SetRotationMode(const FGameplayTag& NewRotationMode);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	void OnRotationModeChanged(const FGameplayTag& PreviousRotationMode);

	void RefreshRotationMode();

	// Desired Stance

public:
	const FGameplayTag& GetDesiredStance() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character", Meta = (AutoCreateRefTerm = "NewDesiredStance"))
	void SetDesiredStance(const FGameplayTag& NewDesiredStance);

protected:
	virtual void ApplyDesiredStance();

	// Stance

public:
	virtual bool CanCrouch() const override;

	virtual void Crouch(bool bClientSimulation = false) override;

	virtual void UnCrouch(bool bClientSimulation = false) override;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

public:
	const FGameplayTag& GetStance() const;

protected:
	void SetStance(const FGameplayTag& NewStance);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	void OnStanceChanged(const FGameplayTag& PreviousStance);

	// Desired Gait

public:
	const FGameplayTag& GetDesiredGait() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character", Meta = (AutoCreateRefTerm = "NewDesiredGait"))
	void SetDesiredGait(const FGameplayTag& NewDesiredGait);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetDesiredGait(const FGameplayTag& NewDesiredGait);

	// Gait

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	const FGameplayTag& GetGait() const;

protected:
	void SetGait(const FGameplayTag& NewGait);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	void OnGaitChanged(const FGameplayTag& PreviousGait);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	FGameplayTag LimitGaitIfNeeded(const FGameplayTag& NewGait) const;

private:
	void RefreshGait();

	bool CanSprint() const;

	// Overlay Mode

public:
	const FGameplayTag& GetOverlayMode() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character", Meta = (AutoCreateRefTerm = "NewOverlayMode"))
	void SetOverlayMode(const FGameplayTag& NewOverlayMode);

	FGarCharacter_OnChangeGameplayTag OnOverlayModeChanged;

private:
	void SetOverlayMode(const FGameplayTag& NewOverlayMode, bool bSendRpc);

	UFUNCTION(Client, Reliable)
	void ClientSetOverlayMode(const FGameplayTag& NewOverlayMode);

	UFUNCTION(Server, Reliable)
	void ServerSetOverlayMode(const FGameplayTag& NewOverlayMode);

	UFUNCTION()
	void OnReplicated_OverlayMode(const FGameplayTag& PreviousOverlayMode) const;

protected:

	// Locomotion Action

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	FGameplayTag GetLocomotionAction() const;

	// Input

public:
	const FVector& GetInputDirection() const;

protected:
	void SetInputDirection(FVector NewInputDirection);

	virtual void RefreshInput(float DeltaTime);

	// Controll Rotation Adjustment

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void SetFocalRotation(const FRotator& NewFocalRotation);

private:
	void TryAdjustControllRotation(float DeltaTime);

	// View

public:
	virtual FRotator GetViewRotation() const override;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void SetLookRotation(const FRotator& NewLookRotation);

private:
	void SetReplicatedViewRotation(const FRotator& NewViewRotation, bool bSendRpc);

	UFUNCTION(Server, Unreliable)
	void ServerSetReplicatedViewRotation(const FRotator& NewViewRotation);

	UFUNCTION()
	void OnReplicated_ReplicatedViewRotation();

	FRotator TargetLookRotation{NAN, NAN, NAN};

public:
	void CorrectViewNetworkSmoothing(const FRotator& NewTargetRotation, bool bRelativeTargetRotation);

public:
	const FGarViewState& GetViewState() const;

private:
	void RefreshView(float DeltaTime);

	void RefreshViewNetworkSmoothing(float DeltaTime);

	// Locomotion

public:
	const FGarLocomotionState& GetLocomotionState() const;

private:
	void SetDesiredVelocityYawAngle(float NewDesiredVelocityYawAngle);

	void RefreshLocomotionLocationAndRotation();

	void RefreshLocomotionEarly();

	void RefreshLocomotion(float DeltaTime);

	void RefreshLocomotionLate(float DeltaTime);

	// Jumping

public:
	virtual void Jump() override;

	// Rotation

public:
	virtual void FaceRotation(FRotator Rotation, float DeltaTime) override final;

	void RefreshRotationInstant(float TargetYawAngle, ETeleportType Teleport = ETeleportType::None);

private:
	void RefreshGroundedRotation(float DeltaTime);

protected:
	virtual bool RefreshCustomGroundedMovingRotation(float DeltaTime);

	virtual bool RefreshCustomGroundedNotMovingRotation(float DeltaTime);

	float CalculateGroundedMovingRotationInterpolationSpeed() const;

	void RefreshGroundedAimingRotation(float DeltaTime);

	bool RefreshConstrainedAimingRotation(float DeltaTime, bool bApplySecondaryConstraint = false);

private:
	void ApplyRotationYawSpeedAnimationCurve(float DeltaTime);

	void RefreshInAirRotation(float DeltaTime);

protected:
	virtual bool RefreshCustomInAirRotation(float DeltaTime);

	void RefreshInAirAimingRotation(float DeltaTime);

	void RefreshRotation(float TargetYawAngle, float DeltaTime, float RotationInterpolationSpeed);

	void RefreshRotationExtraSmooth(float TargetYawAngle, float DeltaTime,
	                                float RotationInterpolationSpeed, float TargetYawAngleRotationSpeed);

	void RefreshTargetYawAngleUsingLocomotionRotation();

	void RefreshTargetYawAngle(float TargetYawAngle);

	void RefreshViewRelativeTargetYawAngle();

	// ADS

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	float GetAimAmount() const;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	bool HasSight() const;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	void GetSightLocAndRot(FVector& Loc, FRotator& Rot) const;

	// Utility

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	const FGameplayTag& DesiredToActual(const FGameplayTag& SourceTag) const;

	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	bool IsCharacterSelf() const;

	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	bool HasServerRole() const;

	// Others

public:
	bool IsLied() const;

	void SetIsLied(bool bNewIsLied);

	UFUNCTION(BlueprintCallable, Category = Character)
	virtual bool CanLie() const;

	UFUNCTION(BlueprintCallable, Category = Character)
	virtual void Lie();

	virtual void OnStartLie(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust);

	virtual void OnEndLie(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust);

	UFUNCTION(BlueprintImplementableEvent, Meta = (DisplayName = "OnStartLie", ScriptName = "OnStartLie"))
	void K2_OnStartLie(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	UFUNCTION(BlueprintImplementableEvent, Meta = (DisplayName = "OnEndLie", ScriptName = "OnEndLie"))
	void K2_OnEndLie(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings", Meta = (ForceUnits = "s"))
	float CapsuleUpdateSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "GarCharacter|State", replicatedUsing = OnRep_IsLied)
	uint32 bIsLied : 1;

private:
	void RefreshCapsuleSize(float DeltaTime);

	void UpdateCapsule(float DeltaTime, float EyeHeight, float EyeHeightSpeed, float HalfHeight, float HalfHeightSpeed, float Radius, float RadiusSpeed);

	/** Handle Lying replicated from server */
	UFUNCTION()
	virtual void OnRep_IsLied();

#if !UE_BUILD_SHIPPING
	// Debug

public:
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation) override;

	FGarCharacter_OnDebugDisplayDelegate OnDisplayDebug;

private:
	static void DisplayDebugHeader(const UCanvas* Canvas, const FText& HeaderText, const FLinearColor& HeaderColor,
	                               float Scale, float HorizontalLocation, float& VerticalLocation);

	TArray<FName> CurveNames;

	void InitializeCurveNames();

	void DisplayDebugCurves(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugState(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugShapes(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugTraces(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugMantling(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;
#endif // !UE_BUILD_SHIPPING
};

inline void AGarCharacter::ReplaceGarAbilitySystem(UGarAbilitySystemComponent* NewAbilitySystem)
{
	AbilitySystem = NewAbilitySystem;
}

inline const FGameplayTag& AGarCharacter::GetDesiredRotationMode() const
{
	return DesiredRotationMode;
}

inline const FGameplayTag& AGarCharacter::GetDesiredStance() const
{
	return DesiredStance;
}

inline const FGameplayTag& AGarCharacter::GetDesiredGait() const
{
	return DesiredGait;
}

inline const FGameplayTag& AGarCharacter::GetLocomotionMode() const
{
	return LocomotionMode;
}

inline const FGameplayTag& AGarCharacter::GetViewMode() const
{
	return ViewMode;
}

inline const FGameplayTag& AGarCharacter::GetOverlayMode() const
{
	return OverlayMode;
}

inline const FVector& AGarCharacter::GetInputDirection() const
{
	return InputDirection;
}

inline const FGarViewState& AGarCharacter::GetViewState() const
{
	return ViewState;
}

inline const FGarLocomotionState& AGarCharacter::GetLocomotionState() const
{
	return LocomotionState;
}

inline bool AGarCharacter::IsLied() const
{
	return bIsLied;
}
