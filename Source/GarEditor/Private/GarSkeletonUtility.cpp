#include "GarSkeletonUtility.h"

#include "Animation/BlendProfile.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarSkeletonUtility)

#define LOCTEXT_NAMESPACE "GarSkeletonUtility"

namespace GarSkeletonUtility
{
	void LogMissingBoneWarning(const USkeleton* Skeleton, const FName& BoneName, const FString& FunctionName)
	{
		FMessageLog MessageLog{GarLog::MessageLogName};

		MessageLog.Warning(FText::Format(
			          LOCTEXT("MissingBoneWarning", "{FunctionName}: Bone {BoneName} does not exist on the {SkeletonName} skeleton!"),
			          {
				          {FString{TEXTVIEW("FunctionName")}, FText::AsCultureInvariant(FunctionName)},
				          {FString{TEXTVIEW("BoneName")}, FText::AsCultureInvariant(BoneName.ToString())},
				          {FString{TEXTVIEW("SkeletonName")}, FText::AsCultureInvariant(Skeleton->GetName())}
			          }))
		          ->AddToken(FUObjectToken::Create(Skeleton));

		MessageLog.Open(EMessageSeverity::Warning);
	}
}

void UGarSkeletonUtility::AddAnimationCurves(USkeleton* Skeleton, const TArray<FName>& CurveNames)
{
	if (!ensure(IsValid(Skeleton)))
	{
		return;
	}

	FMessageLog MessageLog{GarLog::MessageLogName};

	for (FName CurveName : CurveNames)
	{
		CurveName = *CurveName.ToString().TrimStartAndEnd();
		if (CurveName.IsNone())
		{
			MessageLog.Warning(FText::Format(
				LOCTEXT("EmptyCurveNameWarning", "{FunctionName}: Curve must have a valid name!"),
				{
					{FString{TEXTVIEW("FunctionName")}, FText::AsCultureInvariant(__FUNCTION__)}
				}));

			MessageLog.Open(EMessageSeverity::Warning);
			continue;
		}

		Skeleton->AddCurveMetaData(CurveName);
	}
}

void UGarSkeletonUtility::AddOrReplaceSlot(USkeleton* Skeleton, FName SlotName, FName GroupName)
{
	if (!ensure(IsValid(Skeleton)))
	{
		return;
	}

	SlotName = *SlotName.ToString().TrimStartAndEnd();
	if (SlotName.IsNone())
	{
		FMessageLog MessageLog{GarLog::MessageLogName};

		MessageLog.Warning(FText::Format(
			LOCTEXT("EmptySlotNameWarning", "{FunctionName}: Slot must have a valid name!"),
			{
				{FString{TEXTVIEW("FunctionName")}, FText::AsCultureInvariant(__FUNCTION__)}
			}));

		MessageLog.Open(EMessageSeverity::Warning);
		return;
	}

	GroupName = *GroupName.ToString().TrimStartAndEnd();
	if (GroupName.IsNone())
	{
		FMessageLog MessageLog{GarLog::MessageLogName};

		MessageLog.Warning(FText::Format(
			LOCTEXT("EmptySlotGroupNameWarning", "{FunctionName}: Slot group must have a valid name!"),
			{
				{FString{TEXTVIEW("FunctionName")}, FText::AsCultureInvariant(__FUNCTION__)}
			}));

		MessageLog.Open(EMessageSeverity::Warning);
		return;
	}

	if (!Skeleton->ContainsSlotName(SlotName) || Skeleton->GetSlotGroupName(SlotName) != GroupName)
	{
		Skeleton->Modify();
		Skeleton->SetSlotGroupName(SlotName, GroupName);
	}
}

void UGarSkeletonUtility::AddOrReplaceVirtualBone(USkeleton* Skeleton, const FName& SourceBoneName,
                                                  const FName& TargetBoneName, FName VirtualBoneName)
{
	if (!ensure(IsValid(Skeleton)))
	{
		return;
	}

	const auto VirtualBoneString{VirtualBoneName.ToString().TrimStartAndEnd()};
	VirtualBoneName = *VirtualBoneString;

	if (VirtualBoneName.IsNone())
	{
		FMessageLog MessageLog{GarLog::MessageLogName};

		MessageLog.Warning(FText::Format(
			LOCTEXT("EmptyVirtualBoneNameWarning", "{FunctionName}: Virtual bone must have a valid name!"),
			{
				{FString{TEXTVIEW("FunctionName")}, FText::AsCultureInvariant(__FUNCTION__)}
			}));

		MessageLog.Open(EMessageSeverity::Warning);
		return;
	}

	if (!VirtualBoneString.StartsWith(TEXTVIEW("VB "), ESearchCase::CaseSensitive))
	{
		FMessageLog MessageLog{GarLog::MessageLogName};

		MessageLog.Warning(FText::Format(
			LOCTEXT("NoVirtualBonePrefixWarning", "{FunctionName}: Virtual bone {VirtualBoneName} must contain the \"VB \" prefix!"),
			{
				{FString{TEXTVIEW("FunctionName")}, FText::AsCultureInvariant(__FUNCTION__)},
				{FString{TEXTVIEW("VirtualBoneName")}, FText::AsCultureInvariant(VirtualBoneString)}
			}));

		MessageLog.Open(EMessageSeverity::Warning);
		return;
	}

	const auto* ExistingVirtualBone{
		Skeleton->GetVirtualBones().FindByPredicate([&VirtualBoneName](const FVirtualBone& VirtualBone)
		{
			return VirtualBone.VirtualBoneName == VirtualBoneName;
		})
	};

	if (ExistingVirtualBone != nullptr &&
	    ExistingVirtualBone->SourceBoneName == SourceBoneName &&
	    ExistingVirtualBone->TargetBoneName == TargetBoneName)
	{
		return;
	}

	if (Skeleton->GetReferenceSkeleton().FindBoneIndex(SourceBoneName) < 0)
	{
		GarSkeletonUtility::LogMissingBoneWarning(Skeleton, SourceBoneName, __FUNCTION__);
		return;
	}

	if (Skeleton->GetReferenceSkeleton().FindBoneIndex(TargetBoneName) < 0)
	{
		GarSkeletonUtility::LogMissingBoneWarning(Skeleton, TargetBoneName, __FUNCTION__);
		return;
	}

	if (ExistingVirtualBone != nullptr)
	{
		ExistingVirtualBone = nullptr;

		Skeleton->RemoveVirtualBones({VirtualBoneName});
	}

	FName TempVirtualBoneName;

	Skeleton->AddNewVirtualBone(SourceBoneName, TargetBoneName, TempVirtualBoneName);
	Skeleton->RenameVirtualBone(TempVirtualBoneName, VirtualBoneName);
}

void UGarSkeletonUtility::AddOrReplaceSocket(USkeleton* Skeleton, FName SocketName, const FName& BoneName,
                                             const FVector& RelativeLocation, const FRotator& RelativeRotation)
{
	if (!ensure(IsValid(Skeleton)))
	{
		return;
	}

	SocketName = *SocketName.ToString().TrimStartAndEnd();
	if (SocketName.IsNone())
	{
		FMessageLog MessageLog{GarLog::MessageLogName};

		MessageLog.Warning(FText::Format(
			LOCTEXT("EmptySocketNameWarning", "{FunctionName}: Socket must have a valid name!"),
			{
				{FString{TEXTVIEW("FunctionName")}, FText::AsCultureInvariant(__FUNCTION__)}
			}));

		MessageLog.Open(EMessageSeverity::Warning);
		return;
	}

	if (Skeleton->GetReferenceSkeleton().FindBoneIndex(BoneName) < 0)
	{
		GarSkeletonUtility::LogMissingBoneWarning(Skeleton, BoneName, __FUNCTION__);
		return;
	}

	auto* ExistingSocket{Skeleton->FindSocket(SocketName)};
	if (IsValid(ExistingSocket))
	{
		if (ExistingSocket->BoneName != BoneName ||
		    ExistingSocket->RelativeLocation != RelativeLocation ||
		    ExistingSocket->RelativeRotation != RelativeRotation ||
		    ExistingSocket->RelativeScale != FVector::OneVector ||
		    !ExistingSocket->bForceAlwaysAnimated)
		{
			Skeleton->Modify();
			ExistingSocket->Modify();

			ExistingSocket->BoneName = BoneName;
			ExistingSocket->RelativeLocation = RelativeLocation;
			ExistingSocket->RelativeRotation = RelativeRotation;
			ExistingSocket->RelativeScale = FVector::OneVector;
			ExistingSocket->bForceAlwaysAnimated = true;
		}

		return;
	}

	Skeleton->Modify();

	auto* Socket{NewObject<USkeletalMeshSocket>(Skeleton)};
	Socket->SocketName = SocketName;
	Socket->BoneName = BoneName;
	Socket->RelativeLocation = RelativeLocation;
	Socket->RelativeRotation = RelativeRotation;

	Skeleton->Sockets.Emplace(Socket);
}

void UGarSkeletonUtility::AddOrReplaceBlendProfile(USkeleton* Skeleton, FName BlendProfileName,
                                                   const TArray<FGarBlendProfileEntry>& Entries)
{
	if (!ensure(IsValid(Skeleton)))
	{
		return;
	}

	BlendProfileName = *BlendProfileName.ToString().TrimStartAndEnd();
	if (BlendProfileName.IsNone())
	{
		FMessageLog MessageLog{GarLog::MessageLogName};

		MessageLog.Warning(FText::Format(
			LOCTEXT("EmptyBlendProfileNameNameWarning", "{FunctionName}: Blend profile must have a valid name!"),
			{
				{FString{TEXTVIEW("FunctionName")}, FText::AsCultureInvariant(__FUNCTION__)}
			}));

		MessageLog.Open(EMessageSeverity::Warning);
		return;
	}

	auto* BlendProfile{Skeleton->GetBlendProfile(BlendProfileName)};
	if (IsValid(BlendProfile))
	{
		Skeleton->Modify();
		BlendProfile->Modify();
		BlendProfile->ProfileEntries.Reset();
	}
	else
	{
		BlendProfile = Skeleton->CreateNewBlendProfile(BlendProfileName);
	}

	BlendProfile->Mode = EBlendProfileMode::WeightFactor;

	for (const auto& Entry : Entries)
	{
		const auto BoneIndex{Skeleton->GetReferenceSkeleton().FindBoneIndex(Entry.BoneName)};
		if (BoneIndex < 0)
		{
			GarSkeletonUtility::LogMissingBoneWarning(Skeleton, Entry.BoneName, __FUNCTION__);
			continue;
		}

		BlendProfile->SetBoneBlendScale(BoneIndex, Entry.BlendScale, Entry.bIncludeDescendants, true);
	}
}

void UGarSkeletonUtility::SetBoneRetargetingMode(USkeleton* Skeleton, const FName& BoneName,
                                                 const EBoneTranslationRetargetingMode::Type RetargetingMode,
                                                 const bool bIncludeDescendants)
{
	if (!ensure(IsValid(Skeleton)))
	{
		return;
	}

	const auto BoneIndex{Skeleton->GetReferenceSkeleton().FindBoneIndex(BoneName)};
	if (BoneIndex < 0)
	{
		GarSkeletonUtility::LogMissingBoneWarning(Skeleton, BoneName, __FUNCTION__);
		return;
	}

	Skeleton->Modify();
	Skeleton->SetBoneTranslationRetargetingMode(BoneIndex, RetargetingMode, bIncludeDescendants);
}


bool UGarSkeletonUtility::HasBone(USkeleton* Skeleton, const FName& BoneName)
{
	if(!ensure(IsValid(Skeleton)))
	{
		return false;
	}

	return Skeleton->GetReferenceSkeleton().FindBoneIndex(BoneName) >= 0;
}

#undef LOCTEXT_NAMESPACE
