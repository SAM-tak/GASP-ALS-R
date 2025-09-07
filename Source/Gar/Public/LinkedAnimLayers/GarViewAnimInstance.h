#pragma once

#include "GarLinkedAnimationInstance.h"
#include "GarViewAnimInstance.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarLookState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bReinitializationRequired : 1{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float WorldYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float YawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -90, ClampMax = 90, ForceUnits = "deg"))
	float PitchAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 1))
	float YawForwardAmount{0.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 0.5))
	float YawLeftAmount{0.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0.5, ClampMax = 1))
	float YawRightAmount{0.5f};
};

USTRUCT(BlueprintType)
struct GAR_API FGarSpineRotationState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bSpineRotationAllowed : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = 0, ClampMax = 1))
	float SpineAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float InitialYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float TargetYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float CurrentYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR", Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float YawAngle{0.0f};
};

// View Linked Anim Layer
// Tag : "View"
UCLASS(Abstract, AutoExpandCategories = ("GAR|Settings"))
class GAR_API UGarViewAnimInstance : public UGarLinkedAnimationInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR|Settings", Meta = (ClampMin = 0))
	float LookTowardsCameraRotationInterpolationSpeed{8.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR|Settings", Meta = (ClampMin = 0))
	float LookTowardsInputYawAngleInterpolationSpeed{8.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient)
	FRotator Rotation{ForceInit}; // internal, no AnimBP use, used in GarAnimationInstance(ex : RefreshMovementDirection)

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient, Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float YawAngle{0.0f}; // internal, no AnimBP use, used in GarAnimationInstance(ex : RefreshTurnInPlace)

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient, Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float YawSpeed{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient, Meta = (ClampMin = -90, ClampMax = 90, ForceUnits = "deg"))
	float PitchAngle{0.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient, Meta = (ClampMin = 0, ClampMax = 1))
	float PitchAmount{0.5f}; // using in overlay

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient, Meta = (ClampMin = 0, ClampMax = 1))
	float LookAmount{1.0f}; // using

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient)
	FGarSpineRotationState SpineRotation; // using SpineRotation.YawAngle in UGarAnimationInstance::GetControlRigInput 

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAR|State", Transient)
	FGarLookState Look; // using YawAngle PitchAngle YawLeftAmount YawRightAmount YawForwardAmount in AnimBP

protected:
	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe))
	void ReinitializeLook();

	UFUNCTION(BlueprintCallable, Category = "GAR|Animation Instance", Meta = (BlueprintProtected, BlueprintThreadSafe))
	void RefreshLook();

public:
	//virtual void NativeUpdateAnimation(float DeltaTime) override;

	//virtual void NativeThreadSafeUpdateAnimation(float DeltaTime) override;

	void RefreshOnGameThread(float DeltaTime);

	void Refresh(float DeltaTime);
};
