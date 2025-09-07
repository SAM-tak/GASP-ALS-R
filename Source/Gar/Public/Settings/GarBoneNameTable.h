#pragma once

#include "Engine/DataAsset.h"
#include "GarBoneNameTable.generated.h"

UCLASS(Blueprintable, BlueprintType)
class GAR_API UGarBoneNameTable : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName RootBoneName{TEXTVIEW("root")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName PelvisBoneName{TEXTVIEW("pelvis")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName HeadBoneName{TEXTVIEW("head")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName SpineTopBoneName{TEXTVIEW("spine_03")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName FootLeftBoneName{TEXTVIEW("foot_l")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName FootRightBoneName{TEXTVIEW("foot_r")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName HandLeftGunVirtualBoneName{TEXTVIEW("VB hand_l_to_ik_hand_gun")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName HandRightGunVirtualBoneName{TEXTVIEW("VB hand_r_to_ik_hand_gun")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName FootLeftVirtualBoneName{TEXTVIEW("VB foot_l")};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName FootRightVirtualBoneName{TEXTVIEW("VB foot_r")};
};
