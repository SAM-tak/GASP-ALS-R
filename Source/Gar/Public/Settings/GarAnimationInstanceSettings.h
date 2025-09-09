#pragma once

#include "Engine/DataAsset.h"
#include "GarFeetSettings.h"
#include "Settings/GarBoneNameTable.h"
#include "GarAnimationInstanceSettings.generated.h"

UCLASS(Blueprintable, BlueprintType)
class GAR_API UGarAnimationInstanceSettings : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float MovingSmoothSpeedThreshold{150.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGarFeetSettings Feet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGarBoneNameTable> BoneNameTable;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
