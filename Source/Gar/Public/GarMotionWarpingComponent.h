// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionWarpingComponent.h"
#include "GarMotionWarpingComponent.generated.h"

/** Parameter Structure for RPC.
 * Assuming that bFollowComponent is true.
 */
USTRUCT(BlueprintType)
struct GAR_API FGarMotionWarpingTargetSmall
{
	GENERATED_BODY()

	/** Unique name for this warp target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FName Name;

	/** When the warp target is created from a component this stores the location of the component at the time of creation, otherwise its the location supplied by the user */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FVector_NetQuantize10 Location;

	/** When the warp target is created from a component this stores the rotation of the component at the time of creation, otherwise its the rotation supplied by the user */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FRotator Rotation{ForceInitToZero};

	/** Optional component used to calculate the final target transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	TWeakObjectPtr<const USceneComponent> Component;
};

/**
 * 
 */
UCLASS()
class GAR_API UGarMotionWarpingComponent : public UMotionWarpingComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GAR|Motion Warping")
	void AddOrUpdateReplicatedWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector TargetLocation, FRotator TargetRotation);

	UFUNCTION(BlueprintCallable, Category = "GAR|Motion Warping")
	void AutonomousAddOrUpdateReplicatedWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector TargetLocation, FRotator TargetRotation);

	UFUNCTION(BlueprintCallable, Category = "GAR|Motion Warping")
	void AddOrUpdateReplicatedWarpTargetFromComponent(FName WarpTargetName, const USceneComponent* Component, FName BoneName, bool bFollowComponent);

	UFUNCTION(BlueprintCallable, Category = "GAR|Motion Warping")
	void AutonomousAddOrUpdateReplicatedWarpTargetFromComponent(FName WarpTargetName, const USceneComponent* Component, FName BoneName, bool bFollowComponent);

private:
	UFUNCTION(Server, Reliable)
	void ServerAddOrUpdateWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector_NetQuantize TargetLocation, FRotator TargetRotation);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAddOrUpdateWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector_NetQuantize TargetLocation, FRotator TargetRotation);

	UFUNCTION(Server, Reliable)
	void ServerAddOrUpdateWarpTargetFromComponent(FName WarpTargetName, const USceneComponent* Component, FName BoneName, bool bFollowComponent);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAddOrUpdateWarpTargetFromComponent(FName WarpTargetName, const USceneComponent* Component, FName BoneName, bool bFollowComponent);
};
