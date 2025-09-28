#pragma once

#include "MoverComponent.h"
#include "GarGameplayTags.h"
#include "GarLayeredMove_TurnTo.generated.h"

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
