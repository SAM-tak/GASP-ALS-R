#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "GarPoseSearchUtility.generated.h"

/**
 * Editor utility for manipulating PoseSearchDatabase properties that are
 * inaccessible via normal Blueprint/Python API (private UPROPERTY).
 *
 * Uses UE's reflection system (FindFProperty) to bypass C++ access control
 * while still respecting the UPROPERTY metadata.
 */
UCLASS()
class GAREDITOR_API UGarPoseSearchUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Replaces all DatabaseAnimationAssets entries in the given database.
	 * The property is private in UPoseSearchDatabase but registered with
	 * the reflection system, so we can access it via FindFProperty.
	 *
	 * Call MarkPackageDirty() on the database before saving after this.
	 *
	 * @param Database  The PoseSearchDatabase asset to modify.
	 * @param Assets    New array to assign. Replaces existing contents entirely.
	 * @return true on success, false if the property could not be found.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAR|Pose Search Utility",
		meta = (DevelopmentOnly))
	static bool SetDatabaseAnimationAssets(UPoseSearchDatabase* Database,
		const TArray<FPoseSearchDatabaseAnimationAsset>& Assets);

	/**
	 * Returns the current DatabaseAnimationAssets array from the given database.
	 *
	 * @param Database  The PoseSearchDatabase asset to read from.
	 * @param OutAssets Populated with the current array contents.
	 * @return true on success, false if the property could not be found.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAR|Pose Search Utility",
		meta = (DevelopmentOnly))
	static bool GetDatabaseAnimationAssets(const UPoseSearchDatabase* Database,
		TArray<FPoseSearchDatabaseAnimationAsset>& OutAssets);
};
