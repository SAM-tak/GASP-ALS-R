#include "GarCharacter.h"

#include "DisplayDebugHelpers.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "GarPhysicalAnimationComponent.h"
#include "GarConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarUtility.h"

#define LOCTEXT_NAMESPACE "GarCharacterDebug"

#if !UE_BUILD_SHIPPING
void AGarCharacter::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation)
{
	const auto Scale{FMath::Min(Canvas->SizeX / (1280.0f * Canvas->GetDPIScale()), Canvas->SizeY / (720.0f * Canvas->GetDPIScale()))};

	const auto RowOffset{12.0f * Scale};
	const auto ColumnOffset{200.0f * Scale};

	auto MaxVerticalLocation{VerticalLocation};
	auto HorizontalLocation{5.0f * Scale};

	static const auto DebugModeHeaderText{LOCTEXT("DebugModeHeader", "Debug mode is enabled! Press (Shift + 0) to disable.")};

	DisplayDebugHeader(Canvas, DebugModeHeaderText, FLinearColor::Green, Scale, HorizontalLocation, VerticalLocation);

	VerticalLocation += RowOffset;
	MaxVerticalLocation = FMath::Max(MaxVerticalLocation, VerticalLocation);

	if (!DisplayInfo.IsDisplayOn(UGarConstants::CurvesDebugDisplayName()) &&
	    !DisplayInfo.IsDisplayOn(UGarConstants::ShapesDebugDisplayName()) &&
	    !DisplayInfo.IsDisplayOn(UGarConstants::TracesDebugDisplayName()) &&
	    !DisplayInfo.IsDisplayOn(UGarConstants::TraversalDebugDisplayName()) &&
	    !DisplayInfo.IsDisplayOn(UGarConstants::PADebugDisplayName()))
	{
		VerticalLocation = MaxVerticalLocation;

		OnDisplayDebug.Broadcast(Canvas, DisplayInfo, Unused, VerticalLocation);

		Super::DisplayDebug(Canvas, DisplayInfo, Unused, VerticalLocation);
		return;
	}

	const auto InitialVerticalLocation{VerticalLocation};

	static const auto CurvesHeaderText{FText::AsCultureInvariant(FString{TEXTVIEW("Gar.Curves (Shift + 1)")})};

	if (DisplayInfo.IsDisplayOn(UGarConstants::CurvesDebugDisplayName()))
	{
		DisplayDebugHeader(Canvas, CurvesHeaderText, FLinearColor::Green, Scale, HorizontalLocation, VerticalLocation);
		DisplayDebugCurves(Canvas, Scale, HorizontalLocation, VerticalLocation);

		MaxVerticalLocation = FMath::Max(MaxVerticalLocation, VerticalLocation + RowOffset);
		VerticalLocation = InitialVerticalLocation;
		HorizontalLocation += ColumnOffset;
	}
	else
	{
		DisplayDebugHeader(Canvas, CurvesHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);

		VerticalLocation += RowOffset;
	}

	MaxVerticalLocation = FMath::Max(MaxVerticalLocation, VerticalLocation);

	static const auto ShapesHeaderText{FText::AsCultureInvariant(FString{TEXTVIEW("Gar.Shapes (Shift + 2)")})};

	if (DisplayInfo.IsDisplayOn(UGarConstants::ShapesDebugDisplayName()))
	{
		DisplayDebugHeader(Canvas, ShapesHeaderText, FLinearColor::Green, Scale, HorizontalLocation, VerticalLocation);
		DisplayDebugShapes(Canvas, Scale, HorizontalLocation, VerticalLocation);
	}
	else
	{
		DisplayDebugHeader(Canvas, ShapesHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);
	}

	VerticalLocation += RowOffset;
	MaxVerticalLocation = FMath::Max(MaxVerticalLocation, VerticalLocation);

	static const auto TracesHeaderText{FText::AsCultureInvariant(FString{TEXTVIEW("Gar.Traces (Shift + 3)")})};

	if (DisplayInfo.IsDisplayOn(UGarConstants::TracesDebugDisplayName()))
	{
		DisplayDebugHeader(Canvas, TracesHeaderText, FLinearColor::Green, Scale, HorizontalLocation, VerticalLocation);
		DisplayDebugTraces(Canvas, Scale, HorizontalLocation, VerticalLocation);
	}
	else
	{
		DisplayDebugHeader(Canvas, TracesHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);
	}

	VerticalLocation += RowOffset;
	MaxVerticalLocation = FMath::Max(MaxVerticalLocation, VerticalLocation);

	static const auto MantlingHeaderText{FText::AsCultureInvariant(FString{TEXTVIEW("Gar.Traversal (Shift + 4)")})};

	if (DisplayInfo.IsDisplayOn(UGarConstants::TraversalDebugDisplayName()))
	{
		DisplayDebugHeader(Canvas, MantlingHeaderText, FLinearColor::Green, Scale, HorizontalLocation, VerticalLocation);
		DisplayDebugTraversal(Canvas, Scale, HorizontalLocation, VerticalLocation);
	}
	else
	{
		DisplayDebugHeader(Canvas, MantlingHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);
	}

	static const auto PAHeaderText{FText::AsCultureInvariant(FString{TEXTVIEW("Gar.PhysicalAnimation (Shift + 5)")})};

	if (DisplayInfo.IsDisplayOn(UGarConstants::PADebugDisplayName()))
	{
		DisplayDebugHeader(Canvas, PAHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);
		PhysicalAnimation->DisplayDebug(Canvas, DisplayInfo, HorizontalLocation, VerticalLocation);
	}
	else
	{
		DisplayDebugHeader(Canvas, PAHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);
	}

	VerticalLocation += RowOffset;
	MaxVerticalLocation = FMath::Max(MaxVerticalLocation, VerticalLocation);

	VerticalLocation = MaxVerticalLocation;

	OnDisplayDebug.Broadcast(Canvas, DisplayInfo, Unused, VerticalLocation);

	Super::DisplayDebug(Canvas, DisplayInfo, Unused, VerticalLocation);
}

void AGarCharacter::DisplayDebugHeader(const UCanvas* Canvas, const FText& HeaderText, const FLinearColor& HeaderColor,
                                       const float Scale, const float HorizontalLocation, float& VerticalLocation)
{
	FCanvasTextItem Text{
		{HorizontalLocation, VerticalLocation},
		HeaderText,
		GEngine->GetMediumFont(),
		HeaderColor
	};

	Text.Scale = {Scale, Scale};
	Text.EnableShadow(FLinearColor::Black);

	Text.Draw(Canvas->Canvas);

	VerticalLocation += 15.0f * Scale;
}

void AGarCharacter::InitializeCurveNames()
{
	CurveNames.Reset();
	// gave up
	//UClass* _Class = UGarConstants::StaticClass();
	//for (TFieldIterator<UFunction> FuncIt(_Class); FuncIt; ++FuncIt)
	//{
	//	if (FuncIt->HasAnyFunctionFlags(FUNC_Static))
	//	{
	//		const auto ReturnDisplayName = FuncIt->FindMetaData("ReturnDisplayName");
	//		if (ReturnDisplayName && ReturnDisplayName->Equals("Curve Name"))
	//		{
	//			const FName& CurveName = FuncIt->CallStaticFunction();
	//			CurveNames.AddUniqueUnique(CurveName);
	//		}
	//	}
	//}
	
	// Layering Animation Curves
	CurveNames.AddUnique(UGarConstants::LayerHeadCurveName());
	CurveNames.AddUnique(UGarConstants::LayerHeadAdditiveCurveName());
	CurveNames.AddUnique(UGarConstants::LayerHeadSlotCurveName());
	CurveNames.AddUnique(UGarConstants::LayerArmLeftCurveName());
	CurveNames.AddUnique(UGarConstants::LayerArmLeftAdditiveCurveName());
	CurveNames.AddUnique(UGarConstants::LayerArmLeftLocalSpaceCurveName());
	CurveNames.AddUnique(UGarConstants::LayerArmLeftSlotCurveName());
	CurveNames.AddUnique(UGarConstants::LayerArmRightCurveName());
	CurveNames.AddUnique(UGarConstants::LayerArmRightAdditiveCurveName());
	CurveNames.AddUnique(UGarConstants::LayerArmRightLocalSpaceCurveName());
	CurveNames.AddUnique(UGarConstants::LayerArmRightSlotCurveName());
	CurveNames.AddUnique(UGarConstants::LayerHandLeftCurveName());
	CurveNames.AddUnique(UGarConstants::LayerHandRightCurveName());
	CurveNames.AddUnique(UGarConstants::LayerSpineCurveName());
	CurveNames.AddUnique(UGarConstants::LayerSpineAdditiveCurveName());
	CurveNames.AddUnique(UGarConstants::LayerSpineSlotCurveName());
	CurveNames.AddUnique(UGarConstants::LayerPelvisCurveName());
	CurveNames.AddUnique(UGarConstants::LayerPelvisSlotCurveName());
	CurveNames.AddUnique(UGarConstants::LayerLegsCurveName());
	CurveNames.AddUnique(UGarConstants::LayerLegsSlotCurveName());
	CurveNames.AddUnique(UGarConstants::HandLeftIkCurveName());
	CurveNames.AddUnique(UGarConstants::HandRightIkCurveName());

	// Pose Animation Curves
	CurveNames.AddUnique(UGarConstants::PoseGaitCurveName());
	CurveNames.AddUnique(UGarConstants::PoseMovingCurveName());
	CurveNames.AddUnique(UGarConstants::PoseStandingCurveName());
	CurveNames.AddUnique(UGarConstants::PoseCrouchingCurveName());
	CurveNames.AddUnique(UGarConstants::PoseGroundedCurveName());
	CurveNames.AddUnique(UGarConstants::PoseInAirCurveName());
	CurveNames.AddUnique(UGarConstants::PoseAimingCurveName());

	// Feet Animation Curves
	CurveNames.AddUnique(UGarConstants::FootLeftIkCurveName());
	CurveNames.AddUnique(UGarConstants::FootRightIkCurveName());

	// Other Animation Curves
	CurveNames.AddUnique(UGarConstants::AllowTransitionsCurveName());
	CurveNames.AddUnique(UGarConstants::BlockViewCurveName());
	CurveNames.AddUnique(UGarConstants::BlockSprintCurveName());
	CurveNames.AddUnique(UGarConstants::FootstepSoundBlockCurveName());

	// Physical Animation Curves
	auto CurveBoneMappings{PhysicalAnimation->GetCurveBoneMappings()};
	for(const auto& Mapping : CurveBoneMappings)
	{
		CurveNames.AddUnique(Mapping.CurveName);
	}
}

void AGarCharacter::DisplayDebugCurves(const UCanvas* Canvas, const float Scale,
                                       const float HorizontalLocation, float& VerticalLocation) const
{
	VerticalLocation += 4.0f * Scale;

	FCanvasTextItem Text{
		FVector2D::ZeroVector,
		FText::GetEmpty(),
		GEngine->GetSmallFont(),
		FLinearColor::White
	};

	Text.Scale = {Scale * 0.75f, Scale * 0.75f};
	Text.EnableShadow(FLinearColor::Black);

	const auto RowOffset{10.0f * Scale};
	const auto ColumnOffset{145.0f * Scale};

	if (CurveNames.Num() == 0)
	{
		const_cast<AGarCharacter *>(this)->InitializeCurveNames();
	}

	TStringBuilder<32> CurveValueBuilder;

	TArray<FName> MeshCurveNames;
	GetMesh()->GetSkeletalMeshAsset()->GetSkeleton()->GetCurveMetaDataNames(MeshCurveNames);
	for (const auto& CurveName : CurveNames)
	{
		// Skip if CurveName doesn't exist in UGarConstants's Animation Curve Name static members.
		if (!MeshCurveNames.Contains(CurveName))
		{
			continue;
		}

		const auto CurveValue{GetMesh()->GetAnimInstance()->GetCurveValue(CurveName)};

		Text.SetColor(FMath::Lerp(FLinearColor::Gray, FLinearColor::White, UGarMath::Clamp01(CurveValue)));

		Text.Text = FText::AsCultureInvariant(FName::NameToDisplayString(CurveName.ToString(), false));
		Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

		CurveValueBuilder.Appendf(TEXT("%.2f"), CurveValue);

		Text.Text = FText::AsCultureInvariant(FString{CurveValueBuilder});
		Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

		CurveValueBuilder.Reset();

		VerticalLocation += RowOffset;
	}
}

void AGarCharacter::DisplayDebugShapes(const UCanvas* Canvas, const float Scale,
                                       const float HorizontalLocation, float& VerticalLocation) const
{
	VerticalLocation += 4.0f * Scale;

	TStringBuilder<256> DebugStringBuilder;

	FCanvasTextItem Text{
		FVector2D::ZeroVector,
		FText::GetEmpty(),
		GEngine->GetMediumFont(),
		FLinearColor::White
	};

	Text.Scale = {Scale * 0.75f, Scale * 0.75f};
	Text.EnableShadow(FLinearColor::Black);

	const auto RowOffset{12.0f * Scale};
	const auto ColumnOffset{120.0f * Scale};

	static const auto ViewRotationText{FText::AsCultureInvariant("Rotation")};
	const auto ViewRotation{GetViewRotation()};

	auto Color{FLinearColor::Red};
	Text.SetColor(Color);

	Text.Text = ViewRotationText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	DebugStringBuilder << TEXTVIEW("R:");
	DebugStringBuilder.Appendf(TEXT("%.2f"), ViewRotation.Roll);
	DebugStringBuilder << TEXTVIEW(" P:");
	DebugStringBuilder.Appendf(TEXT("%.2f"), ViewRotation.Pitch);
	DebugStringBuilder << TEXTVIEW(" Y:");
	DebugStringBuilder.Appendf(TEXT("%.2f"), ViewRotation.Yaw);

	Text.Text = FText::AsCultureInvariant(FString{DebugStringBuilder});
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	DebugStringBuilder.Reset();

#if ENABLE_DRAW_DEBUG
	DrawDebugCone(GetWorld(), GetPawnViewLocation(),
				  ViewRotation.Vector(), 100.0f, FMath::DegreesToRadians(15.0f), FMath::DegreesToRadians(15.0f),
	              8, Color.ToFColor(true), false, -1.0f, SDPG_World, 1.0f);
#endif

	VerticalLocation += RowOffset;

	static const auto InputYawAngleText{FText::AsCultureInvariant(FName::NameToDisplayString(GET_MEMBER_NAME_STRING_CHECKED(ThisClass, InputYawAngle), false))};

	Color = HasMovementInput() ? FLinearColor{1.0f, 0.5f, 0.0f} : FLinearColor{0.5f, 0.25f, 0.0f};
	Text.SetColor(Color);

	Text.Text = InputYawAngleText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	DebugStringBuilder.Appendf(TEXT("%.2f"), InputYawAngle);

	Text.Text = FText::AsCultureInvariant(FString{DebugStringBuilder});
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	DebugStringBuilder.Reset();

#if ENABLE_DRAW_DEBUG
	const auto FeetLocation{GetActorLocation() - FVector{0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight()}};

	DrawDebugDirectionalArrow(GetWorld(),
	                          FeetLocation + FVector{0.0f, 0.0f, 3.0f},
	                          FeetLocation + FVector{0.0f, 0.0f, 3.0f} +
	                          UGarMath::AngleToDirectionXY(InputYawAngle) * 50.0f,
	                          50.0f, Color.ToFColor(true), false, -1.0f, SDPG_World, 3.0f);
#endif

	VerticalLocation += RowOffset;

	static const auto SpeedText{FText::AsCultureInvariant("Speed")};

	Color = HasSpeed() ? FLinearColor{0.75f, 0.0f, 1.0f} : FLinearColor{0.375f, 0.0f, 0.5f};
	Text.SetColor(Color);

	Text.Text = SpeedText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	DebugStringBuilder.Appendf(TEXT("%.2f"), GetVelocity().Size2D());

	Text.Text = FText::AsCultureInvariant(FString{DebugStringBuilder});
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	DebugStringBuilder.Reset();

	VerticalLocation += RowOffset;

	static const auto VelocityDirectionText{FText::AsCultureInvariant(FString{TEXTVIEW("Velocity Direction")})};

	const auto VelocityDirection{GetVelocity().GetSafeNormal()};

	Text.Text = VelocityDirectionText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	DebugStringBuilder << TEXTVIEW("X:");
	DebugStringBuilder.Appendf(TEXT("%.2f"), VelocityDirection.X);
	DebugStringBuilder << TEXTVIEW(" Y:");
	DebugStringBuilder.Appendf(TEXT("%.2f"), VelocityDirection.Y);
	DebugStringBuilder << TEXTVIEW(" Z:");
	DebugStringBuilder.Appendf(TEXT("%.2f"), VelocityDirection.Z);

	Text.Text = FText::AsCultureInvariant(FString{DebugStringBuilder});
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	DebugStringBuilder.Reset();

	VerticalLocation += RowOffset;

#if ENABLE_DRAW_DEBUG
	DrawDebugCapsule(GetWorld(), Capsule->GetComponentLocation(), Capsule->GetScaledCapsuleHalfHeight(),
	                 Capsule->GetScaledCapsuleRadius(), Capsule->GetComponentRotation().Quaternion(),
	                 FColor::Green, false, -1.0f, SDPG_World, 1.0f);
	if (ProneCapsule->IsWelded())
	{
		DrawDebugCapsule(GetWorld(), ProneCapsule->GetComponentLocation(), ProneCapsule->GetScaledCapsuleHalfHeight(),
						 ProneCapsule->GetScaledCapsuleRadius(), ProneCapsule->GetComponentRotation().Quaternion(),
						 FColor::Green, false, -1.0f, SDPG_World, 1.0f);
	}
#endif
}

void AGarCharacter::DisplayDebugTraces(const UCanvas* Canvas, const float Scale,
                                       const float HorizontalLocation, float& VerticalLocation) const
{
	VerticalLocation += 4.0f * Scale;

	FCanvasTextItem Text{
		FVector2D::ZeroVector,
		FText::GetEmpty(),
		GEngine->GetMediumFont(),
		FLinearColor::White
	};

	Text.Scale = {Scale * 0.75f, Scale * 0.75f};
	Text.EnableShadow(FLinearColor::Black);

	const auto RowOffset{12.0f * Scale};

	static const auto FootOffsetTraceText{LOCTEXT("FootOffsetTrace", "Foot Offset")};

	Text.SetColor({0.0f, 0.75f, 1.0f});

	Text.Text = FootOffsetTraceText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto GroundPredictionTraceText{LOCTEXT("GroundPredictionTrace", "Ground Prediction")};

	Text.SetColor({0.75f, 0.0f, 1.0f});

	Text.Text = GroundPredictionTraceText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto FootstepEffectsTraceText{LOCTEXT("FootstepEffectsTrace", "Footstep Effects")};

	Text.SetColor(FLinearColor::Red);

	Text.Text = FootstepEffectsTraceText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;
}

void AGarCharacter::DisplayDebugTraversal(const UCanvas* Canvas, const float Scale, const float HorizontalLocation, float& VerticalLocation) const
{
	VerticalLocation += 4.0f * Scale;

	FCanvasTextItem Text{
		FVector2D::ZeroVector,
		FText::GetEmpty(),
		GEngine->GetMediumFont(),
		FLinearColor::White
	};

	Text.Scale = {Scale * 0.75f, Scale * 0.75f};
	Text.EnableShadow(FLinearColor::Black);

	const auto RowOffset{12.0f * Scale};

	static const auto ForwardTraceText{LOCTEXT("ForwardTrace", "Forward Trace")};

	Text.SetColor({0.0f, 0.75f, 1.0f});

	Text.Text = ForwardTraceText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto DownwardTraceText{LOCTEXT("DownwardTrace", "Downward Trace")};

	Text.SetColor({0.75f, 0.0f, 1.0f});

	Text.Text = DownwardTraceText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto FailedTargetLocationOverlapText{LOCTEXT("FailedTargetocationOverlap", "Failed Target Location Overlap")};

	Text.SetColor(FLinearColor::Red);

	Text.Text = FailedTargetLocationOverlapText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto FailedStartLocationOverlapText{LOCTEXT("FailedStartLocationOverlap", "Failed Start Location Overlap")};

	Text.SetColor({1.0f, 0.5f, 0.0f});

	Text.Text = FailedStartLocationOverlapText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;
}
#endif // !UE_BUILD_SHIPPING

#undef LOCTEXT_NAMESPACE
