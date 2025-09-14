#pragma once

#include "Components/PawnComponent.h"
#include "Core/CameraVariableTableFwd.h"
#include "GarCameraGameplayTags.h"
#include "GarGameplayCameraStateComponent.generated.h"

class UGarGameplayCameraStateSettings;
class AGarCharacter;
class UGameplayCameraComponentBase;
class UFloatCameraVariable;
class UVector3dCameraVariable;
class URotator3dCameraVariable;

UCLASS(Blueprintable, ClassGroup=Camera, meta=(BlueprintSpawnableComponent))
class GARCAMERA_API UGarGameplayCameraStateComponent : public UPawnComponent
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGarGameplayCameraStateSettings> Settings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "State")
	TWeakObjectPtr<AGarCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UGameplayCameraComponentBase> GameplayCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FVector CameraLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FRotator CameraRotation;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (ClampMin = 0, ForceUnits = "cm"))
	float FocalLength{500.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bIsFocusPawn : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTag Perspective{GarCameraPerspectiveTags::ThirdPerson};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "State", Transient)
	FGameplayTag DesiredPerspective{GarCameraPerspectiveTags::ThirdPerson};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Replicated)
	FGameplayTag ConfirmedDesiredPerspective{GarCameraPerspectiveTags::ThirdPerson};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTag PreviousConfirmedDesiredPerspective{GarCameraPerspectiveTags::ThirdPerson};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (ClampMin = 0, ForceUnits = "s"))
	float PerspectiveChangeBlockTime{0.f};
		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Replicated)
	FGameplayTag ShoulderMode{GarCameraShoulderModeTags::Right};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTag PreviousShoulderMode{GarCameraShoulderModeTags::Right};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	float FirstPersonFactor{0.f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	float TraceSphreRadius{10.f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	float TanHalfVfov{0.57f}; // ≒tan(60°/2)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FVector SightLocationOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FQuat SightRotationOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bIsSightOffsetValid : 1{false};

protected:

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Gameplay Camera State")
	void OnPossessed(AController* NewController);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|Gameplay Camera State")
	void OnUnPossessed(AController* PreviousController);

protected:

	FCameraVariableID FirstPersonFactorVariableId;
	FCameraVariableID TraceSphreRadiusVariableId;
	FCameraVariableID FirstPersonLocationVariableId;
	FCameraVariableID EyeLocationVariableId;
	FCameraVariableID ADSLocationVariableId;
	FCameraVariableID ADSRotationVariableId;
	FCameraVariableID BoomOffsetVariableId;
	FCameraVariableID CenterShoulderOffsetVariableId;
	FCameraVariableID LeftShoulderOffsetVariableId;
	FCameraVariableID RightShoulderOffsetVariableId;

	FVector3d BoomOffset{FVector3d::ZeroVector};
	FVector3d CenterShoulderOffset{FVector3d::ZeroVector};
	FVector3d LeftShoulderOffset{FVector3d::ZeroVector};
	FVector3d RightShoulderOffset{FVector3d::ZeroVector};

	virtual void OnRegister() override;

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void Activate(bool bReset = false) override;

	virtual void Deactivate() override;

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Gameplay Camera State")
	void InitializeByCameraVariables(UVector3dCameraVariable* BoomOffsetVariable,
		UVector3dCameraVariable* CenterShoulderOffsetVariable,
		UVector3dCameraVariable* LeftShoulderOffsetVariable,
		UVector3dCameraVariable* RightShoulderOffsetVariable);

	UFUNCTION(BlueprintCallable, Category = "GAR|Gameplay Camera State")
	void BindCameraVariables(UFloatCameraVariable* FirstPersonFactorVariable,
		UFloatCameraVariable* TraceSphreRadiusVariable,
		UVector3dCameraVariable* FirstPersonLocationVariable,
		UVector3dCameraVariable* EyeLocationVariable,
		UVector3dCameraVariable* ADSLocationVariable,
		URotator3dCameraVariable* ADSRotationVariable);

	UFUNCTION(BlueprintPure, Category = "GAR|Gameplay Camera State", Meta = (ReturnDisplayName = "Camera Location"))
	FVector GetFirstPersonCameraLocation() const;

	UFUNCTION(BlueprintPure, Category = "GAR|Gameplay Camera State", Meta = (ReturnDisplayName = "Camera Location"))
	FVector GetEyeCameraLocation() const;

	UFUNCTION(BlueprintPure, Category = "GAR|Gameplay Camera State", Meta = (ReturnDisplayName = "Trace Start"))
	FVector GetThirdPersonTraceStartLocation() const;

	UFUNCTION(BlueprintPure, Category = "GAR|Gameplay Camera State", Meta = (ReturnDisplayName = "Trace Start"))
	FVector GetFirstPersonTraceStartLocation() const;

	UFUNCTION(BlueprintPure, Category = "GAR|Gameplay Camera State", Meta = (ReturnDisplayName = "Focus Location"))
	FVector GetCurrentFocusLocation() const;

	float GetTanHalfVfov() const;

	float GetFirstPersonFactor() const;

	// Desired View Mode

public:
	const FGameplayTag& GetDesiredViewMode() const;

	void SetDesiredViewMode(const FGameplayTag& NewDesiredViewMode);

	const FGameplayTag& GetConfirmedDesiredViewMode() const;

protected:
	void SetConfirmedDesiredViewMode(const FGameplayTag& NewDesiredViewMode);

private:
	UFUNCTION(Server, Unreliable)
	void ServerSetConfirmedDesiredViewMode(const FGameplayTag& NewDesiredViewMode);

	// ShoulderMode

public:
	const FGameplayTag& GetShoulderMode() const;

	UFUNCTION(BlueprintCallable, Category = "GAR|Gameplay Camera State")
	void SetShoulderMode(const FGameplayTag& NewShoulderMode);

	UFUNCTION(BlueprintCallable, Category = "GAR|Gameplay Camera State")
	void ToggleShoulder();

	UFUNCTION(BlueprintCallable, Category = "GAR|Gameplay Camera State")
	void UpdateState(float DeltaTime);

private:
	UFUNCTION(Server, Unreliable)
	void ServerSetShoulderMode(const FGameplayTag& NewShoulderMode);

private:
	void UpdateViewMode();

	void UpdateFocalLength();

#if !UE_BUILD_SHIPPING
	// Debug

public:
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation);

private:
	static void DisplayDebugHeader(const UCanvas* Canvas, const FText& HeaderText, const FLinearColor& HeaderColor,
		float Scale, float HorizontalLocation, float& VerticalLocation);

	void DisplayDebugState(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugTraces(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;
#endif // !UE_BUILD_SHIPPING
};

inline FVector UGarGameplayCameraStateComponent::GetCurrentFocusLocation() const
{
	return CameraLocation + CameraRotation.Vector() * FocalLength;
}

inline const FGameplayTag& UGarGameplayCameraStateComponent::GetDesiredViewMode() const
{
	return DesiredPerspective;
}

inline const FGameplayTag& UGarGameplayCameraStateComponent::GetConfirmedDesiredViewMode() const
{
	return ConfirmedDesiredPerspective;
}

inline const FGameplayTag& UGarGameplayCameraStateComponent::GetShoulderMode() const
{
	return ShoulderMode;
}

inline float UGarGameplayCameraStateComponent::GetTanHalfVfov() const
{
	return TanHalfVfov;
}

inline float UGarGameplayCameraStateComponent::GetFirstPersonFactor() const
{
	return FirstPersonFactor;
}
