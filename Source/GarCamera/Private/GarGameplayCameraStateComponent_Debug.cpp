#include "GarGameplayCameraStateComponent.h"

#include "DisplayDebugHelpers.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GarCharacter.h"
#include "GarConstants.h"
#include "GarCameraConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarUtility.h"

#define LOCTEXT_NAMESPACE "GarGameplayCameraStateComponentDebug"

#if !UE_BUILD_SHIPPING
void UGarGameplayCameraStateComponent::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation)
{
	const auto Scale{FMath::Min(Canvas->SizeX / (1280.0f * Canvas->GetDPIScale()), Canvas->SizeY / (720.0f * Canvas->GetDPIScale()))};

	const auto RowOffset{12.0f * Scale};
	const auto ColumnOffset{200.0f * Scale};

	auto MaxVerticalLocation{VerticalLocation};
	auto HorizontalLocation{5.0f * Scale};

	static const auto StateHeaderText{FText::AsCultureInvariant(FString{TEXTVIEW("Gar.CameraState (Shift + 6)")})};
	static const auto TracesHeaderText{FText::AsCultureInvariant(FString{TEXTVIEW("Gar.CameraTraces (Shift + 7)")})};

	if (!DisplayInfo.IsDisplayOn(UGarCameraConstants::CameraCurvesDebugDisplayName()) &&
	    !DisplayInfo.IsDisplayOn(UGarCameraConstants::CameraTracesDebugDisplayName()))
	{
		if (!DisplayInfo.IsDisplayOn(UGarConstants::CurvesDebugDisplayName()) &&
			!DisplayInfo.IsDisplayOn(UGarConstants::ShapesDebugDisplayName()) &&
			!DisplayInfo.IsDisplayOn(UGarConstants::TracesDebugDisplayName()) &&
			!DisplayInfo.IsDisplayOn(UGarConstants::TraversalDebugDisplayName()) &&
			!DisplayInfo.IsDisplayOn(UGarConstants::PADebugDisplayName()))
		{
			return;
		}

		DisplayDebugHeader(Canvas, StateHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);
		VerticalLocation += RowOffset;
		DisplayDebugHeader(Canvas, TracesHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);
		VerticalLocation += RowOffset;
		return;
	}

	const auto InitialVerticalLocation{VerticalLocation};

	if (DisplayInfo.IsDisplayOn(UGarCameraConstants::CameraCurvesDebugDisplayName()))
	{
		DisplayDebugHeader(Canvas, StateHeaderText, FLinearColor::Green, Scale, HorizontalLocation, VerticalLocation);
		DisplayDebugState(Canvas, Scale, HorizontalLocation, VerticalLocation);

		MaxVerticalLocation = FMath::Max(MaxVerticalLocation, VerticalLocation + RowOffset);
		VerticalLocation = InitialVerticalLocation;
		HorizontalLocation += ColumnOffset;
	}
	else
	{
		DisplayDebugHeader(Canvas, StateHeaderText, {0.0f, 0.333333f, 0.0f}, Scale, HorizontalLocation, VerticalLocation);

		VerticalLocation += RowOffset;
	}

	MaxVerticalLocation = FMath::Max(MaxVerticalLocation, VerticalLocation);

	if (DisplayInfo.IsDisplayOn(UGarCameraConstants::CameraTracesDebugDisplayName()))
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

	VerticalLocation = MaxVerticalLocation;
}

void UGarGameplayCameraStateComponent::DisplayDebugHeader(const UCanvas* Canvas, const FText& HeaderText, const FLinearColor& HeaderColor,
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

void UGarGameplayCameraStateComponent::DisplayDebugState(const UCanvas* Canvas, const float Scale, const float HorizontalLocation, float& VerticalLocation) const
{
	VerticalLocation += 4.0f * Scale;

	TStringBuilder<256> DebugStringBuilder;

	FCanvasTextItem Text{
		FVector2D::ZeroVector,
		FText::GetEmpty(),
		GEngine->GetSmallFont(),
		FLinearColor::White
	};

	Text.Scale = {Scale * 0.75f, Scale * 0.75f};
	Text.EnableShadow(FLinearColor::Black);

	const auto RowOffset{12.0f * Scale};
	const auto ColumnOffset{145.0f * Scale};

	static const auto DesiredPerspectiveText{
		FText::AsCultureInvariant(FName::NameToDisplayString(GET_MEMBER_NAME_STRING_CHECKED(ThisClass, DesiredPerspective), false))
	};

	Text.Text = DesiredPerspectiveText;
	Text.SetColor(FLinearColor::White);
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	Text.Text = FText::AsCultureInvariant(FName::NameToDisplayString(UGarUtility::GetSimpleTagName(DesiredPerspective).ToString(), false));
	Text.SetColor(FLinearColor::White);
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto ConfirmedDesiredPerspectiveText{
		FText::AsCultureInvariant(FName::NameToDisplayString(GET_MEMBER_NAME_STRING_CHECKED(ThisClass, ConfirmedDesiredPerspective), false))
	};

	Text.Text = ConfirmedDesiredPerspectiveText;
	Text.SetColor(FLinearColor::White);
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	Text.Text = FText::AsCultureInvariant(FName::NameToDisplayString(UGarUtility::GetSimpleTagName(ConfirmedDesiredPerspective).ToString(), false));
	Text.SetColor(ConfirmedDesiredPerspective == DesiredPerspective ? FLinearColor::White : FLinearColor::Red);
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto PerspectiveText{
		FText::AsCultureInvariant(FName::NameToDisplayString(GET_MEMBER_NAME_STRING_CHECKED(ThisClass, Perspective), false))
	};

	Text.Text = PerspectiveText;
	Text.SetColor(FLinearColor::White);
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	Text.Text = FText::AsCultureInvariant(FName::NameToDisplayString(UGarUtility::GetSimpleTagName(Perspective).ToString(), false));
	Text.SetColor(Perspective == ConfirmedDesiredPerspective ? FLinearColor::White : FLinearColor::Red);
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto DesiredShoulderModeText{
		FText::AsCultureInvariant(FName::NameToDisplayString(GET_MEMBER_NAME_STRING_CHECKED(ThisClass, DesiredShoulderMode), false))
	};

	Text.Text = DesiredShoulderModeText;
	Text.SetColor(FLinearColor::White);
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	Text.Text = FText::AsCultureInvariant(FName::NameToDisplayString(UGarUtility::GetSimpleTagName(DesiredShoulderMode).ToString(), false));
	Text.SetColor(FLinearColor::White);
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto ShoulderModeText{
		FText::AsCultureInvariant(FName::NameToDisplayString(GET_MEMBER_NAME_STRING_CHECKED(ThisClass, ShoulderMode), false))
	};

	Text.Text = ShoulderModeText;
	Text.SetColor(FLinearColor::White);
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	Text.Text = FText::AsCultureInvariant(FName::NameToDisplayString(UGarUtility::GetSimpleTagName(ShoulderMode).ToString(), false));
	Text.SetColor(ShoulderMode == DesiredShoulderMode ? FLinearColor::White : FLinearColor::Red);
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto FirstPersonFactorText{
		FText::AsCultureInvariant(FName::NameToDisplayString(GET_MEMBER_NAME_STRING_CHECKED(ThisClass, FirstPersonFactor), false))
	};

	Text.Text = FirstPersonFactorText;
	Text.SetColor(FLinearColor::White);
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	DebugStringBuilder.Appendf(TEXT("%.2f"), FirstPersonFactor);

	Text.Text = FText::AsCultureInvariant(FString{DebugStringBuilder});
	Text.SetColor(FMath::Lerp(FLinearColor::Gray, FLinearColor::White, UGarMath::Clamp01(FirstPersonFactor)));
	Text.Draw(Canvas->Canvas, {HorizontalLocation + ColumnOffset, VerticalLocation});

	DebugStringBuilder.Reset();

	VerticalLocation += RowOffset;
}

void UGarGameplayCameraStateComponent::DisplayDebugTraces(const UCanvas* Canvas, const float Scale,
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

	static const auto BlockingGeometryAdjustmentText{LOCTEXT("BlockingGeometryAdjustment", "Blocking Geometry Adjustment")};

	Text.SetColor({0.0f, 0.75f, 1.0f});

	Text.Text = BlockingGeometryAdjustmentText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto CameraTraceNoHitText{LOCTEXT("CameraTraceNoHit", "Camera Trace (No Hit)")};

	Text.SetColor(FLinearColor::Green);

	Text.Text = CameraTraceNoHitText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;

	static const auto CameraTraceBlockingHitText{LOCTEXT("CameraTraceBlockingHit", "Camera Trace (Blocking Hit)")};

	Text.SetColor(FLinearColor::Red);

	Text.Text = CameraTraceBlockingHitText;
	Text.Draw(Canvas->Canvas, {HorizontalLocation, VerticalLocation});

	VerticalLocation += RowOffset;
}
#endif // !UE_BUILD_SHIPPING

#undef LOCTEXT_NAMESPACE
