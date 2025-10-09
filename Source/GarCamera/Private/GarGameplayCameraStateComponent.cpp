#include "GarGameplayCameraStateComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameplayCameraComponentBase.h"
#include "GameFramework/GameplayCamerasPlayerCameraManager.h"
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
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredPerspective, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredShoulderMode, Parameters)
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
	if (GameplayCameraComponent.IsValid())
	{
		auto Context = GameplayCameraComponent->GetEvaluationContext();
		if (Context && (PreviousController == nullptr || Context->GetPlayerController() == PreviousController))
		{
			GameplayCameraComponent->DeactivateCamera();
		}
	}
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

	PreviousPerspective = Perspective = DesiredPerspective = Settings->DesiredPerspective;
	PreviousShoulderMode = ShoulderMode = DesiredShoulderMode = Settings->ThirdPerson.DesiredShoulderMode;
	Character->SetPerspective(Perspective == GarCameraPerspectiveTags::ThirdPerson ? GarPerspectiveTags::ThirdPerson : GarPerspectiveTags::FirstPerson);
}

void UGarGameplayCameraStateComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGarGameplayCameraStateComponent::TickComponent()"), STAT_UGarGameplayCameraStateComponent_Tick, STATGROUP_Gar)

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(Settings) || !Character.IsValid() || !GameplayCameraComponent.IsValid() || !Character->IsLocallyControlled())
	{
		return;
	}

	FirstPersonFactor = 0.0f;

	auto CameraSystemEvaluator = GameplayCameraComponent->GetCameraSystemEvaluator();
	if (CameraSystemEvaluator.IsValid())
	{
		const auto& Result = CameraSystemEvaluator->GetEvaluatedResult();

		auto NewCameraLocation = Result.CameraPose.GetLocation();
		auto NewCameraRotation = Result.CameraPose.GetRotation();

		if (Character->GetRemoteRole() == ROLE_Authority
			&& !NewCameraLocation.Equals(CameraLocation, 0.001) || !NewCameraRotation.Equals(CameraRotation, 0.001))
		{
			ServerSetCameraLocRot(NewCameraLocation, NewCameraRotation);
		}

		CameraLocation = NewCameraLocation;
		CameraRotation = NewCameraRotation;

		auto* PlayerController{Cast<APlayerController>(Character->GetController())};
		if (IsValid(PlayerController))
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
	else
	{
		auto* PlayerController{Cast<APlayerController>(Character->GetController())};
		if (IsValid(PlayerController) && IsValid(PlayerController->PlayerCameraManager))
		{
			auto NewCameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
			auto NewCameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();

			if (Character->GetRemoteRole() == ROLE_Authority
				&& !NewCameraLocation.Equals(CameraLocation, 0.001) || !NewCameraRotation.Equals(CameraRotation, 0.001))
			{
				ServerSetCameraLocRot(NewCameraLocation, NewCameraRotation);
			}

			CameraLocation = NewCameraLocation;
			CameraRotation = NewCameraRotation;

			auto MinimalViewInfo{PlayerController->PlayerCameraManager->GetCameraCacheView()};
			TanHalfVfov = FMath::Tan(FMath::DegreesToRadians(MinimalViewInfo.PanoramaFOV) * 0.5f);
			if (MinimalViewInfo.bPanoramic)
			{
				int32 SizeX, SizeY;
				PlayerController->GetViewportSize(SizeX, SizeY);
				float AspectRatio{SizeX * (1 - MinimalViewInfo.PanoramaSideViewRate * 2 / 3) / (float)SizeY};
				TanHalfVfov /= AspectRatio;
			}

			auto* GameplayCamerasPlayerCameraManager{Cast<AGameplayCamerasPlayerCameraManager>(PlayerController->PlayerCameraManager)};
			if (GameplayCamerasPlayerCameraManager)
			{
				CameraSystemEvaluator = GameplayCamerasPlayerCameraManager->GetCameraSystemEvaluator();
				if (CameraSystemEvaluator.IsValid())
				{
					const auto& Result = CameraSystemEvaluator->GetEvaluatedResult();
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
			}
		}
	}

#if ENABLE_DRAW_DEBUG
	const auto bDisplayDebugCameraTraces{
		UGarUtility::ShouldDisplayDebugForActor(Character.Get(), UGarCameraConstants::CameraTracesDebugDisplayName())
	};
#endif

	if (PerspectiveChangeBlockTime > 0.f)
	{
		PerspectiveChangeBlockTime -= DeltaTime;
	}

	if (ShoulderChangeBlockTime > 0.f)
	{
		ShoulderChangeBlockTime -= DeltaTime;
	}

	bool bSkipCorrection = UpdatePerspectiveAndShoulderMode();
	UpdateFocalLength();

	if (PreviousPerspective != Perspective)
	{
		// Set aim point correction during change FPP/TPP
		if (PreviousPerspective == GarCameraPerspectiveTags::FirstPerson && !bSkipCorrection)
		{
			// FPP -> TPP
			auto FocusLocation{GetCurrentFocusLocation()};
			auto TraceStart{GetThirdPersonTraceStartLocation()};
			auto RotMax{FRotationMatrix::MakeFromXZ(FocusLocation - TraceStart, Character->GetViewRotation().RotateVector(FVector::UpVector))};
			auto FocalRotation{RotMax.Rotator()};
			Character->SetFocalRotation(FocalRotation);
#if ENABLE_DRAW_DEBUG
			if (bDisplayDebugCameraTraces)
			{
				DrawDebugLine(GetWorld(), TraceStart, FocusLocation, FLinearColor{0.0f, 0.75f, 1.0f}.ToFColor(true),
					false, 3.0f, 0, UGarUtility::DrawLineThickness);
			}
#endif
		}
		else if(PreviousPerspective == GarCameraPerspectiveTags::ThirdPerson && !bSkipCorrection)
		{
			// TPP -> FPP
			auto FocusLocation{GetCurrentFocusLocation()};
			auto TraceStart{GetFirstPersonTraceStartLocation()};
			auto RotMax{FRotationMatrix::MakeFromXZ(FocusLocation - TraceStart, Character->GetViewRotation().RotateVector(FVector::UpVector))};
			auto FocalRotation{RotMax.Rotator()};
			Character->SetFocalRotation(FocalRotation);
#if ENABLE_DRAW_DEBUG
			if (bDisplayDebugCameraTraces)
			{
				DrawDebugLine(GetWorld(), TraceStart, FocusLocation, FLinearColor{0.75f, 0.0f, 1.0f}.ToFColor(true),
					false, 3.0f, 0, UGarUtility::DrawLineThickness);
			}
#endif
		}
		PerspectiveChangeBlockTime = Settings->PerspectiveChangeBlockTime;
		PreviousPerspective = Perspective;
	}

	if (PreviousShoulderMode != ShoulderMode)
	{
		if (Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(GarPerspectiveTags::ThirdPerson)
			&& (!Settings->ThirdPerson.bAllowFocusingPawnOnly || bIsFocusPawn))
		{
			// Set aim point correction during change shoulder
			auto FocusLocation{GetCurrentFocusLocation()};
			auto NextCameraLocation{GetThirdPersonTraceStartLocation()};
			auto RotMax{FRotationMatrix::MakeFromXZ(FocusLocation - NextCameraLocation, Character->GetViewRotation().RotateVector(FVector::UpVector))};
			auto FocalRotation{RotMax.Rotator()};
			Character->SetFocalRotation(FocalRotation);
#if ENABLE_DRAW_DEBUG
			if (bDisplayDebugCameraTraces)
			{
				DrawDebugLine(GetWorld(), NextCameraLocation, FocusLocation, FLinearColor{0.0f, 0.75f, 1.0f}.ToFColor(true),
					false, 3.0f, 0, UGarUtility::DrawLineThickness);
			}
#endif
		}
		ShoulderChangeBlockTime = Settings->ShoulderChangeBlockTime;
		PreviousShoulderMode = ShoulderMode;
	}

	Character->SetPerspective(FirstPersonFactor > Settings->FirstPerson.FirstPersonFactorThreshold
		? GarPerspectiveTags::FirstPerson : GarPerspectiveTags::ThirdPerson);
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
		auto RotMax{FRotationMatrix::MakeFromXZ(Rotation.Vector(), Character->GetViewRotation().RotateVector(FVector::UpVector))};
		Rotation = RotMax.Rotator();
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
		auto RotMax{FRotationMatrix::MakeFromXZ(Rotation.Vector(), Character->GetViewRotation().RotateVector(FVector::UpVector))};
		Rotation = RotMax.Rotator();
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

void UGarGameplayCameraStateComponent::ServerSetCameraLocRot_Implementation(const FVector& NewCameraLocation, const FRotator& NewCameraRotation)
{
	CameraLocation = NewCameraLocation;
	CameraRotation = NewCameraRotation;
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
	return GetThirdPersonTraceStartLocation(ShoulderMode);
}

FVector UGarGameplayCameraStateComponent::GetThirdPersonTraceStartLocation(const FGameplayTag& InShoulderMode) const
{
	return GameplayCameraComponent->GetComponentLocation() + CameraRotation.RotateVector(BoomOffset) + CameraRotation.RotateVector(
		InShoulderMode == GarCameraShoulderModeTags::Right ? RightShoulderOffset :
		InShoulderMode == GarCameraShoulderModeTags::Left ? LeftShoulderOffset :
		CenterShoulderOffset
	);
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

bool UGarGameplayCameraStateComponent::UpdatePerspectiveAndShoulderMode()
{
	bool bForceTPP{Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(GarCameraStatusTags::ForceTPP)};
	if (DesiredPerspective == GarCameraPerspectiveTags::FirstPerson && !bForceTPP)
	{
		if (PerspectiveChangeBlockTime <= 0.0f)
		{
			Perspective = GarCameraPerspectiveTags::FirstPerson;
		}
		if (ShoulderChangeBlockTime <= 0.0f)
		{
			ShoulderMode = DesiredShoulderMode;
		}
		return false;
	}

	if (!Settings->ThirdPerson.bAllowAutoShoulderSwitching && (Settings->ThirdPerson.AutoFPPStartDistance <= 0.0f || bForceTPP))
	{
		if (PerspectiveChangeBlockTime <= 0.0f)
		{
			Perspective = GarCameraPerspectiveTags::ThirdPerson;
		}
		if (ShoulderChangeBlockTime <= 0.0f)
		{
			ShoulderMode = DesiredShoulderMode;
		}
		return false;
	}

	// Auto FPP processing

	static const FName MainTraceTag{FString::Printf(TEXT("%hs (Main Trace)"), __FUNCTION__)};
	bool bSkipCorrection = false;
	if (PerspectiveChangeBlockTime <= 0.0f)
	{
		double Distance{BIG_NUMBER};
		auto TraceStart{GetThirdPersonTraceStartLocation(DesiredShoulderMode)};
		auto TraceEnd{TraceStart - CameraRotation.Vector() * Settings->ThirdPerson.AutoFPPEndDistance};
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
			Distance = FVector::Dist(TraceStart, TraceResult);
		}
#if ENABLE_DRAW_DEBUG
		if (UGarUtility::ShouldDisplayDebugForActor(Character.Get(), UGarCameraConstants::CameraTracesDebugDisplayName()))
		{
			UGarUtility::DrawDebugSweepSphere(GetWorld(), TraceStart, TraceResult, CollisionShape.GetCapsuleRadius(),
				Hit.IsValidBlockingHit() ? FLinearColor::Red : FLinearColor::Green);
		}
#endif
		if(PreviousPerspective == GarCameraPerspectiveTags::ThirdPerson
			&& Settings->ThirdPerson.AutoFPPStartDistance > 0.f && Distance < Settings->ThirdPerson.AutoFPPStartDistance)
		{
			Perspective = GarCameraPerspectiveTags::FirstPerson;
			bSkipCorrection = Perspective != DesiredPerspective;
		}
		if (PreviousPerspective == GarCameraPerspectiveTags::FirstPerson
			&& Settings->ThirdPerson.AutoFPPStartDistance > 0.f && Distance > Settings->ThirdPerson.AutoFPPEndDistance)
		{
			Perspective = GarCameraPerspectiveTags::ThirdPerson;
			bSkipCorrection = Perspective != DesiredPerspective;
		}
	}

	// Auto shoulder switch processing

	if (Perspective == GarCameraPerspectiveTags::FirstPerson && DesiredShoulderMode != GarCameraShoulderModeTags::Center
		&& Settings->ThirdPerson.bAllowAutoShoulderSwitching)
	{
		auto Distance{BIG_NUMBER};
		auto TraceStart{GetThirdPersonTraceStartLocation(DesiredShoulderMode == GarCameraShoulderModeTags::Right
			? GarCameraShoulderModeTags::Left : GarCameraShoulderModeTags::Right)};
		auto TraceEnd{TraceStart - CameraRotation.Vector() * Settings->ThirdPerson.AutoFPPEndDistance};
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
			Distance = FVector::Dist(TraceStart, TraceResult);
		}
#if ENABLE_DRAW_DEBUG
		if (UGarUtility::ShouldDisplayDebugForActor(Character.Get(), UGarCameraConstants::CameraTracesDebugDisplayName()))
		{
			UGarUtility::DrawDebugSweepSphere(GetWorld(), TraceStart, TraceResult, CollisionShape.GetCapsuleRadius(),
				Hit.IsValidBlockingHit() ? FLinearColor::Red : FLinearColor::Green);
		}
#endif
		if(PreviousPerspective == GarCameraPerspectiveTags::ThirdPerson
			&& Settings->ThirdPerson.AutoFPPStartDistance > 0.f && Distance >= Settings->ThirdPerson.AutoFPPStartDistance)
		{
			Perspective = GarCameraPerspectiveTags::ThirdPerson;
			if (ShoulderChangeBlockTime <= 0.0f)
			{
				ShoulderMode = DesiredShoulderMode == GarCameraShoulderModeTags::Left ? GarCameraShoulderModeTags::Right : GarCameraShoulderModeTags::Left;
				if (Settings->ThirdPerson.bAllowPermanentSwitching)
				{
					SetDesiredShoulderMode(ShoulderMode);
				}
			}
		}
		else if(ShoulderChangeBlockTime <= 0.0f)
		{
			ShoulderMode = DesiredShoulderMode;
		}
	}
	else if (ShoulderChangeBlockTime <= 0.0f)
	{
		ShoulderMode = DesiredShoulderMode;
	}

	return bSkipCorrection;
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

void UGarGameplayCameraStateComponent::SetDesiredPerspective(const FGameplayTag& NewDesiredPerspective)
{
	if (DesiredPerspective == NewDesiredPerspective || Character->GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	DesiredPerspective = NewDesiredPerspective;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredPerspective, this)

	if (Character->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetDesiredPerspective(NewDesiredPerspective);
	}
}

void UGarGameplayCameraStateComponent::ServerSetDesiredPerspective_Implementation(const FGameplayTag& NewDesiredPerspective)
{
	SetDesiredPerspective(NewDesiredPerspective);
}

void UGarGameplayCameraStateComponent::SetDesiredShoulderMode(const FGameplayTag& NewDesiredShoulderMode)
{
	if (DesiredShoulderMode == NewDesiredShoulderMode || Character->GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	DesiredShoulderMode = NewDesiredShoulderMode;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredShoulderMode, this)

	if (Character->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetDesiredShoulderMode(NewDesiredShoulderMode);
	}
}

void UGarGameplayCameraStateComponent::ServerSetDesiredShoulderMode_Implementation(const FGameplayTag& NewDesiredShoulderMode)
{
	SetDesiredShoulderMode(NewDesiredShoulderMode);
}

void UGarGameplayCameraStateComponent::ToggleShoulder()
{
	if (ShoulderMode != GarCameraShoulderModeTags::Center)
	{
		SetDesiredShoulderMode(DesiredShoulderMode == GarCameraShoulderModeTags::Right ? GarCameraShoulderModeTags::Left : GarCameraShoulderModeTags::Right);
	}
}
