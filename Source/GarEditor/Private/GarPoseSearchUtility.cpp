#include "GarPoseSearchUtility.h"

#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarPoseSearchUtility)

bool UGarPoseSearchUtility::SetDatabaseAnimationAssets(UPoseSearchDatabase* Database,
	const TArray<FPoseSearchDatabaseAnimationAsset>& Assets)
{
	if (!Database)
	{
		return false;
	}

	FArrayProperty* Prop = FindFProperty<FArrayProperty>(
		UPoseSearchDatabase::StaticClass(), TEXT("DatabaseAnimationAssets"));

	if (!Prop)
	{
		return false;
	}

	FScriptArrayHelper Helper(Prop, Prop->ContainerPtrToValuePtr<void>(Database));
	Helper.EmptyValues();

	const FStructProperty* InnerStructProp = CastField<FStructProperty>(Prop->Inner);
	if (!InnerStructProp)
	{
		return false;
	}

	for (const FPoseSearchDatabaseAnimationAsset& Entry : Assets)
	{
		const int32 NewIndex = Helper.AddValue();
		InnerStructProp->Struct->CopyScriptStruct(Helper.GetRawPtr(NewIndex), &Entry);
	}

	Database->MarkPackageDirty();
	return true;
}

bool UGarPoseSearchUtility::GetDatabaseAnimationAssets(const UPoseSearchDatabase* Database,
	TArray<FPoseSearchDatabaseAnimationAsset>& OutAssets)
{
	OutAssets.Reset();

	if (!Database)
	{
		return false;
	}

	FArrayProperty* Prop = FindFProperty<FArrayProperty>(
		UPoseSearchDatabase::StaticClass(), TEXT("DatabaseAnimationAssets"));

	if (!Prop)
	{
		return false;
	}

	// ContainerPtrToValuePtr on a const object: we need a non-const ptr for the
	// helper but we won't mutate the array, so the cast is safe here.
	void* ContainerPtr = const_cast<void*>(
		static_cast<const void*>(
			Prop->ContainerPtrToValuePtr<void>(Database)));

	FScriptArrayHelper Helper(Prop, ContainerPtr);

	const FStructProperty* InnerStructProp = CastField<FStructProperty>(Prop->Inner);
	if (!InnerStructProp)
	{
		return false;
	}

	OutAssets.SetNum(Helper.Num());
	for (int32 i = 0; i < Helper.Num(); ++i)
	{
		InnerStructProp->Struct->CopyScriptStruct(&OutAssets[i], Helper.GetRawPtr(i));
	}

	return true;
}
