#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "Settings/GarMovementSettings.h"
#include "GarCharacterMovementComponent.generated.h"

using FGarPhysicsRotationDelegate = TMulticastDelegate<void(float DeltaTime)>;

struct FGarCharacterMovementComponentAsyncInput : public FCharacterMovementComponentAsyncInput
{
	FGameplayTag RotationMode{GarRotationModeTags::ViewDirection};
	FGameplayTag Stance{GarStanceTags::Standing};
	FGameplayTag MaxAllowedGait{GarGaitTags::Walking};
	bool bWantsToLie;
	bool bIsLied;
};

struct FGarCharacterMovementComponentAsyncOutput : public FCharacterMovementComponentAsyncOutput
{
	FGameplayTag RotationMode{GarRotationModeTags::ViewDirection};
	FGameplayTag Stance{GarStanceTags::Standing};
	FGameplayTag MaxAllowedGait{GarGaitTags::Walking};
	bool bWantsToLie;
	bool bIsLied;
};

class GAR_API FGarCharacterMovementComponentAsyncCallback
	: public Chaos::TSimCallbackObject<FGarCharacterMovementComponentAsyncInput, FGarCharacterMovementComponentAsyncOutput>
{
public:
	virtual FName GetFNameForStatId() const override;
private:
	virtual void OnPreSimulate_Internal() override;
};

class GAR_API FGarCharacterNetworkMoveData : public FCharacterNetworkMoveData
{
private:
	using Super = FCharacterNetworkMoveData;

public:
	FGameplayTag RotationMode{GarRotationModeTags::ViewDirection};

	FGameplayTag Stance{GarStanceTags::Standing};

	FGameplayTag MaxAllowedGait{GarGaitTags::Walking};

public:
	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& Move, ENetworkMoveType MoveType) override;

	virtual bool Serialize(UCharacterMovementComponent& Movement, FArchive& Archive, UPackageMap* Map, ENetworkMoveType MoveType) override;
};

class GAR_API FGarCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
public:
	TStaticArray<FGarCharacterNetworkMoveData, 3> MoveData;

public:
	FGarCharacterNetworkMoveDataContainer();
};

class GAR_API FGarSavedMove : public FSavedMove_Character
{
private:
	using Super = FSavedMove_Character;

public:
	FGameplayTag RotationMode{GarRotationModeTags::ViewDirection};

	FGameplayTag Stance{GarStanceTags::Standing};

	FGameplayTag MaxAllowedGait{GarGaitTags::Walking};

	bool bWantsToLie{false};

public:
	virtual void Clear() override;

	virtual void SetMoveFor(ACharacter* Character, float NewDeltaTime, const FVector& NewAcceleration,
	                        FNetworkPredictionData_Client_Character& PredictionData) override;

	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDeltaTime) const override;

	virtual void CombineWith(const FSavedMove_Character* PreviousMove, ACharacter* Character,
	                         APlayerController* Player, const FVector& PreviousStartLocation) override;

	virtual void PrepMoveFor(ACharacter* Character) override;
};

class GAR_API FGarNetworkPredictionData : public FNetworkPredictionData_Client_Character
{
private:
	using Super = FNetworkPredictionData_Client_Character;

public:
	explicit FGarNetworkPredictionData(const UCharacterMovementComponent& Movement);

	virtual FSavedMovePtr AllocateNewMove() override;
};

UCLASS(ClassGroup = "GAR")
class GAR_API UGarCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	friend FGarSavedMove;

protected:
	FGarCharacterNetworkMoveDataContainer MoveDataContainer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMovement|MovementCapabilities", meta = (DisplayName = "Can Lie"))
	uint8 NavAgentProps_bCanLie : 1{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	TObjectPtr<UGarMovementSettings> MovementSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	FGarMovementGaitSettings GaitSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	FGameplayTag RotationMode{GarRotationModeTags::ViewDirection};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	FGameplayTag Stance{GarStanceTags::Standing};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	FGameplayTag MaxAllowedGait{GarGaitTags::Walking};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	uint8 bMovementModeLocked : 1 {false};

	// Used to temporarily prohibit the player from moving the character. Garo works for AI-controlled characters.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	uint8 bInputBlocked : 1 {false};

	// Valid only on locally controlled characters.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	FRotator PreviousControlRotation{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	FVector PrePenetrationAdjustmentVelocity{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMovement|State", Transient)
	uint8 bPrePenetrationAdjustmentVelocityValid : 1 {false};

public:
	FGarPhysicsRotationDelegate OnPhysicsRotation;

public:
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* Property) const override;
#endif

	virtual void BeginPlay() override;

	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual bool ShouldPerformAirControlForPathFollowing() const override;

	virtual void UpdateBasedRotation(FRotator& FinalRotation, const FRotator& ReducedRotation) override;

	virtual bool ApplyRequestedMove(float DeltaTime, float CurrentMaxAcceleration, float MaxSpeed, float Friction,
	                                float BrakingDeceleration, FVector& RequestedAcceleration, float& RequestedSpeed) override;

	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual float GetMaxAcceleration() const override;

	virtual float GetMaxBrakingDeceleration() const override;

protected:
	virtual void ControlledCharacterMove(const FVector& InputVector, float DeltaTime) override;

public:
	virtual void PhysicsRotation(float DeltaTime) override;

protected:
	virtual void PhysWalking(float DeltaTime, int32 IterationCount) override;

	virtual void PhysNavWalking(float DeltaTime, int32 IterationCount) override;

	virtual void PhysCustom(float DeltaTime, int32 IterationCount) override;

	virtual FVector ConsumeInputVector() override;

public:
	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult,
	                              float SweepRadius, const FHitResult* DownwardSweepResult) const override;

protected:
	virtual void PerformMovement(float DeltaTime) override;

public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	virtual void SmoothClientPosition(float DeltaTime) override;

	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAcceleration) override;

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Character Movement")
	void SetMovementSettings(UGarMovementSettings* NewMovementSettings);

	const FGarMovementGaitSettings& GetGaitSettings() const;

private:
	void RefreshGaitSettings();

public:
	void SetRotationMode(const FGameplayTag& NewRotationMode);

	void SetStance(const FGameplayTag& NewStance);

	void SetMaxAllowedGait(const FGameplayTag& NewMaxAllowedGait);

private:
	void RefreshMaxWalkSpeed();

public:
	float CalculateGaitAmount() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Character Movement")
	void SetMovementModeLocked(bool bNewMovementModeLocked);

	UFUNCTION(BlueprintCallable, Category = "GAR|Character Movement")
	void SetInputBlocked(bool bNewInputBlocked);

	bool TryConsumePrePenetrationAdjustmentVelocity(FVector& OutVelocity);

public:
	/** If true, try to lie (or keep lying down) on next update. If false, try to stop lying on next update. */
	UPROPERTY(Category = "Character Movement (General Settings)", VisibleInstanceOnly, BlueprintReadOnly)
	uint8 bWantsToLie : 1{false};

	virtual void Crouch(bool bClientSimulation = false) override;
	virtual void UnCrouch(bool bClientSimulation = false) override;
	virtual bool IsLying() const;
	virtual void Lie(bool bClientSimulation = false);
	virtual void UnLie(bool bClientSimulation = false);
	virtual bool CanLieInCurrentState() const;

	FORCEINLINE bool CanEverLie() const { return NavAgentProps_bCanLie; }

	virtual bool CanAttemptJump() const override;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	
	TObjectPtr<class AGarCharacter> GetGarCharacter() const;

	virtual void UpdateCapsuleSize(float DeltaTime, float TargetHalfHeight, float HeightSpeed, float TargetRadius, float RadiusSpeed);

protected:
	/* Prepare inputs for asynchronous simulation on physics thread */
	virtual void FillAsyncInput(const FVector& InputVector, FCharacterMovementComponentAsyncInput& AsyncInput) override;
	virtual void BuildAsyncInput() override;
	/* Apply outputs from async sim. */
	virtual void ApplyAsyncOutput(FCharacterMovementComponentAsyncOutput& Output) override;
	virtual void ProcessAsyncOutput() override;

	/* Register async callback with physics system. */
	virtual void RegisterAsyncCallback() override;
	virtual bool IsAsyncCallbackRegistered() const override;

private:
	FGarCharacterMovementComponentAsyncCallback* GarAsyncCallback;
};

inline const FGarMovementGaitSettings& UGarCharacterMovementComponent::GetGaitSettings() const
{
	return GaitSettings;
}
