#pragma once

#include "Engine/DataAsset.h"
#include "GarFeetSettings.h"
#include "GarGeneralAnimationSettings.h"
#include "GarGroundedSettings.h"
#include "GarInAirSettings.h"
#include "GarRotateInPlaceSettings.h"
#include "GarTransitionsSettings.h"
#include "GarTurnInPlaceSettings.h"
#include "Settings/GarBoneNameTable.h"
#include "GarAnimationInstanceSettings.generated.h"

UCLASS(Blueprintable, BlueprintType)
class GAR_API UGarAnimationInstanceSettings : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarGeneralAnimationSettings General;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarGroundedSettings Grounded;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarInAirSettings InAir;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarFeetSettings Feet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarTransitionsSettings Transitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarRotateInPlaceSettings RotateInPlace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarGeneralTurnInPlaceSettings TurnInPlace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGarBoneNameTable> BoneNameTable;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
