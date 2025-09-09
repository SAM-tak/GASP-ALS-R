#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GarMath.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarSpringFloatState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	float Velocity{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	float PreviousTarget{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bStateValid : 1 {false};

	void Reset();
};

USTRUCT(BlueprintType)
struct GAR_API FGarSpringVectorState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector Velocity{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FVector PreviousTarget{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bStateValid : 1 {false};

	void Reset();
};

UCLASS()
class GAR_API UGarMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr auto CounterClockwiseRotationAngleThreshold{5.0f};

public:
	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (ReturnDisplayName = "Value"))
	static float Clamp01(float Value);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (ReturnDisplayName = "Value"))
	static float LerpClamped(float From, float To, float Alpha);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (ReturnDisplayName = "Angle"))
	static float LerpAngle(float From, float To, float Alpha);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (AutoCreateRefTerm = "From, To", ReturnDisplayName = "Rotator"))
	static FRotator LerpRotator(const FRotator& From, const FRotator& To, float Alpha);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (ReturnDisplayName = "Interpolation Ammount"))
	static float Damp(float DeltaTime, float Smoothing);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (ReturnDisplayName = "Interpolation Ammount"))
	static float ExponentialDecay(float DeltaTime, float Lambda);

	template <typename ValueType>
	static ValueType Damp(const ValueType& Current, const ValueType& Target, float DeltaTime, float Smoothing);

	template <typename ValueType>
	static ValueType ExponentialDecay(const ValueType& Current, const ValueType& Target, float DeltaTime, float Lambda);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (ReturnDisplayName = "Angle"))
	static float DampAngle(float Current, float Target, float DeltaTime, float Smoothing);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (ReturnDisplayName = "Angle"))
	static float ExponentialDecayAngle(float Current, float Target, float DeltaTime, float Lambda);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (AutoCreateRefTerm = "Current, Target", ReturnDisplayName = "Rotator"))
	static FRotator DampRotator(const FRotator& Current, const FRotator& Target, float DeltaTime, float Smoothing);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (AutoCreateRefTerm = "Current, Target", ReturnDisplayName = "Rotator"))
	static FRotator ExponentialDecayRotator(const FRotator& Current, const FRotator& Target, float DeltaTime, float Lambda);

	UFUNCTION(BlueprintPure, Category = "GAR|Math", Meta = (ReturnDisplayName = "Angle"))
	static float InterpolateAngleConstant(float Current, float Target, float DeltaTime, float InterpolationSpeed);

	template <typename ValueType, typename StateType>
	static ValueType SpringDamp(const ValueType& Current, const ValueType& Target, StateType& SpringState,
	                            float DeltaTime, float Frequency, float DampingRatio, float TargetVelocityAmount = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "GAR|Math", Meta = (ReturnDisplayName = "Value"))
	static float SpringDampFloat(float Current, float Target, UPARAM(ref) FGarSpringFloatState& SpringState,
	                             float DeltaTime, float Frequency, float DampingRatio, float TargetVelocityAmount = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "GAR|Math", Meta = (AutoCreateRefTerm = "Current, Target", ReturnDisplayName = "Vector"))
	static FVector SpringDampVector(const FVector& Current, const FVector& Target, UPARAM(ref) FGarSpringVectorState& SpringState,
	                                float DeltaTime, float Frequency, float DampingRatio, float TargetVelocityAmount = 1.0f);

	// Remaps the angle from the [175, 180] range to [-185, -180]. Used to
	// make the character rotate counterclockwise during a 180 degree turn.
	template <typename ValueType> requires std::is_floating_point_v<ValueType>
	static constexpr float RemapAngleForCounterClockwiseRotation(ValueType Angle);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (AutoCreateRefTerm = "Vector", ReturnDisplayName = "Vector"))
	static FVector ClampMagnitude01(const FVector& Vector);

	static FVector3f ClampMagnitude01(const FVector3f& Vector);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", DisplayName = "Clamp Magnitude 01 2D",
		Meta = (AutoCreateRefTerm = "Vector", ReturnDisplayName = "Vector"))
	static FVector2D ClampMagnitude012D(const FVector2D& Vector);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (ReturnDisplayName = "Direction"))
	static FVector2D RadianToDirection(float Radian);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (ReturnDisplayName = "Direction"))
	static FVector RadianToDirectionXY(float Radian);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (ReturnDisplayName = "Direction"))
	static FVector2D AngleToDirection(float Angle);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (ReturnDisplayName = "Direction"))
	static FVector AngleToDirectionXY(float Angle);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (AutoCreateRefTerm = "Direction", ReturnDisplayName = "Angle"))
	static double DirectionToAngle(const FVector2D& PoleDirection);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (AutoCreateRefTerm = "Direction", ReturnDisplayName = "Angle"))
	static double DirectionToAngleXY(const FVector& PoleDirection);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (AutoCreateRefTerm = "Vector", ReturnDisplayName = "Vector"))
	static FVector PerpendicularClockwiseXY(const FVector& Vector);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", Meta = (AutoCreateRefTerm = "Vector", ReturnDisplayName = "Vector"))
	static FVector PerpendicularCounterClockwiseXY(const FVector& Vector);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector", DisplayName = "Angle Between (Skip Normalization)",
		Meta = (AutoCreateRefTerm = "From, To", ReturnDisplayName = "Angle"))
	static double AngleBetweenSkipNormalization(const FVector& From, const FVector& To);

	UFUNCTION(BlueprintPure, Category = "GAR|Math|Vector",
		DisplayName = "Slerp (Skip Normalization)", Meta = (AutoCreateRefTerm = "From, To", ReturnDisplayName = "Direction"))
	static FVector SlerpSkipNormalization(const FVector& From, const FVector& To, float Alpha);

	// Calculates the projection location and direction of the perpendicular to AC through B.
	UFUNCTION(BlueprintCallable, Category = "GAR|Math|Input",
		Meta = (AutoCreateRefTerm = "ALocation, BLocation, CLocation", ExpandBoolAsExecs = "ReturnValue"))
	static bool TryCalculatePoleVector(const FVector& ALocation, const FVector& BLocation, const FVector& CLocation,
	                                   FVector& ProjectionLocation, FVector& PoleDirection);
};

template <typename ValueType> requires std::is_floating_point_v<ValueType>
constexpr float UGarMath::RemapAngleForCounterClockwiseRotation(const ValueType Angle)
{
	if (Angle > 180.0f - CounterClockwiseRotationAngleThreshold)
	{
		return Angle - 360.0f;
	}

	return Angle;
}

inline void FGarSpringFloatState::Reset()
{
	Velocity = 0.f;
	PreviousTarget = 0.f;
	bStateValid = false;
}

inline void FGarSpringVectorState::Reset()
{
	Velocity = FVector::ZeroVector;
	PreviousTarget = FVector::ZeroVector;
	bStateValid = false;
}

inline float UGarMath::Clamp01(const float Value)
{
	return Value <= 0.0f
		       ? 0.0f
		       : Value >= 1.0f
		       ? 1.0f
		       : Value;
}

inline float UGarMath::LerpClamped(const float From, const float To, const float Alpha)
{
	return From + (To - From) * Clamp01(Alpha);
}

inline float UGarMath::LerpAngle(const float From, const float To, const float Alpha)
{
	auto Delta{FMath::UnwindDegrees(To - From)};
	Delta = RemapAngleForCounterClockwiseRotation(Delta);

	return FMath::UnwindDegrees(From + Delta * Alpha);
}

inline FRotator UGarMath::LerpRotator(const FRotator& From, const FRotator& To, const float Alpha)
{
	auto Result{To - From};
	Result.Normalize();

	Result.Pitch = RemapAngleForCounterClockwiseRotation(Result.Pitch);
	Result.Yaw = RemapAngleForCounterClockwiseRotation(Result.Yaw);
	Result.Roll = RemapAngleForCounterClockwiseRotation(Result.Roll);

	Result *= Alpha;
	Result += From;
	Result.Normalize();

	return Result;
}

inline float UGarMath::Damp(const float DeltaTime, const float Smoothing)
{
	// https://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/

	return 1.0f - FMath::Pow(Smoothing, DeltaTime);
}

inline float UGarMath::ExponentialDecay(const float DeltaTime, const float Lambda)
{
	// https://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/

	return 1.0f - FMath::InvExpApprox(Lambda * DeltaTime);
}

template <typename ValueType>
ValueType UGarMath::Damp(const ValueType& Current, const ValueType& Target, const float DeltaTime, const float Smoothing)
{
	return Smoothing > 0.0f
		       ? FMath::Lerp(Current, Target, Damp(DeltaTime, Smoothing))
		       : Target;
}

template <typename ValueType>
ValueType UGarMath::ExponentialDecay(const ValueType& Current, const ValueType& Target, const float DeltaTime, const float Lambda)
{
	return Lambda > 0.0f
		       ? FMath::Lerp(Current, Target, ExponentialDecay(DeltaTime, Lambda))
		       : Target;
}

template <>
inline FRotator UGarMath::Damp(const FRotator& Current, const FRotator& Target, const float DeltaTime, const float Smoothing)
{
	return Smoothing > 0.0f
		       ? LerpRotator(Current, Target, Damp(DeltaTime, Smoothing))
		       : Target;
}

template <>
inline FRotator UGarMath::ExponentialDecay(const FRotator& Current, const FRotator& Target, const float DeltaTime, const float Lambda)
{
	return Lambda > 0.0f
		       ? LerpRotator(Current, Target, ExponentialDecay(DeltaTime, Lambda))
		       : Target;
}

inline float UGarMath::DampAngle(const float Current, const float Target, const float DeltaTime, const float Smoothing)
{
	return Smoothing > 0.0f
		       ? LerpAngle(Current, Target, Damp(DeltaTime, Smoothing))
		       : Target;
}

inline float UGarMath::ExponentialDecayAngle(const float Current, const float Target, const float DeltaTime, const float Lambda)
{
	return Lambda > 0.0f
		       ? LerpAngle(Current, Target, ExponentialDecay(DeltaTime, Lambda))
		       : Target;
}

inline FRotator UGarMath::DampRotator(const FRotator& Current, const FRotator& Target, const float DeltaTime, const float Smoothing)
{
	return Damp(Current, Target, DeltaTime, Smoothing);
}

inline FRotator UGarMath::ExponentialDecayRotator(const FRotator& Current, const FRotator& Target,
                                                  const float DeltaTime, const float Lambda)
{
	return ExponentialDecay(Current, Target, DeltaTime, Lambda);
}

inline float UGarMath::InterpolateAngleConstant(const float Current, const float Target,
                                                const float DeltaTime, const float InterpolationSpeed)
{
	if (InterpolationSpeed <= 0.0f || Current == Target)
	{
		return Target;
	}

	auto Delta{FMath::UnwindDegrees(Target - Current)};
	Delta = RemapAngleForCounterClockwiseRotation(Delta);

	const auto Alpha{InterpolationSpeed * DeltaTime};

	return FMath::UnwindDegrees(Current + FMath::Clamp(Delta, -Alpha, Alpha));
}

template <typename ValueType, typename StateType>
ValueType UGarMath::SpringDamp(const ValueType& Current, const ValueType& Target, StateType& SpringState, const float DeltaTime,
                               const float Frequency, const float DampingRatio, const float TargetVelocityAmount)
{
	if (DeltaTime <= UE_SMALL_NUMBER)
	{
		return Current;
	}

	if (!SpringState.bStateValid)
	{
		SpringState.Velocity = ValueType{0.0f};
		SpringState.PreviousTarget = Target;
		SpringState.bStateValid = true;

		return Target;
	}

	ValueType Result{Current};
	FMath::SpringDamper(Result, SpringState.Velocity, Target,
	                    (Target - SpringState.PreviousTarget) * Clamp01(TargetVelocityAmount) / DeltaTime,
	                    DeltaTime, Frequency, DampingRatio);

	SpringState.PreviousTarget = Target;

	return Result;
}

inline FVector UGarMath::ClampMagnitude01(const FVector& Vector)
{
	const auto MagnitudeSquared{Vector.SizeSquared()};

	if (MagnitudeSquared <= 1.0f)
	{
		return Vector;
	}

	const auto Scale{FMath::InvSqrt(MagnitudeSquared)};

	return {Vector.X * Scale, Vector.Y * Scale, Vector.Z * Scale};
}

inline FVector3f UGarMath::ClampMagnitude01(const FVector3f& Vector)
{
	const auto MagnitudeSquared{Vector.SizeSquared()};

	if (MagnitudeSquared <= 1.0f)
	{
		return Vector;
	}

	const auto Scale{FMath::InvSqrt(MagnitudeSquared)};

	return {Vector.X * Scale, Vector.Y * Scale, Vector.Z * Scale};
}

inline FVector2D UGarMath::ClampMagnitude012D(const FVector2D& Vector)
{
	const auto MagnitudeSquared{Vector.SizeSquared()};

	if (MagnitudeSquared <= 1.0f)
	{
		return Vector;
	}

	const auto Scale{FMath::InvSqrt(MagnitudeSquared)};

	return {Vector.X * Scale, Vector.Y * Scale};
}

inline FVector2D UGarMath::RadianToDirection(const float Radian)
{
	float Sin, Cos;
	FMath::SinCos(&Sin, &Cos, Radian);

	return {Cos, Sin};
}

inline FVector UGarMath::RadianToDirectionXY(const float Radian)
{
	float Sin, Cos;
	FMath::SinCos(&Sin, &Cos, Radian);

	return {Cos, Sin, 0.0f};
}

inline FVector2D UGarMath::AngleToDirection(const float Angle)
{
	return RadianToDirection(FMath::DegreesToRadians(Angle));
}

inline FVector UGarMath::AngleToDirectionXY(const float Angle)
{
	return RadianToDirectionXY(FMath::DegreesToRadians(Angle));
}

inline double UGarMath::DirectionToAngle(const FVector2D& PoleDirection)
{
	return FMath::RadiansToDegrees(FMath::Atan2(PoleDirection.Y, PoleDirection.X));
}

inline double UGarMath::DirectionToAngleXY(const FVector& PoleDirection)
{
	return FMath::RadiansToDegrees(FMath::Atan2(PoleDirection.Y, PoleDirection.X));
}

inline FVector UGarMath::PerpendicularClockwiseXY(const FVector& Vector)
{
	return {Vector.Y, -Vector.X, Vector.Z};
}

inline FVector UGarMath::PerpendicularCounterClockwiseXY(const FVector& Vector)
{
	return {-Vector.Y, Vector.X, Vector.Z};
}

inline double UGarMath::AngleBetweenSkipNormalization(const FVector& From, const FVector& To)
{
	return FMath::RadiansToDegrees(FMath::Acos(From | To));
}
