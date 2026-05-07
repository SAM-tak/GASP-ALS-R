// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Actions/GarGameplayAbility_MotionMatchBase.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GarGameplayAbility_Traversal.generated.h"

class UCurveFloat;
class UPrimitiveComponent;
class UAnimMontage;
class UMotionWarpingComponent;
class UChooserTable;
class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct GAR_API FGarTraversalChooserInputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bHasFrontLedge : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bHasBackLedge : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bHasBackFloor : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float ObstacleHeight{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float ObstacleDepth{0.0f};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float BackFloorFall{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FGameplayTagContainer CurrentGameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float Speed{0.0f};
};

USTRUCT(BlueprintType)
struct GAR_API FGarTraversalChooserOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FGameplayTagContainer Tags;
};

USTRUCT(BlueprintType)
struct GAR_API FGarTraversalTraceResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TWeakObjectPtr<UPrimitiveComponent> TargetPrimitive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector FrontWallNormal{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector FrontLedgeLocation{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector UpperLedgeNormal{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector BackLedgeLocation{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bHasBackFloor : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector BackFloorLocation{FVector::ZeroVector};
};

USTRUCT(BlueprintType)
struct GAR_API FGarTraversalParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FGarTraversalChooserOutput ChooserOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FPoseSearchBlueprintResult Result;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TWeakObjectPtr<UPrimitiveComponent> TargetPrimitive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector FrontWallNormal{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector FrontLedgeLocation{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector UpperLedgeNormal{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector BackLedgeLocation{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector BackFloorLocation{FVector::ZeroVector};
};

/** FGarTraversalParameters をネット越しに運ぶための TargetData。
 *  AutonomousProxy のクライアントが選定したモンタージュ・トレース結果をサーバーへ転送する。 */
USTRUCT()
struct GAR_API FGarTraversalTargetData : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGarTraversalParameters Parameters;

	// TStructOpsTypeTraits による NetSerialize。override 不要（virtual ではなくトレイト経由で呼ばれる）。
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FGarTraversalTargetData::StaticStruct();
	}
	virtual FString ToString() const override { return TEXT("FGarTraversalTargetData"); }
};

template <>
struct TStructOpsTypeTraits<FGarTraversalTargetData> : public TStructOpsTypeTraitsBase2<FGarTraversalTargetData>
{
	enum { WithNetSerializer = true };
};

USTRUCT(BlueprintType)
struct GAR_API FGarTraversalTraceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0))
	FVector2f LedgeHeight{50.0f, 250.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float ReachDistance{75.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bDrawFailedTraces : 1{false};
};

/**
 * Ragdolling
 */
UCLASS(Abstract)
class GAR_API UGarGameplayAbility_Traversal : public UGarGameplayAbility_MotionMatchBase
{
	GENERATED_UCLASS_BODY()

public:
	/** BP から TryActivateAbilityByClass の代わりに呼ぶ。
	 *  クライアント側でトレース・モンタージュ選定を行い、EventData に乗せてサーバーへ送る。
	 *  戻り値: ローカル側でアビリティを起動できた場合 true。 */
	UFUNCTION(BlueprintCallable, Category = "GAR|Ability|Traversal")
	static bool TryActivateTraversal(UAbilitySystemComponent* InASC,
									 TSubclassOf<UGarGameplayAbility_Traversal> AbilityClass);

	/** トラバーサル起動に使うゲームプレイイベントタグ。
	 *  この能力の AbilityTrigger と一致させること。デフォルト: Gar.Event.Traversal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	FGameplayTag TraversalEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	TObjectPtr<UChooserTable> Chooser;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	FName ExcludeTargetTag{TEXTVIEW("Not Traversable")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float StaticSpeedThreshold{50.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float TraceAngleThreshold{110.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float MaxReachAngle{50.0f};

	// Prevents mantling on surfaces whose slope angle exceeds this value.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ClampMin = 0, ClampMax = 90, ForceUnits = "deg"))
	float WallAngleThreshold{30.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", AdvancedDisplay, Meta = (ClampMin = 0, ClampMax = 1))
	float WallAngleThresholdCos{FMath::Cos(FMath::DegreesToRadians(30.0f))};

	// Prevents mantling on surfaces whose slope angle exceeds this value.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ClampMin = 0, ClampMax = 90, ForceUnits = "deg"))
	float SlopeAngleThreshold{35.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", AdvancedDisplay, Meta = (ClampMin = 0, ClampMax = 1))
	float SlopeAngleThresholdCos{FMath::Cos(FMath::DegreesToRadians(35.0f))};

	// If a dynamic object has a speed bigger than this value, then do not start mantling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ForceUnits = "cm/s"))
	float TargetPrimitiveSpeedThreshold{10.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	FGarTraversalTraceSettings GroundedTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	FGarTraversalTraceSettings InAirTrace{{50.0f, 150.0f}, 70.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	TEnumAsByte<ECollisionChannel> TraversalTraceChannel{ECC_Visibility};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	TArray<TEnumAsByte<ECollisionChannel>> TraversalTraceResponseChannels{ECC_WorldStatic, ECC_WorldDynamic, ECC_Destructible};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", AdvancedDisplay)
	FCollisionResponseContainer TraversalTraceResponses{ECR_Ignore};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ClampMin = 0, ForceUnits = "cm"))
	float MinimumDepth{25.0f};

	// If checked, ragdolling will start if the object the character is mantling on was destroyed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	bool bOffCollisitonInAction{false};

	// If checked, ragdolling will start if the object the character is mantling on was destroyed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	bool bChangeCapsuleInAction{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ClampMin = 0, ForceUnits = "cm",
		EditCondition = "bChangeCapsuleInAction"))
	float CapsuleHalfHeightWhileInAction{40.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (ClampMin = 0, ForceUnits = "cm",
		EditCondition = "bChangeCapsuleInAction"))
	float CapsuleRadiusWhileInAction{20.0f};

	// If checked, ragdolling will start if the object the character is mantling on was destroyed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal")
	bool bStartRagdollingOnTargetPrimitiveDestruction{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GarAbility|Traversal", Meta = (EditCondition = "bStartRagdollingOnTargetPrimitiveDestruction"))
	FGameplayTag TryActiveOnPrimitiveDestruction{GarLocomotionActionTags::FreeFalling};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	TWeakObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	TObjectPtr<UAnimMontage> ChosenMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	FGameplayTagContainer ActionTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	float OriginalUnscaledCapsuleHalfHeight{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	float OriginalUnscaledCapsuleRadius{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	TWeakObjectPtr<UPrimitiveComponent> CurrentTargetPrimitive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	FVector FrontWallNormalLS{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	FVector FrontLedgeOffset{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	FRotator FrontLedgeRotationOffset{FRotator::ZeroRotator};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	FVector UpperLedgeNormalLS{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	FVector BackLedgeOffset{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GarAbility|Traversal|State", Transient)
	FVector BackFloorOffset{FVector::ZeroVector};

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Ability|Traversal")
	void Tick(const float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "GAR|Ability|Traversal")
	void CommitParameters(const FGameplayAbilitySpecHandle Handle, const FGarTraversalParameters& Parameters) const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Ability|Traversal")
	bool CanTraversal(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo, FGarTraversalParameters& OutParameters) const;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Ability|Traversal")
	bool TraceEnvironment(AGarCharacter* Character, FGarTraversalTraceResult& OutResult) const;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Ability|Traversal")
	void ChooseCandidate(const FGarTraversalChooserInputs& Input, FGarTraversalChooserOutput& Output, TArray<UAnimMontage*>& OutMontages) const;

	bool HasNonTraversalTag(const FHitResult& HitResult) const;

	void UpdateWarpTarget();

	void UpdateWarpTarget(const FGarTraversalParameters& Parameters);

	void UpdateWarpTargetFromComponent(const FGarTraversalParameters& Parameters);

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
									const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
									OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility, bool bWasCancelled) override;

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	static TMap<FGameplayAbilitySpecHandle, FGarTraversalParameters> ParameterMap;

	TWeakObjectPtr<class UGarAbilityTask_Tick> TickTask;

#if ENABLE_DRAW_DEBUG
	void DebugDrawWarpTarget();
#endif
};
