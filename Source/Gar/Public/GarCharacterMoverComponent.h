#pragma once

#include "MoverComponent.h"
#include "GarGameplayTags.h"
#include "GarCharacterMoverComponent.generated.h"

class UMoverTrajectoryPredictor;
class UMotionWarpingMoverAdapter;
class AGarCharacter;
class UGarMovementSettings;

/**
 * Fires when a stance is changed, if stance handling is enabled (see @SetHandleStanceChanges)
 * Note: If a stance was just Activated it will fire with an invalid OldStance
 *		 If a stance was just Deactivated it will fire with an invalid NewStance
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGarMover_OnStanceChanged, FGameplayTag, OldStance, FGameplayTag, NewStance);

UCLASS(ClassGroup = "GAR", BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GAR_API UGarCharacterMoverComponent : public UMoverComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacterMover|Settings")
	FGameplayTagContainer LocomotionModeTags{GarLocomotionModeTags::Root};

	// Whether this component should directly handle jumping or not 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarCharacterMover|Settings")
	uint8 bHandleJump : 1 = 1;

	// Whether this component should directly handle stance changes, including crouching input
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarCharacterMover|Settings")
	uint8 bHandleStanceChanges : 1 = 1;

	// Broadcast when this actor changes stances.
	UPROPERTY(BlueprintAssignable, Category = GarCharacterMover)
	FGarMover_OnStanceChanged OnStanceChanged;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	TWeakObjectPtr<AGarCharacter> Character;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	FGameplayTag LocomotionMode{GarLocomotionModeTags::Grounded};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	FGameplayTag RotationMode{GarRotationModeTags::ViewDirection};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	FGameplayTag Stance{GarStanceTags::Standing};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	FGameplayTag Gait{GarGaitTags::Running};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	TObjectPtr<UMoverTrajectoryPredictor> TrajectoryPredictor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	TObjectPtr<UMotionWarpingMoverAdapter> MotionWarpingMoverAdapter;

public:
	/** X = Forward Speed, Y = Strafe Speed, Z = Backwards Speed */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	FVector CurrentMaxSpeed{ForceInit};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	float CurrentAcceleration{0.0f};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	float CurrentDeceleration{0.0f};

	// Constants

	static constexpr float MIN_FLOOR_DIST = 1.9f;	// Smallest distance we want our primitive floating above walkable floors while in ground-based movement.
	static constexpr float MAX_FLOOR_DIST = 2.4f;	// Largest distance we want our primitive floating above walkable floors while in ground-based movement.

public:
	UGarCharacterMoverComponent();

	virtual void InitializeComponent() override;

	virtual void BeginPlay() override;

protected:
	virtual bool Jump();

	UFUNCTION()
	virtual void OnMoverPreSimulationTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd);

	UFUNCTION()
	virtual void OnMoverMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName);

public:
	UFUNCTION(BlueprintPure, Category = "GAR|CharacterMover")
	const UGarMovementSettings* GetSettings() const { return Settings; }

	const FGameplayTag& GetLocomotionMode() const { return LocomotionMode; }

	const FGameplayTag& GetRotationMode() const { return RotationMode; }

	const FGameplayTag& GetStance() const { return Stance; }

	const FGameplayTag& GetGait() const { return Gait; }

	UMoverTrajectoryPredictor* GetTrajectoryPredictor() const { return TrajectoryPredictor; }

	UMotionWarpingMoverAdapter* GetMotionWarpingMoverAdapter() const { return MotionWarpingMoverAdapter; }

	void AppendOwnedGameplayTags(FGameplayTagContainer& TagContainer);

	void SetInitialGameplayTags(const FGameplayTag& InRotationMode, const FGameplayTag& InStance, const FGameplayTag& InGait);

	/** Return true if the hit result should be considered a walkable surface for the character. */
	UFUNCTION(BlueprintCallable, Category = "GAR|CharacterMover")
	virtual bool IsWalkable(const FHitResult& Hit) const;

private:
	TObjectPtr<const UGarMovementSettings> Settings;
};
