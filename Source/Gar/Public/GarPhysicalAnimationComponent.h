// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "GarGameplayTags.h"
#include "GarPhysicalAnimationComponent.generated.h"

class UChooserTable;
class USkeletalBodySetup;
class AGarCharacter;
class UGarRagdollingSettings;
class UGarRagdollingAnimInstance;
class UGarPhysicalAnimationComponent;

USTRUCT(BlueprintType)
struct GAR_API FGarPAProfileChooserResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TArray<FName> ProfileNames;
};

USTRUCT(BlueprintType)
struct GAR_API FGarPACurveBoneMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FName CurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TArray<FName> BoneNames;
};

USTRUCT(BlueprintType)
struct GAR_API FGarPhysicalAnimationCurveValues
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	TMap<FName, float> Values;

	void Refresh(AGarCharacter* Character, const TArray<FGarPACurveBoneMapping>& Mapping);

	float GetLockedValue(const FName& BoneName, const TArray<FGarPACurveBoneMapping>& Mapping) const;
};

USTRUCT(BlueprintType)
struct GAR_API FGarRagdollingState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "deg"))
	float LyingDownYawAngleDelta{0.0f};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "N"))
	float PullForce{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	uint8 bGrounded : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	uint8 bFacingUpward : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "s"))
	float ElapsedTime{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "s"))
	float TimeAfterGrounded{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "s"))
	float TimeAfterGroundedAndStopped{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector PrevActorLocation{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	uint8 bFreezing : 1{false};

	// Speed of root bone
	// Exists solely for display in the editor. Useful when determining setting value.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "cm/s"))
	float RootBoneSpeed{0.0f};

	// Highest speed among all bodies
	// Exists solely for display in the editor. Useful when determining setting value.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "cm/s"))
	float MaxBoneSpeed{0.0f};

	// Highest angular speed among all bodies
	// Exists solely for display in the editor. Useful when determining setting value.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "deg"))
	float MaxBoneAngularSpeed{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bPreviousGrounded : 1{false};

	// SimProxy: Start() で記録した初速。初回物理 Tick で TopBone に注入する (CMC の PostNetReceivePhysicState 相当)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector InitialRagdollVelocity{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bPendingInitialVelocity : 1{false};

	// サーバー非ローカルキャラだけ、ラグドール中に一時的に AlwaysTickPoseAndRefreshBones へ切替する。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bOverrodeVisibilityBasedAnimTickOption : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 PreviousVisibilityBasedAnimTickOption{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UGarRagdollingAnimInstance* RagdollingAnimInstance{nullptr};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AGarCharacter* Character{nullptr};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UGarRagdollingSettings* Settings{nullptr};

	void Start(UGarRagdollingSettings* NewSettings, const UGarPhysicalAnimationComponent* PhysicalAnimation);

	void Tick(float DeltaTime, const UGarPhysicalAnimationComponent* PhysicalAnimation);

	void End(const UGarPhysicalAnimationComponent* PhysicalAnimation);

	FVector TraceGround();

	bool IsGroundedAndAged() const;
};

/**
 * PhysicalAnimationComponent for GAR Refactored
 */
UCLASS()
class GAR_API UGarPhysicalAnimationComponent : public UPhysicalAnimationComponent
{
	GENERATED_BODY()
	
protected:
	// The blend time Of physics blend Weight on activate physics body.
	// Not used when ragdolling activate. Ragdolling start with weight 1.0 immediately.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalAnimation|Settings", Meta = (ClampMin = 0, ForceUnits = "s"))
	float BlendTimeOfBlendWeightOnActivate{0.1f};

	// The blend time Of physics blend Weight on deactivate physics body.
	// Not used when ragdolling deactivate. In the case of a ragdoll, the weight becomes zero immediately.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalAnimation|Settings", Meta = (ClampMin = 0, ForceUnits = "s"))
	float BlendTimeOfBlendWeightOnDeactivate{0.1f};

	/**
	 * Chooser for determin physics animation profile and constraint profile
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalAnimation|Settings")
	TObjectPtr<UChooserTable> ProfileChooser;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PhysicalAnimation|Settings")
	TArray<FGarPACurveBoneMapping> CurveBoneMappings{
		{FName{TEXTVIEW("palock_arm_l")},  {{FName{TEXTVIEW("clavicle_l")}, FName{TEXTVIEW("upperarm_l")}, FName{TEXTVIEW("lowerarm_l")}}}},
		{FName{TEXTVIEW("palock_arm_r")},  {{FName{TEXTVIEW("clavicle_r")}, FName{TEXTVIEW("upperarm_r")}, FName{TEXTVIEW("lowerarm_r")}}}},
		{FName{TEXTVIEW("palock_hand_l")}, {{FName{TEXTVIEW("hand_l")}}}},
		{FName{TEXTVIEW("palock_hand_r")}, {{FName{TEXTVIEW("hand_r")}}}},
		{FName{TEXTVIEW("palock_leg_l")},  {{FName{TEXTVIEW("thigh_l")}, FName{TEXTVIEW("calf_l")}}}},
		{FName{TEXTVIEW("palock_leg_r")},  {{FName{TEXTVIEW("thigh_r")}, FName{TEXTVIEW("calf_r")}}}},
		{FName{TEXTVIEW("palock_foot_l")}, {{FName{TEXTVIEW("foot_l")}, FName{TEXTVIEW("ball_l")}}}},
		{FName{TEXTVIEW("palock_foot_r")}, {{FName{TEXTVIEW("foot_r")}, FName{TEXTVIEW("ball_r")}}}},
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PhysicalAnimation|Settings")
	FName TopBoneName{TEXTVIEW("pelvis")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalAnimation|Settings")
	TMap<FGameplayTag, TObjectPtr<UGarRagdollingSettings>> RagdollingSettingsMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalAnimation|State", Transient)
	TEnumAsByte<ECollisionChannel> PrevCollisionObjectType{ECC_Pawn};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalAnimation|State", Transient)
	TEnumAsByte<ECollisionEnabled::Type> PrevCollisionEnabled{ECollisionEnabled::QueryOnly};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "PhysicalAnimation|State", Transient)
	TEnumAsByte<EPhysicsTransformUpdateMode::Type> OriginalUpdateMode;

	UPROPERTY(VisibleAnywhere, Category = "PhysicalAnimation|State", Transient)
	TArray<FName> CurrentProfileNames;

	UPROPERTY(VisibleAnywhere, Category = "PhysicalAnimation|State", Transient)
	FGameplayTagContainer CurrentGameplayTags;

	UPROPERTY(VisibleAnywhere, Category = "PhysicalAnimation|State", Transient)
	FGameplayTagContainer PreviousGameplayTags;

	UPROPERTY(VisibleAnywhere, Category = "PhysicalAnimation|State", Transient)
	FGarPhysicalAnimationCurveValues CurveValues;

	UPROPERTY(VisibleAnywhere, Category = "PhysicalAnimation|State", Transient)
	uint8 bActive : 1{false};

	UPROPERTY(VisibleAnywhere, Category = "PhysicalAnimation|State", Transient)
	uint8 bRagdolling : 1{false};

	UPROPERTY(VisibleAnywhere, Category = "PhysicalAnimation|State", Transient)
	FGameplayTag CurrentRagdolling;

	UPROPERTY(VisibleAnywhere, Category = "PhysicalAnimation|State", Transient)
	FGarRagdollingState RagdollingState;

protected:
	virtual void OnRegister() override;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Ability|Traversal")
	void ChooseProfile(FGarPAProfileChooserResult& OutResult) const;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation);

	FORCEINLINE const TArray<FGarPACurveBoneMapping>& GetCurveBoneMappings() const
	{
		return CurveBoneMappings;
	}

	FORCEINLINE const FName& GetTopBoneName() const
	{
		return TopBoneName;
	}

	UFUNCTION(BlueprintPure, Category = "GAR|PhysicalAnimation")
	bool HasRagdollingSettings(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintPure, Category = "GAR|PhysicalAnimation")
	bool IsRagdolling() const;

	// テレポート後にシミュレーション中の物理ボディと PAC の TargetActors を
	// 現在のアニメーションポーズ位置へ即座にスナップする。
	void TeleportPhysics();

	UFUNCTION(BlueprintPure, Category = "GAR|PhysicalAnimation")
	bool IsRagdollingAndGroundedAndAged() const;

	UFUNCTION(BlueprintPure, Category = "GAR|PhysicalAnimation")
	bool IsRagdollingFacingUpward() const;

	UFUNCTION(BlueprintPure, Category = "GAR|PhysicalAnimation")
	bool IsBoneUnderSimulation(const FName& BoneName) const;

	UFUNCTION(BlueprintPure, Category = "GAR|PhysicalAnimation")
	const FGarRagdollingState& GetRagdollingState() const
	{
		return RagdollingState;
	}

private:
	void ClearGameplayTags();

	void RefreshBodyState(float DelaTime);

	bool IsProfileExist(const FName& ProfileName) const;

	bool HasAnyProfile(const class USkeletalBodySetup* BodySetup) const;

	bool NeedsProfileChange();

	void SelectProfile();
};
