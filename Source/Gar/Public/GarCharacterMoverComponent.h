#pragma once

#include "MoverComponent.h"
#include "GarGameplayTags.h"
#include "GarCharacterMoverComponent.generated.h"

class UMoverTrajectoryPredictor;
class UMotionWarpingMoverAdapter;
class AGarCharacter;
class UGarMovementSettings;

UCLASS(ClassGroup = "GAR", BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GAR_API UGarCharacterMoverComponent : public UMoverComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GarCharacterMover|Settings")
	FGameplayTagContainer LocomotionModeTags{GarLocomotionModeTags::Root};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient)
	TWeakObjectPtr<AGarCharacter> Character;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient, Replicated, ReplicatedUsing = OnReplicated_LocomotionMode)
	FGameplayTag LocomotionMode{GarLocomotionModeTags::Grounded};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient, Replicated)
	FGameplayTag RotationMode{GarRotationModeTags::ViewDirection};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient, Replicated)
	FGameplayTag Stance{GarStanceTags::Standing};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GarCharacterMover|State", Transient, Replicated)
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void InitializeComponent() override;

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	virtual void OnMoverPreSimulationTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd);

	UFUNCTION()
	virtual void OnMoverMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName);

	UFUNCTION()
	void OnReplicated_LocomotionMode(const FGameplayTag& PreviousOverlayMode) const;

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
