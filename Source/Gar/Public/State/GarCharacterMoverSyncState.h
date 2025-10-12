#pragma once

#include "MoverDataModelTypes.h"
#include "GarGameplayTags.h"
#include "GarCharacterMoverSyncState.generated.h"

USTRUCT(BlueprintType)
struct GAR_API FGarCharacterMoverSyncState : public FMoverDataStructBase
{
	GENERATED_BODY()

public:
	FGarCharacterMoverSyncState() : RotationMode(GarRotationModeTags::ViewDirection), Stance(GarStanceTags::Standing), Gait(GarGaitTags::Running)
	{
	}

	bool operator==(const FGarCharacterMoverSyncState& Other) const
	{
		return RotationMode == Other.RotationMode && Stance == Other.Stance && Gait == Other.Gait;
	}

	bool operator!=(const FGarCharacterMoverSyncState& Other) const { return !operator==(Other); }

	// @return newly allocated copy of this FCharacterDefaultInputs. Must be overridden by child classes
	virtual FMoverDataStructBase* Clone() const override
	{
		// TODO: ensure that this memory allocation jives with deletion method
		return new FGarCharacterMoverSyncState(*this);
	}
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override
	{
		Super::NetSerialize(Ar, Map, bOutSuccess);
		Ar << RotationMode;
		Ar << Stance;
		Ar << Gait;
		bOutSuccess = true;
		return true;
	}
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	virtual void ToString(FAnsiStringBuilderBase& Out) const override
	{
		Super::ToString(Out);
		Out.Appendf("RotationMode: %s\tStance: %s\tGait: %s\n",
			TCHAR_TO_ANSI(*RotationMode.ToString()), TCHAR_TO_ANSI(*Stance.ToString()), TCHAR_TO_ANSI(*Gait.ToString()));
	}
	//virtual void AddReferencedObjects(FReferenceCollector& Collector) override { Super::AddReferencedObjects(Collector); }
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override
	{
		auto& TypedAuthority = static_cast<const FGarCharacterMoverSyncState&>(AuthorityState);
		return *this != TypedAuthority;
	}
	virtual void Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct) override
	{
		Super::Interpolate(From, To, Pct);
		auto& TypedFrom = static_cast<const FGarCharacterMoverSyncState&>(From);
		auto& TypedTo = static_cast<const FGarCharacterMoverSyncState&>(To);

		auto ClosestInputs = Pct < 0.5f ? &TypedFrom : &TypedTo;
		if(ClosestInputs->RotationMode.IsValid()) RotationMode = ClosestInputs->RotationMode;
		if(ClosestInputs->Stance.IsValid()) Stance = ClosestInputs->Stance;
		if(ClosestInputs->Gait.IsValid()) Gait = ClosestInputs->Gait;
	}
	virtual void Merge(const FMoverDataStructBase& From) override
	{
		Super::Merge(From);
		auto& TypedFrom = static_cast<const FGarCharacterMoverSyncState&>(From);
		if(TypedFrom.RotationMode.IsValid()) RotationMode = TypedFrom.RotationMode;
		if(TypedFrom.Stance.IsValid()) Stance = TypedFrom.Stance;
		if(TypedFrom.Gait.IsValid()) Gait = TypedFrom.Gait;
	}
	//virtual void Decay(float DecayAmount) override { Super::Decay(DecayAmount); }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mover)
	FGameplayTag RotationMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mover)
	FGameplayTag Stance;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mover)
	FGameplayTag Gait;
};

template<>
struct TStructOpsTypeTraits<FGarCharacterMoverSyncState> : public TStructOpsTypeTraitsBase2<FGarCharacterMoverSyncState>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
