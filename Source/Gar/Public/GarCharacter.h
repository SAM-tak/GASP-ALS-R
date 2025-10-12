#pragma once

#include "CoreMinimal.h"
#include "MoverSimulationTypes.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GarGameplayTags.h"
#include "GarCharacter.generated.h"

class UCapsuleComponent;
class UMotionWarpingComponent;
class UGarCharacterSettings;
class UGarAnimationInstance;
class UGarCharacterMoverComponent;
class UGarPhysicalAnimationComponent;
class UGarAbilitySystemComponent;

DECLARE_EVENT_OneParam(AGarCharacter, FGarCharacter_OnPossessorChanged, AController*);

DECLARE_EVENT_OneParam(AGarCharacter, FGarCharacter_OnSetupPlayerInputComponent, UInputComponent*);

DECLARE_EVENT_OneParam(AGarCharacter, FGarCharacter_OnTick, float);

DECLARE_EVENT_OneParam(AGarCharacter, FGarCharacter_OnChangeGameplayTag, const FGameplayTag &);

#if !UE_BUILD_SHIPPING
DECLARE_EVENT_FourParams(AGarCharacter, FGarCharacter_OnDebugDisplayDelegate, UCanvas*, const FDebugDisplayInfo&, float&, float&);
#endif

UCLASS(Abstract, AutoExpandCategories = ("GarCharacter|Settings"))
class GAR_API AGarCharacter : public APawn, public IMoverInputProducerInterface,
	public IAbilitySystemInterface, public IGameplayCueInterface, public IGameplayTagAssetInterface
{
	GENERATED_UCLASS_BODY()

	friend UGarPhysicalAnimationComponent;
	friend UGarCharacterMoverComponent;

protected:
	/** The root capsule collider associated with this Character (optional sub-object). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> Capsule;

	/** The alternative capsule collider for prone (optional sub-object). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> ProneCapsule;

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter")
	TObjectPtr<UGarCharacterMoverComponent> CharacterMover;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter")
	TObjectPtr<UGarAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter")
	TObjectPtr<UGarPhysicalAnimationComponent> PhysicalAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacter|Settings")
	TObjectPtr<UGarCharacterSettings> Settings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings")
	float CrouchedCapsuleHalfHeight{55.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings")
	float CrouchedEyeHeight{45.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings")
	float LiedCapsuleHalfHeight{30.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings")
	float LiedProneCapsuleHalfHeight{30.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings")
	float LiedProneCapsuleZOffset{30.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacter|Settings")
	float LiedEyeHeight{35.0f};

	// Whether or not we author our movement inputs relative to whatever base we're standing on, or leave them in world space. Only applies if standing on a base of some sort.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarCharacter|Settings")
	bool bUseBaseRelativeMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMovement|MovementCapabilities", meta = (DisplayName = "Can Lie"))
	uint8 NavAgentProps_bCanLie : 1{true};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State")
	FGameplayTag InitialDesiredRotationMode{GarDesiredRotationModeTags::ViewDirection};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State")
	FGameplayTag InitialDesiredStance{GarDesiredStanceTags::Standing};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State")
	FGameplayTag InitialDesiredGait{GarDesiredGaitTags::Running};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State")
	FGameplayTag InitialPerspective{GarPerspectiveTags::ThirdPerson};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State")
	FGameplayTag InitialOverlay{FGameplayTag::EmptyTag};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GarCharacter|Settings|Desired State")
	FGameplayTag InitialDeltaOverlay{FGameplayTag::EmptyTag};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter|State", Transient, Replicated)
	FRotator ReplicatedControlRotation{ForceInit};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	TWeakObjectPtr<UGarAnimationInstance> AnimationInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	FVector InputDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	float InputYawAngle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	FRotator PendingFocalRotationRelativeAdjustment{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	float InitialEyeHeight{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	float InitialCapsuleHalfHeight{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	float InitialCapsuleRadius{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	float InitialProneCapsuleHalfHeight{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	float InitialProneCapsuleRadius{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	float InitialMeshZ{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacter|State", Transient)
	float InitialProneCapsuleX{0.0f};

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;

	/** Name of the PhysicalAnimationComponent. */
	static FName CapsuleComponentName;

	// Accessor for the character's skeletal mesh component
	FORCEINLINE UCapsuleComponent* GetCapsule() const { return Capsule; }

	/** Name of the PhysicalAnimationComponent. */
	static FName ProneCapsuleComponentName;

	// Accessor for the character's skeletal mesh component
	FORCEINLINE UCapsuleComponent* GetProneCapsule() const { return ProneCapsule; }

	/** Name of the PhysicalAnimationComponent. */
	static FName SkeletalMeshComponentName;

	// Accessor for the character's skeletal mesh component
	FORCEINLINE USkeletalMeshComponent* GetMesh() const { return Mesh; }

	/** Name of the GarCharacterMoverComponent. */
	static FName CharacterMoverComponentName;

	// Accessor for the actor's movement component
	FORCEINLINE UGarCharacterMoverComponent* GetMover() const { return CharacterMover; }

	/** Name of the PhysicalAnimationComponent. */
	static FName PhysicalAnimationComponentName;

	/** Returns PhysicalAnimation subobject **/
	FORCEINLINE UGarPhysicalAnimationComponent* GetPhysicalAnimation() const { return PhysicalAnimation; }

	/** Name of the PhysicalAnimationComponent. */
	static FName AbilitySystemComponentName;

	FORCEINLINE UGarAbilitySystemComponent* GetGarAbilitySystem() const { return AbilitySystem; }

	/** Name of the MotionWarpingComponent. */
	static FName MotionWarpingComponentName;

	FORCEINLINE UMotionWarpingComponent* GetMotionWarping() const { return MotionWarping; }

	// Accessor for the mesh's GarAnimationInstance
	FORCEINLINE UGarAnimationInstance* GetGarAnimationInstace() const { return AnimationInstance.Get(); }

protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	// Entry point for input production. Do not override. To extend in derived character types,
	// override OnProduceInput for native types or implement "Produce Input" blueprint event
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

public:
	FGarCharacter_OnPossessorChanged OnPossessed_Client;

	FGarCharacter_OnPossessorChanged OnUnPossessed_Client;

	FGarCharacter_OnSetupPlayerInputComponent OnSetupPlayerInputComponent;

	FGarCharacter_OnTick OnTick;

	virtual void Tick(float DeltaTime) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void UnPossessed() override;

	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false) override;

	virtual FVector ConsumeMovementInputVector() override;

	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	virtual FRotator GetViewRotation() const override;

	virtual FVector GetVelocity() const override;

	// IAbilitySystemInterface

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// IGameplayTagAssetInterface

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void ReplaceAbilitySystem(UGarAbilitySystemComponent* NewAbilitySystem)
	{
		AbilitySystem = NewAbilitySystem;
	}

private:
	UFUNCTION(Client, Reliable)
	void ClientPossessed(AController* NewContoller);

	UFUNCTION(Client, Reliable)
	void ClientUnPossessed();

	mutable FGameplayTagContainer PrevTagContainer; // for ProduceInput

	// Locomotion
public:
	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	void OnLocomotionModeChanged(const FGameplayTag& PreviousLocomotionMode);

	// Perspective

private:
	FGameplayTag GetPerspective() const;

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void SetPerspective(const FGameplayTag& NewPerspective);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	void OnPerspectiveChanged(const FGameplayTag& PreviousPerspective);

	// Desired Rotation Mode

private:
	FGameplayTag GetDesiredRotationMode() const;

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Character", Meta = (AutoCreateRefTerm = "NewDesiredRotationMode"))
	void SetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode);

	// Rotation Mode

protected:

	void RefreshRotationMode();

	// Desired Stance

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Character", Meta = (AutoCreateRefTerm = "NewDesiredStance"))
	void SetDesiredStance(const FGameplayTag& NewDesiredStance);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "GAR|Character")
	bool CanCrouch() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "GAR|Character")
	bool CanUnCrouch() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "GAR|Character")
	bool CanLie() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void Crouch();

	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void UnCrouch();

	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void Lie();

	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	bool IsCrouching() const;

	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	bool IsLying() const;

protected:
	virtual void ApplyDesiredStance();

	void CheckCanUnCrouchIfNeeded();

	void CheckCanCrouchIfNeeded();

	void CheckCanLieIfNeeded();

	// Stance

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void SetInputStance(const FGameplayTag &NewInputStance);

protected:
	bool UpdateMainCapsule(float DeltaTime, float TargetHalfHeight, float HeightSpeed, float TargetRadius, float RadiusSpeed);

	bool UpdateProneCapsule(float DeltaTime, float TargetHalfHeight, float HeightSpeed, float TargetRadius, float RadiusSpeed,
		float TargetOffset, float OffsetSpeed);

	void RefreshCapsuleSize(float DeltaTime);

	void RefreshEyeHeight(float DeltaTime);

private:
	FVector MovementInputVector = FVector::ZeroVector;

	bool bUnCrouchBlocked = false;
	bool bCrouchBlocked = false;
	bool bLieBlocked = false;

	// Desired Gait

private:
	FGameplayTag GetDesiredGait() const;

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Character", Meta = (AutoCreateRefTerm = "NewDesiredGait"))
	void SetDesiredGait(const FGameplayTag& NewDesiredGait);

	// Gait

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "GAR|Character")
	bool CanSprint() const;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Character")
	FGameplayTag LimitGaitIfNeeded(const FGameplayTag& NewGait) const;

private:
	void RefreshGait();

	// Locomotion Action

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	FGameplayTag GetLocomotionAction() const;

	// Input

public:
	FORCEINLINE const FVector& GetInputDirection() const { return InputDirection; }

	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	FORCEINLINE bool HasMovementInput() const { return MovementInputVector.SizeSquared() > UE_KINDA_SMALL_NUMBER; }

	FORCEINLINE float GetInputYawAngle() const { return InputYawAngle; }

protected:
	virtual void RefreshInput();

	UFUNCTION(Server, Unreliable)
	void ServerSetControlRotation(const FRotator& NewControlRotation);

private:
	FORCEINLINE bool HasSpeed() const { return GetVelocity().Size2D() > 1.0; }

	bool IsMoving() const;

	UPROPERTY(VisibleInstanceOnly, Category = "GarCharacter|State", Transient)
	FGameplayTag InputRotationMode{GarRotationModeTags::ViewDirection};

	UPROPERTY(VisibleInstanceOnly, Category = "GarCharacter|State", Transient)
	FGameplayTag InputStance{GarStanceTags::Standing};

	UPROPERTY(VisibleInstanceOnly, Category = "GarCharacter|State", Transient)
	FGameplayTag InputGait{GarGaitTags::Running};

	// Controll Rotation Adjustment

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void SetFocalRotation(const FRotator& NewFocalRotation);

private:
	void TryAdjustControllRotation(float DeltaTime);

	// Sprint

private:
	void RefreshSprintState();

	// Jumping

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "GAR|Character")
	bool CanJump() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void Jump();

	UFUNCTION(BlueprintCallable, Category = "GAR|Character")
	void StopJumping();

private:
	bool bIsJumpJustPressed = false;
	bool bIsJumpPressed = false;

	// ADS

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	float GetAimAmount() const;

	UFUNCTION(BlueprintPure, BlueprintImplementableEvent, Category = "GAR|Character")
	bool HasSight() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|Character")
	void GetSightLocAndRot(FVector& Loc, FRotator& Rot) const;

	// Utility

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Character")
	const FGameplayTag& DesiredToActual(const FGameplayTag& SourceTag) const;

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

	void DisplayDebugShapes(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugTraces(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugTraversal(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;
#endif // !UE_BUILD_SHIPPING
};
