#pragma once

#include "Animation/Skeleton.h"
#include "GarSkeletonUtility.generated.h"

USTRUCT(BlueprintType)
struct GAREDITOR_API FGarBlendProfileEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	FName BoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	float BlendScale{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAR")
	uint8 bIncludeDescendants : 1 {false};
};

UCLASS()
class GAREDITOR_API UGarSkeletonUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Skeleton Utility")
	static void AddAnimationCurves(USkeleton* Skeleton, const TArray<FName>& CurveNames);

	UFUNCTION(BlueprintCallable, Category = "GAR|Skeleton Utility")
	static void AddOrReplaceSlot(USkeleton* Skeleton, FName SlotName, FName GroupName);

	UFUNCTION(BlueprintCallable, Category = "GAR|Skeleton Utility", Meta = (AutoCreateRefTerm = "SourceBoneName, TargetBoneName"))
	static void AddOrReplaceVirtualBone(USkeleton* Skeleton, const FName& SourceBoneName,
	                                    const FName& TargetBoneName, FName VirtualBoneName);

	UFUNCTION(BlueprintCallable, Category = "GAR|Skeleton Utility",
		Meta = (AutoCreateRefTerm = "BoneName, RelativeLocation, RelativeRotation"))
	static void AddOrReplaceSocket(USkeleton* Skeleton, FName SocketName, const FName& BoneName,
	                               const FVector& RelativeLocation, const FRotator& RelativeRotation);

	UFUNCTION(BlueprintCallable, Category = "GAR|Skeleton Utility")
	static void AddOrReplaceBlendProfile(USkeleton* Skeleton, FName BlendProfileName,
	                                     const TArray<FGarBlendProfileEntry>& Entries);

	UFUNCTION(BlueprintCallable, Category = "GAR|Skeleton Utility", Meta = (AutoCreateRefTerm = "BoneName"))
	static void SetBoneRetargetingMode(USkeleton* Skeleton, const FName& BoneName,
	                                   EBoneTranslationRetargetingMode::Type RetargetingMode, bool bIncludeDescendants);

	UFUNCTION(BlueprintPure, Category = "GAR|Skeleton Utility", Meta = (AutoCreateRefTerm = "BoneName"))
	static bool HasBone(USkeleton* Skeleton, const FName& BoneName);
};
