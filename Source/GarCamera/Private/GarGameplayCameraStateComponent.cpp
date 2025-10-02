#include "GarGameplayCameraStateComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameplayCameraComponentBase.h"
#include "AbilitySystemComponent.h"
#include "Core/CameraSystemEvaluator.h"
#include "Core/CameraVariableAssets.h"
#include "Core/RootCameraNode.h"
#include "Curves/CurveFloat.h"
#include "Engine/OverlapResult.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "GarGameplayCameraStateSettings.h"
#include "GarCharacter.h"
#include "GarCharacterMoverComponent.h"
#include "GarCameraConstants.h"
#include "Utility/GarMath.h"
#include "Utility/GarUtility.h"
#include "Utility/GarLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarGameplayCameraStateComponent)

UGarGameplayCameraStateComponent::UGarGameplayCameraStateComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	bAutoActivate = true;
	bTickInEditor = false;

	SetIsReplicatedByDefault(true);
	SetCanEverAffectNavigation(false);
}

void UGarGameplayCameraStateComponent::OnRegister()
{
	Super::OnRegister();

	Character = GetPawn<AGarCharacter>();

	if (!Character.IsValid())
	{
		UE_LOG(LogGar, Error, TEXT("[UGarCharacterComponent::OnRegister] This component has been added to a blueprint whose base class is not a child of GarCharacter. To use this component, it MUST be placed on a child of GarCharacter Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("GarCharacterComponent", "NotOnGarCharacterError", "has been added to a blueprint whose base class is not a child of GarCharacter. To use this component, it MUST be placed on a child of GarCharacter Blueprint. This will cause a crash if you PIE!");
			static const FName MessageLogName = TEXT("GarCharacterComponent");
			
			FMessageLog(MessageLogName).Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));
			
			FMessageLog(MessageLogName).Open();
		}
#endif
	}
	else
	{
		Character->OnPossessed_Client.AddUObject(this, &ThisClass::OnPossessed);
		Character->OnUnPossessed_Client.AddUObject(this, &ThisClass::OnUnPossessed);
		GameplayCameraComponent = Character->GetComponentByClass<UGameplayCameraComponentBase>();
	}
}

void UGarGameplayCameraStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ConfirmedDesiredPerspective, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ShoulderMode, Parameters)
}

void UGarGameplayCameraStateComponent::Activate(const bool bReset)
{
	Super::Activate(bReset);
#if !UE_BUILD_SHIPPING
	if (Character.IsValid())
	{
		Character->OnDisplayDebug.AddUObject(this, &ThisClass::DisplayDebug);
	}
#endif
	SetComponentTickEnabled(true);
}

void UGarGameplayCameraStateComponent::Deactivate()
{
	SetComponentTickEnabled(false);
#if !UE_BUILD_SHIPPING
	if (Character.IsValid())
	{
		Character->OnDisplayDebug.RemoveAll(this);
	}
#endif
	Super::Deactivate();
}

void UGarGameplayCameraStateComponent::OnPossessed_Implementation(AController* NewController)
{
	auto* NewPlayerController{Cast<APlayerController>(NewController)};
	if (IsValid(NewPlayerController) && GameplayCameraComponent.IsValid())
	{
		GameplayCameraComponent->ActivateCameraForPlayerController(NewPlayerController);
	}
}

void UGarGameplayCameraStateComponent::OnUnPossessed_Implementation(AController* PreviousController)
{
}

void UGarGameplayCameraStateComponent::InitializeByCameraVariables(
	UVector3dCameraVariable* BoomOffsetVariable,
	UVector3dCameraVariable* CenterShoulderOffsetVariable,
	UVector3dCameraVariable* LeftShoulderOffsetVariable,
	UVector3dCameraVariable* RightShoulderOffsetVariable)
{
	if (IsValid(BoomOffsetVariable))
	{
		BoomOffset = BoomOffsetVariable->GetDefaultValue();
		BoomOffsetVariableId = BoomOffsetVariable->GetVariableID();
	}
	if (IsValid(CenterShoulderOffsetVariable))
	{
		CenterShoulderOffset = CenterShoulderOffsetVariable->GetDefaultValue();
		CenterShoulderOffsetVariableId = CenterShoulderOffsetVariable->GetVariableID();
	}
	if (IsValid(LeftShoulderOffsetVariable))
	{
		LeftShoulderOffset = LeftShoulderOffsetVariable->GetDefaultValue();
		LeftShoulderOffsetVariableId = LeftShoulderOffsetVariable->GetVariableID();
	}
	if (IsValid(RightShoulderOffsetVariable))
	{
		RightShoulderOffset = RightShoulderOffsetVariable->GetDefaultValue();
		RightShoulderOffsetVariableId = RightShoulderOffsetVariable->GetVariableID();
	}
}

void UGarGameplayCameraStateComponent::BindCameraVariables(UFloatCameraVariable* FirstPersonFactorVariable,
	UFloatCameraVariable* TraceSphreRadiusVariable,
	UVector3dCameraVariable* FirstPersonLocationVariable,
	UVector3dCameraVariable* EyeLocationVariable,
	UVector3dCameraVariable* ADSLocationVariable,
	URotator3dCameraVariable* ADSRotationVariable)
{
	if (FirstPersonFactorVariable)
	{
		FirstPersonFactorVariableId = FirstPersonFactorVariable->GetVariableID();
	}
	if (TraceSphreRadiusVariable)
	{
		TraceSphreRadiusVariableId = TraceSphreRadiusVariable->GetVariableID();
	}

	auto Context = GameplayCameraComponent->GetEvaluationContext();
	if (!Context.IsValid())
	{
		return;
	}
	auto& VariableTable = Context->GetInitialResult().VariableTable;
	if (FirstPersonLocationVariable)
	{
		FirstPersonLocationVariableId = FirstPersonLocationVariable->GetVariableID();
		if (!VariableTable.ContainsValue(FirstPersonLocationVariableId))
		{
			VariableTable.AddVariable(FirstPersonLocationVariable->GetVariableDefinition());
			UE_LOG(LogGar, Log, TEXT("Added FirstPersonLocation variable to VariableTable: %s"), *FirstPersonLocationVariable->GetDisplayName());
		}
	}
	if (EyeLocationVariable)
	{
		EyeLocationVariableId = EyeLocationVariable->GetVariableID();
		if (!VariableTable.ContainsValue(EyeLocationVariableId))
		{
			VariableTable.AddVariable(EyeLocationVariable->GetVariableDefinition());
			UE_LOG(LogGar, Log, TEXT("Added EyeLocation variable to VariableTable: %s"), *EyeLocationVariable->GetDisplayName());
		}
	}
	if (ADSLocationVariable)
	{
		ADSLocationVariableId = ADSLocationVariable->GetVariableID();
		if (!VariableTable.ContainsValue(ADSLocationVariableId))
		{
			VariableTable.AddVariable(ADSLocationVariable->GetVariableDefinition());
			UE_LOG(LogGar, Log, TEXT("Added ADSLocation variable to VariableTable: %s"), *ADSLocationVariable->GetDisplayName());
		}
	}
	if (ADSRotationVariable)
	{
		ADSRotationVariableId = ADSRotationVariable->GetVariableID();
		if (!VariableTable.ContainsValue(ADSRotationVariableId))
		{
			VariableTable.AddVariable(ADSRotationVariable->GetVariableDefinition());
			UE_LOG(LogGar, Log, TEXT("Added ADSRotation variable to VariableTable: %s"), *ADSRotationVariable->GetDisplayName());
		}
	}
}

void UGarGameplayCameraStateComponent::BeginPlay()
{
	if(!ensure(IsValid(Settings))) return;
	Super::BeginPlay();

	PreviousShoulderMode = ShoulderMode = Settings->ThirdPerson.ShoulderMode;
	PreviousConfirmedDesiredPerspective = DesiredPerspective = Settings->DesiredPerspective;
	SetConfirmedDesiredViewMode(DesiredPerspective);
	Character->SetPerspective(Perspective == GarCameraPerspectiveTags::ThirdPerson ? GarPerspectiveTags::ThirdPerson : GarPerspectiveTags::FirstPerson);
}

void UGarGameplayCameraStateComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarGameplayCameraStateComponent::TickComponent()"), STAT_UGarGameplayCameraStateComponent_Tick, STATGROUP_Gar)

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(Settings) || !Character.IsValid() || !GameplayCameraComponent.IsValid())
	{
		return;
	}

	FirstPersonFactor = 0.0f;

	auto CameraSystemEvaluator = GameplayCameraComponent->GetCameraSystemEvaluator();
	if (CameraSystemEvaluator.IsValid())
	{
		const auto& Result = CameraSystemEvaluator->GetEvaluatedResult();
		CameraLocation = Result.CameraPose.GetLocation();
		CameraRotation = Result.CameraPose.GetRotation();

		auto* PlayerController{Cast<APlayerController>(Character->GetController())};
		if (GameplayCameraComponent.IsValid() && IsValid(PlayerController))
		{
			TanHalfVfov = FMath::Tan(FMath::DegreesToRadians(Result.CameraPose.GetEffectiveFieldOfView()) * 0.5f);
			if (Result.CameraPose.GetPanoramic())
			{
				int32 SizeX, SizeY;
				PlayerController->GetViewportSize(SizeX, SizeY);
				float AspectRatio{SizeX * (1 - Result.CameraPose.GetPanoramaSideViewRate() * 2 / 3) / (float)SizeY};
				TanHalfVfov /= AspectRatio;
			}
		}

		if(FirstPersonFactorVariableId.IsValid())
		{
			float OutValue;
			if (Result.VariableTable.TryGetValue<float>(FirstPersonFactorVariableId, OutValue))
			{
				FirstPersonFactor = UGarMath::Clamp01(OutValue);
			}
		}
		if(TraceSphreRadiusVariableId.IsValid())
		{
			float OutValue;
			if (Result.VariableTable.TryGetValue<float>(TraceSphreRadiusVariableId, OutValue))
			{
				TraceSphreRadius = FMath::Max(OutValue, 0.001f);
			}
		}
	}

#if ENABLE_DRAW_DEBUG
	const auto bDisplayDebugCameraTraces{
		UGarUtility::ShouldDisplayDebugForActor(Character.Get(), UGarCameraConstants::CameraTracesDebugDisplayName())
	};
#endif

	// Refresh desired view mode information.

	if (Character->IsLocallyControlled())
	{
		if (PerspectiveChangeBlockTime > 0.f)
		{
			PerspectiveChangeBlockTime -= DeltaTime;
		}
		else
		{
			if (DesiredPerspective != ConfirmedDesiredPerspective)
			{
				PerspectiveChangeBlockTime = Settings->PerspectiveChangeBlockTime;
			}
			SetConfirmedDesiredViewMode(DesiredPerspective);
		}
	}

	if (PreviousConfirmedDesiredPerspective != ConfirmedDesiredPerspective)
	{
		// Set aim point correction during change FPP/TPP
		auto FocusLocation{GetCurrentFocusLocation()};
		if (PreviousConfirmedDesiredPerspective == GarCameraPerspectiveTags::FirstPerson)
		{
			// FPP -> TPP
			auto TraceStart{GetThirdPersonTraceStartLocation()};
			auto FocalRotation{(FocusLocation - TraceStart).Rotation()};
			FocalRotation.Roll = Character->GetViewRotation().Roll;
			if (Settings->HeuristicPitchMapping && IsValid(Settings->HeuristicPitchMapping))
			{
				FocalRotation.Normalize();
				auto Mapped = FMath::Lerp(-180.0f, 180.0f, Settings->HeuristicPitchMapping->GetFloatValue((FocalRotation.Pitch + 180.0f) / 360.0f));
				//UE_LOG(LogTemp, Log, TEXT("%.2f -> %.2f"), FocalRotation.Pitch, Mapped);
				FocalRotation.Pitch = Mapped;
			}
			Character->SetFocalRotation(FocalRotation);
#if ENABLE_DRAW_DEBUG
			if (bDisplayDebugCameraTraces)
			{
				DrawDebugLine(GetWorld(), TraceStart, FocusLocation, FLinearColor{0.0f, 0.75f, 1.0f}.ToFColor(true),
					false, 3.0f, 0, UGarUtility::DrawLineThickness);
			}
#endif
		}
		else if(PreviousConfirmedDesiredPerspective == GarCameraPerspectiveTags::ThirdPerson)
		{
			// TPP -> FPP
			auto TraceStart{GetFirstPersonTraceStartLocation()};
			auto FocalRotation{(FocusLocation - TraceStart).Rotation()};
			FocalRotation.Roll = Character->GetViewRotation().Roll;
			if (Settings->HeuristicPitchMapping && IsValid(Settings->HeuristicPitchMapping))
			{
				FocalRotation.Normalize();
				auto Mapped = FMath::Lerp(-180.0f, 180.0f, Settings->HeuristicPitchMapping->GetFloatValue((FocalRotation.Pitch + 180.0f) / 360.0f));
				//UE_LOG(LogTemp, Log, TEXT("%.2f -> %.2f"), FocalRotation.Pitch, Mapped);
				FocalRotation.Pitch = Mapped;
			}
			Character->SetFocalRotation(FocalRotation);
#if ENABLE_DRAW_DEBUG
			if (bDisplayDebugCameraTraces)
			{
				DrawDebugLine(GetWorld(), TraceStart, FocusLocation, FLinearColor{0.75f, 0.0f, 1.0f}.ToFColor(true),
					false, 3.0f, 0, UGarUtility::DrawLineThickness);
			}
#endif
		}
		PreviousConfirmedDesiredPerspective = ConfirmedDesiredPerspective;
	}

	if (PreviousShoulderMode != ShoulderMode)
	{
		if (Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(GarPerspectiveTags::ThirdPerson) && bIsFocusPawn)
		{
			// Set aim point correction during change shoulder
			auto FocusLocation{GetCurrentFocusLocation()};
			auto NextCameraLocation{GetThirdPersonTraceStartLocation()};
			auto FocalRotation{(FocusLocation - NextCameraLocation).Rotation()};
			FocalRotation.Roll = Character->GetViewRotation().Roll;
			Character->SetFocalRotation(FocalRotation);
#if ENABLE_DRAW_DEBUG
			if (bDisplayDebugCameraTraces)
			{
				DrawDebugLine(GetWorld(), NextCameraLocation, FocusLocation, FLinearColor{0.0f, 0.75f, 1.0f}.ToFColor(true),
					false, 3.0f, 0, UGarUtility::DrawLineThickness);
			}
#endif
		}
		PreviousShoulderMode = ShoulderMode;
	}

	UpdatePerspective();
	UpdateFocalLength();

	Character->SetPerspective(FirstPersonFactor > Settings->FirstPerson.FirstPersonFactorThreshold ? GarPerspectiveTags::FirstPerson : GarPerspectiveTags::ThirdPerson);
}

void UGarGameplayCameraStateComponent::UpdateState(const float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarGameplayCameraStateComponent::Update()"), STAT_UGarGameplayCameraStateComponent_Update, STATGROUP_Gar)

	if (!IsValid(Settings) || !Character.IsValid() || !GameplayCameraComponent.IsValid())
	{
		return;
	}

	GameplayCameraComponent->SetRelativeLocation(FVector{0.0f, 0.0f, Character->BaseEyeHeight});

	auto Context = GameplayCameraComponent->GetEvaluationContext();
	if (!Context.IsValid())
	{
		return;
	}

	auto& VariableTable = Context->GetInitialResult().VariableTable;
	auto ControlRotation = Character->GetControlRotation();
	if (FirstPersonLocationVariableId.IsValid())
	{
		VariableTable.SetValue<FVector3d>(FirstPersonLocationVariableId, GetFirstPersonCameraLocation()
			- ControlRotation.Vector() * Settings->FirstPerson.RetreatDistance);
	}
	if (EyeLocationVariableId.IsValid())
	{
		VariableTable.SetValue<FVector3d>(EyeLocationVariableId, GetEyeCameraLocation()
			- ControlRotation.Vector() * Settings->FirstPerson.RetreatDistance);
	}

	if(Character->GetAimAmount() < Settings->FirstPerson.ADSThreshold)
	{
		bIsSightOffsetValid = false;
	}

	FVector Location;
	FRotator Rotation;
	if(Character->HasSight() && Character->GetAimAmount() >= Settings->FirstPerson.ADSThreshold
		&& Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(GarAimingModeTags::AimDownSight)
		&& !Character->GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Settings->FirstPerson.BlockSightViewTags))
	{
		Character->GetSightLocAndRot(Location, Rotation);
		Rotation.Roll = Character->GetControlRotation().Roll;
		auto ControlRotationInverse{ControlRotation.Quaternion().Inverse()};
		SightLocationOffset = ControlRotationInverse.RotateVector(Location - GetEyeCameraLocation());
		SightRotationOffset = Rotation.Quaternion() * ControlRotationInverse;
		bIsSightOffsetValid = true;
		Location = FVector::PointPlaneProject(Location, GetEyeCameraLocation(), Rotation.Vector())
			- Rotation.Vector() * Settings->FirstPerson.RetreatDistance;
	}
	else if(bIsSightOffsetValid)
	{
		Location = ControlRotation.RotateVector(SightLocationOffset) + GetEyeCameraLocation();
		Rotation = (ControlRotation.Quaternion() * SightRotationOffset).Rotator();
		Rotation.Roll = Character->GetControlRotation().Roll;
		Location = FVector::PointPlaneProject(Location, GetEyeCameraLocation(), Rotation.Vector())
			- Rotation.Vector() * Settings->FirstPerson.RetreatDistance;
	}
	else
	{
		Location = GetEyeCameraLocation() - ControlRotation.Vector() * Settings->FirstPerson.RetreatDistance;
		Rotation = ControlRotation;
	}

	if (ADSLocationVariableId.IsValid())
	{
		VariableTable.SetValue<FVector3d>(ADSLocationVariableId, Location);
	}
	if (ADSRotationVariableId.IsValid())
	{
		VariableTable.SetValue<FRotator3d>(ADSRotationVariableId, Rotation);
	}

	if (BoomOffsetVariableId.IsValid())
	{
		VariableTable.SetValue<FVector3d>(BoomOffsetVariableId, BoomOffset);
	}
	if (RightShoulderOffsetVariableId.IsValid())
	{
		VariableTable.SetValue<FVector3d>(RightShoulderOffsetVariableId, RightShoulderOffset);
	}
	if (CenterShoulderOffsetVariableId.IsValid())
	{
		VariableTable.SetValue<FVector3d>(CenterShoulderOffsetVariableId, CenterShoulderOffset);
	}
	if (LeftShoulderOffsetVariableId.IsValid())
	{
		VariableTable.SetValue<FVector3d>(LeftShoulderOffsetVariableId, LeftShoulderOffset);
	}
}

FVector UGarGameplayCameraStateComponent::GetFirstPersonCameraLocation() const
{
	return Character->GetMesh()->GetSocketLocation(Settings->FirstPerson.CameraSocketName);
}

FVector UGarGameplayCameraStateComponent::GetEyeCameraLocation() const
{
	return Character->GetMesh()->GetSocketLocation(Settings->FirstPerson.bLeftDominantEye
												   ? Settings->FirstPerson.LeftEyeCameraSocketName
												   : Settings->FirstPerson.RightEyeCameraSocketName);
}

FVector UGarGameplayCameraStateComponent::GetThirdPersonTraceStartLocation() const
{
	auto ShoulderOffset{
		ShoulderMode == GarCameraShoulderModeTags::Right ? RightShoulderOffset :
		ShoulderMode == GarCameraShoulderModeTags::Left ? LeftShoulderOffset :
		CenterShoulderOffset
	};
	return GameplayCameraComponent->GetComponentLocation() + CameraRotation.RotateVector(BoomOffset) + CameraRotation.RotateVector(ShoulderOffset);
}

FVector UGarGameplayCameraStateComponent::GetFirstPersonTraceStartLocation() const
{
	auto ViewRotation = Character->GetViewRotation();
	if (Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(GarAimingModeTags::AimDownSight))
	{
		if (bIsSightOffsetValid)
		{
			auto Location{ViewRotation.RotateVector(SightLocationOffset) + GetEyeCameraLocation()};
			auto Rotation{(ViewRotation.Quaternion() * SightRotationOffset).Rotator()};
			return FVector::PointPlaneProject(Location, GetEyeCameraLocation(), Rotation.Vector())
				- Rotation.Vector() * Settings->FirstPerson.RetreatDistance;
		}
		else
		{
			return GetEyeCameraLocation() - ViewRotation.Vector() * Settings->FirstPerson.RetreatDistance;
		}
	};
	return GetFirstPersonCameraLocation() - ViewRotation.Vector() * Settings->FirstPerson.RetreatDistance;
}

void UGarGameplayCameraStateComponent::UpdatePerspective()
{
	if (ConfirmedDesiredPerspective == GarCameraPerspectiveTags::FirstPerson)
	{
		Perspective = GarCameraPerspectiveTags::FirstPerson;
		return;
	}
	if (Settings->ThirdPerson.AutoFPPStartDistance <= 0.0f || Character->GetLocomotionAction().IsValid())
	{
		Perspective = GarCameraPerspectiveTags::ThirdPerson;
		return;
	}

	// Auto FPP processing

	static const FName MainTraceTag{FString::Printf(TEXT("%hs (Main Trace)"), __FUNCTION__)};
	auto TraceStart{GetThirdPersonTraceStartLocation()};
	auto TraceEnd{GetThirdPersonTraceStartLocation() - CameraRotation.Vector() * Settings->ThirdPerson.AutoFPPEndDistance};
	const auto CollisionShape{FCollisionShape::MakeSphere(TraceSphreRadius)};
	auto TraceResult{TraceEnd};
	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, Settings->ThirdPerson.TraceChannel, CollisionShape,
		{MainTraceTag, false, Character.Get()}))
	{
		if (!Hit.bStartPenetrating)
		{
			TraceResult = Hit.Location;
		}
		else
		{
			TraceResult = TraceStart;
		}
	}
#if ENABLE_DRAW_DEBUG
	if (UGarUtility::ShouldDisplayDebugForActor(Character.Get(), UGarCameraConstants::CameraTracesDebugDisplayName()))
	{
		UGarUtility::DrawDebugSweepSphere(GetWorld(), TraceStart, TraceResult, CollisionShape.GetCapsuleRadius(),
			Hit.IsValidBlockingHit() ? FLinearColor::Red : FLinearColor::Green);
	}
#endif
	auto Distance{FVector::Dist(TraceStart, TraceResult)};
	if (Perspective == GarCameraPerspectiveTags::FirstPerson && Distance > Settings->ThirdPerson.AutoFPPEndDistance)
	{
		Perspective = GarCameraPerspectiveTags::ThirdPerson;
	}
	else if(Perspective == GarCameraPerspectiveTags::ThirdPerson && Distance < Settings->ThirdPerson.AutoFPPStartDistance)
	{
		Perspective = GarCameraPerspectiveTags::FirstPerson;
	}
}

void UGarGameplayCameraStateComponent::UpdateFocalLength()
{
	static const FName MainTraceTag{FString::Printf(TEXT("%hs (Main Trace)"), __FUNCTION__)};
	const auto CollisionShape{FCollisionShape::MakeSphere(Settings->FocusTraceRadius)};

	auto EyeVec = CameraRotation.Vector();
	FVector TraceStart = CameraLocation + EyeVec * Settings->MinFocalLength;
	if (!Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(GarPerspectiveTags::FirstPerson))
	{
		auto ProjectedLocation = FVector::PointPlaneProject(CameraLocation, Character->GetActorLocation(), -EyeVec);
		auto ProjectedLocationDistance = FVector::Distance(TraceStart, ProjectedLocation);
		TraceStart = ProjectedLocation + EyeVec * FMath::Max(-ProjectedLocationDistance, Settings->ThirdPerson.FocusTraceStartOffset);
	}
	FVector TraceEnd{CameraLocation + EyeVec * Settings->MaxFocalLength};
	FVector TraceResult{TraceEnd};

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, Settings->FocusTraceChannel,
										 CollisionShape, {MainTraceTag, false, Character.Get()}))
	{
		TraceResult = Hit.Location;
		if (Hit.HasValidHitObjectHandle())
		{
			bIsFocusPawn = IsValid(Cast<APawn>(Hit.GetActor()));
		}
	}
	else
	{
		bIsFocusPawn = false;
	}

#if ENABLE_DRAW_DEBUG
	if (UGarUtility::ShouldDisplayDebugForActor(Character.Get(), UGarCameraConstants::CameraTracesDebugDisplayName()))
	{
		UGarUtility::DrawDebugSweepSphere(GetWorld(), TraceStart, TraceResult, CollisionShape.GetSphereRadius(),
			Hit.IsValidBlockingHit() ? FLinearColor::Red : FLinearColor::Green);
	}
#endif

	FocalLength = FMath::Max(Settings->MinFocalLength, FVector::Distance(TraceResult, CameraLocation));
}

void UGarGameplayCameraStateComponent::SetDesiredViewMode(const FGameplayTag& NewDesiredViewMode)
{
	DesiredPerspective = NewDesiredViewMode;
}

void UGarGameplayCameraStateComponent::SetConfirmedDesiredViewMode(const FGameplayTag& NewConfirmedDesiredViewMode)
{
	if (ConfirmedDesiredPerspective == NewConfirmedDesiredViewMode || Character->GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	ConfirmedDesiredPerspective = NewConfirmedDesiredViewMode;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ConfirmedDesiredPerspective, this)

	if (Character->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetConfirmedDesiredViewMode(NewConfirmedDesiredViewMode);
	}
}

void UGarGameplayCameraStateComponent::ServerSetConfirmedDesiredViewMode_Implementation(const FGameplayTag& NewViewMode)
{
	SetConfirmedDesiredViewMode(NewViewMode);
}

void UGarGameplayCameraStateComponent::SetShoulderMode(const FGameplayTag& NewShoulderMode)
{
	if (ShoulderMode == NewShoulderMode || Character->GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	ShoulderMode = NewShoulderMode;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ShoulderMode, this)

	if (Character->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetShoulderMode(NewShoulderMode);
	}
}

void UGarGameplayCameraStateComponent::ServerSetShoulderMode_Implementation(const FGameplayTag& NewShoulderMode)
{
	SetShoulderMode(NewShoulderMode);
}

void UGarGameplayCameraStateComponent::ToggleShoulder()
{
	if (ShoulderMode != GarCameraShoulderModeTags::Center)
	{
		SetShoulderMode(ShoulderMode == GarCameraShoulderModeTags::Right ? GarCameraShoulderModeTags::Left : GarCameraShoulderModeTags::Right);
	}
}
