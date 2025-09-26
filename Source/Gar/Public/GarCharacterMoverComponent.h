#pragma once

#include "MoverComponent.h"
#include "GarGameplayTags.h"
#include "GarCharacterMoverComponent.generated.h"

class UMoverTrajectoryPredictor;
class UMotionWarpingMoverAdapter;
class AGarCharacter;
class UGarMovementSettings;

/** TurnTo: Turn Actor from the starting location to the target location over a duration of time.*/
USTRUCT(BlueprintType)
struct GAR_API FGarLayeredMove_TurnTo : public FLayeredMoveBase
{
	GENERATED_BODY()

	FGarLayeredMove_TurnTo();
	virtual ~FGarLayeredMove_TurnTo() {}

	// Location to Start the MoveTo move from
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mover)
	FRotator StartRotation;
	
	// Location to move towards
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mover)
	FRotator TargetRotation;

	// Optional CurveFloat to apply to how fast the actor moves as they get closer to the target location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mover)
	TObjectPtr<UCurveFloat> TimeMappingCurve;
	
	// Generate a movement 
	virtual bool GenerateMove(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, const UMoverComponent* MoverComp, UMoverBlackboard* SimBlackboard, FProposedMove& OutProposedMove) override;

	virtual FLayeredMoveBase* Clone() const override;

	virtual void NetSerialize(FArchive& Ar) override;

	virtual UScriptStruct* GetScriptStruct() const override;

	virtual FString ToSimpleString() const override;

	virtual void AddReferencedObjects(class FReferenceCollector& Collector) override;
	
protected:
	// helper function to apply movement vector offset from the PathOffsetCurve
	FVector GetPathOffsetInWorldSpace(const float MoveFraction) const;

	// Helper function to apply TimeMappingCurve to the layered move
	float EvaluateFloatCurveAtFraction(const UCurveFloat& Curve, const float Fraction) const;
};

template<>
struct TStructOpsTypeTraits< FGarLayeredMove_TurnTo > : public TStructOpsTypeTraitsBase2< FGarLayeredMove_TurnTo >
{
	enum
	{
		//WithNetSerializer = true,
		WithCopy = true
	};
};


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

	void SetLocomotionMode(const FGameplayTag& NewLocomotionMode);

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

	void AppendOwnedGameplayTags(FGameplayTagContainer& TagContainer);

	void SetInitialGameplayTags(const FGameplayTag& InRotationMode, const FGameplayTag& InStance, const FGameplayTag& InGait);

	/** Return true if the hit result should be considered a walkable surface for the character. */
	UFUNCTION(BlueprintCallable, Category = "GAR|CharacterMover")
	virtual bool IsWalkable(const FHitResult& Hit) const;

private:
	TObjectPtr<const UGarMovementSettings> Settings;
};
