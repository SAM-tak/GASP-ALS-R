#include "Notifies/GarAnimNotify_FootstepEffects.h"

#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"
#include "AbilitySystemComponent.h"
#include "GarCharacter.h"
#include "GarConstants.h"
#include "Settings/GarBoneNameTable.h"
#include "Utility/GarEnumUtility.h"
#include "Utility/GarMath.h"
#include "Utility/GarUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarAnimNotify_FootstepEffects)

#if WITH_EDITOR
void FGarFootstepEffectSettings::PostEditChangeProperty(const FPropertyChangedEvent& PropertyChangedEvent)
{
	DecalFootLeftRotationOffsetQuaternion = DecalFootLeftRotationOffset.Quaternion();
	DecalFootRightRotationOffsetQuaternion = DecalFootRightRotationOffset.Quaternion();
	ParticleSystemFootLeftRotationOffsetQuaternion = ParticleSystemFootLeftRotationOffset.Quaternion();
	ParticleSystemFootRightRotationOffsetQuaternion = ParticleSystemFootRightRotationOffset.Quaternion();
}

void UGarFootstepEffectsSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, DecalSpawnAngleThreshold))
	{
		DecalSpawnAngleThresholdCos = FMath::Cos(FMath::DegreesToRadians(DecalSpawnAngleThreshold));
	}
	else if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, Effects))
	{
		for (auto& Tuple : Effects)
		{
			Tuple.Value.PostEditChangeProperty(PropertyChangedEvent);
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

FString UGarAnimNotify_FootstepEffects::GetNotifyName_Implementation() const
{
	TStringBuilder<64> NotifyNameBuilder{InPlace, TEXTVIEW("Gar Footstep Effects: "), GarEnumUtility::GetNameStringByValue(FootBone)};

	return FString{NotifyNameBuilder};
}

void UGarAnimNotify_FootstepEffects::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
                                            const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(Mesh, Animation, EventReference);

	if (!IsValid(Mesh) || !ensure(IsValid(FootstepEffectsSettings)))
	{
		return;
	}

	const auto* Character{Cast<AGarCharacter>(Mesh->GetOwner())};

	if (bSkipEffectsWhenInAir && IsValid(Character) && Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(GarLocomotionModeTags::InAir))
	{
		return;
	}

#if ENABLE_DRAW_DEBUG
	const auto bDisplayDebug{UGarUtility::ShouldDisplayDebugForActor(Mesh->GetOwner(), UGarConstants::TracesDebugDisplayName())};
#endif

	const auto* World{Mesh->GetWorld()};
	const auto MeshScale{Mesh->GetComponentScale().Z};

	const auto& FootBoneName{FootBone == EGarFootBone::Left
		? FootstepEffectsSettings->BoneNameTable->FootLeftBoneName
		: FootstepEffectsSettings->BoneNameTable->FootRightBoneName};
	const auto FootTransform{Mesh->GetSocketTransform(FootBoneName)};

	const auto FootZAxis{
		FootTransform.TransformVectorNoScale(FootBone == EGarFootBone::Left
			                                     ? FVector{FootstepEffectsSettings->FootLeftZAxis}
			                                     : FVector{FootstepEffectsSettings->FootRightZAxis})
	};

	FCollisionQueryParams QueryParameters{__FUNCTION__, true, Mesh->GetOwner()};
	QueryParameters.bReturnPhysicalMaterial = true;

	FHitResult FootstepHit;
	if (!World->LineTraceSingleByChannel(FootstepHit, FootTransform.GetLocation(),
	                                     FootTransform.GetLocation() - FootZAxis *
	                                     (FootstepEffectsSettings->SurfaceTraceDistance * MeshScale),
	                                     FootstepEffectsSettings->SurfaceTraceChannel, QueryParameters))
	{
		// As a fallback, trace down the world Z axis if the first trace didn't hit anything.

		World->LineTraceSingleByChannel(FootstepHit, FootTransform.GetLocation(),
		                                FootTransform.GetLocation() - FVector{
			                                0.0f, 0.0f, FootstepEffectsSettings->SurfaceTraceDistance * MeshScale
		                                }, FootstepEffectsSettings->SurfaceTraceChannel, QueryParameters);
	}

#if ENABLE_DRAW_DEBUG
	if (bDisplayDebug)
	{
		UGarUtility::DrawDebugLineTraceSingle(World, FootstepHit.TraceStart, FootstepHit.TraceEnd, FootstepHit.bBlockingHit,
		                                      FootstepHit, {0.333333f, 0.0f, 0.0f}, FLinearColor::Red, 10.0f);
	}
#endif

	if (!FootstepHit.bBlockingHit)
	{
		return;
	}

	const auto SurfaceType{FootstepHit.PhysMaterial.IsValid() ? FootstepHit.PhysMaterial->SurfaceType.GetValue() : SurfaceType_Default};
	const auto* EffectSettings{FootstepEffectsSettings->Effects.Find(SurfaceType)};

	if (EffectSettings == nullptr)
	{
		for (const auto& Tuple : FootstepEffectsSettings->Effects)
		{
			EffectSettings = &Tuple.Value;
			break;
		}

		if (EffectSettings == nullptr)
		{
			return;
		}
	}

	const auto FootstepLocation{FootstepHit.ImpactPoint};

	const auto FootstepRotation{
		FRotationMatrix::MakeFromZY(FootstepHit.ImpactNormal,
		                            FootTransform.TransformVectorNoScale(FootBone == EGarFootBone::Left
			                                                                 ? FVector{FootstepEffectsSettings->FootLeftYAxis}
			                                                                 : FVector{FootstepEffectsSettings->FootRightYAxis})).ToQuat()
	};

#if ENABLE_DRAW_DEBUG
	if (bDisplayDebug)
	{
		DrawDebugCoordinateSystem(World, FootstepLocation, FootstepRotation.Rotator(),
		                          25.0f, false, 10.0f, 0, UGarUtility::DrawLineThickness);
	}
#endif

	if (bSpawnSound)
	{
		SpawnSound(Mesh, *EffectSettings, FootstepLocation, FootstepRotation);
	}

	if (bSpawnDecal)
	{
		SpawnDecal(Mesh, *EffectSettings, FootstepLocation, FootstepRotation, FootstepHit, FootZAxis);
	}

	if (bSpawnParticleSystem)
	{
		SpawnParticleSystem(Mesh, *EffectSettings, FootstepLocation, FootstepRotation);
	}
}

#if WITH_EDITOR
void UGarAnimNotify_FootstepEffects::OnAnimNotifyCreatedInEditor(FAnimNotifyEvent& ContainingAnimNotifyEvent)
{
	ContainingAnimNotifyEvent.bTriggerOnDedicatedServer = false;
}
#endif

void UGarAnimNotify_FootstepEffects::SpawnSound(USkeletalMeshComponent* Mesh, const FGarFootstepEffectSettings& EffectSettings,
                                                const FVector& FootstepLocation, const FQuat& FootstepRotation) const
{
	auto VolumeMultiplier{SoundVolumeMultiplier};

	if (!bIgnoreFootstepSoundBlockCurve && IsValid(Mesh->GetAnimInstance()))
	{
		VolumeMultiplier *= 1.0f - UGarMath::Clamp01(Mesh->GetAnimInstance()->GetCurveValue(UGarConstants::FootstepSoundBlockCurveName()));
	}

	if (!FAnimWeight::IsRelevant(VolumeMultiplier) || !IsValid(EffectSettings.Sound.LoadSynchronous()))
	{
		return;
	}

	UAudioComponent* Audio{nullptr};

	if (EffectSettings.SoundSpawnMode == EGarFootstepSoundSpawnMode::SpawnAtTraceHitLocation)
	{
		const auto* World{Mesh->GetWorld()};

		if (World->WorldType == EWorldType::EditorPreview)
		{
			UGameplayStatics::PlaySoundAtLocation(World, EffectSettings.Sound.Get(), FootstepLocation,
			                                      VolumeMultiplier, SoundPitchMultiplier);
		}
		else
		{
			Audio = UGameplayStatics::SpawnSoundAtLocation(World, EffectSettings.Sound.Get(), FootstepLocation,
			                                               FootstepRotation.Rotator(),
			                                               VolumeMultiplier, SoundPitchMultiplier);
		}
	}
	else if (EffectSettings.SoundSpawnMode == EGarFootstepSoundSpawnMode::SpawnAttachedToFootBone)
	{
		const auto& FootBoneName{
			FootBone == EGarFootBone::Left
				? FootstepEffectsSettings->BoneNameTable->FootLeftBoneName
				: FootstepEffectsSettings->BoneNameTable->FootRightBoneName
		};

		Audio = UGameplayStatics::SpawnSoundAttached(EffectSettings.Sound.Get(), Mesh, FootBoneName, FVector::ZeroVector,
		                                             FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
		                                             true, VolumeMultiplier, SoundPitchMultiplier);
	}

	if (IsValid(Audio))
	{
		Audio->SetIntParameter(FName{TEXTVIEW("FootstepType")}, static_cast<int32>(SoundType));
	}
}

void UGarAnimNotify_FootstepEffects::SpawnDecal(USkeletalMeshComponent* Mesh, const FGarFootstepEffectSettings& EffectSettings,
                                                const FVector& FootstepLocation, const FQuat& FootstepRotation,
                                                const FHitResult& FootstepHit, const FVector& FootZAxis) const
{
	if ((FootstepHit.ImpactNormal | FootZAxis) < FootstepEffectsSettings->DecalSpawnAngleThresholdCos)
	{
		return;
	}

	if (!IsValid(EffectSettings.DecalMaterial.LoadSynchronous()))
	{
		return;
	}

	const auto DecalRotation{
		FootstepRotation * FQuat{
			FootBone == EGarFootBone::Left
				? EffectSettings.DecalFootLeftRotationOffsetQuaternion
				: EffectSettings.DecalFootRightRotationOffsetQuaternion
		}
	};

	const auto MeshScale{Mesh->GetComponentScale().Z};

	const auto DecalLocation{
		FootstepLocation + DecalRotation.RotateVector(FVector{EffectSettings.DecalLocationOffset} * MeshScale)
	};

	UDecalComponent* Decal{nullptr};

	if (EffectSettings.DecalSpawnMode == EGarFootstepDecalSpawnMode::SpawnAtTraceHitLocation || !FootstepHit.Component.IsValid())
	{
		Decal = UGameplayStatics::SpawnDecalAtLocation(Mesh->GetWorld(), EffectSettings.DecalMaterial.Get(),
		                                               FVector{EffectSettings.DecalSize} * MeshScale,
		                                               DecalLocation, DecalRotation.Rotator());
	}
	else if (EffectSettings.DecalSpawnMode == EGarFootstepDecalSpawnMode::SpawnAttachedToTraceHitComponent)
	{
		Decal = UGameplayStatics::SpawnDecalAttached(EffectSettings.DecalMaterial.Get(),
		                                             FVector{EffectSettings.DecalSize} * MeshScale,
		                                             FootstepHit.Component.Get(), NAME_None, DecalLocation,
		                                             DecalRotation.Rotator(), EAttachLocation::KeepWorldPosition);
	}

	if (IsValid(Decal))
	{
		Decal->SetFadeOut(EffectSettings.DecalDuration, EffectSettings.DecalFadeOutDuration, false);
	}
}

void UGarAnimNotify_FootstepEffects::SpawnParticleSystem(USkeletalMeshComponent* Mesh, const FGarFootstepEffectSettings& EffectSettings,
                                                         const FVector& FootstepLocation, const FQuat& FootstepRotation) const
{
	if (!IsValid(EffectSettings.ParticleSystem.LoadSynchronous()))
	{
		return;
	}

	const auto MeshScale{Mesh->GetComponentScale().Z};

	if (EffectSettings.ParticleSystemSpawnMode == EGarFootstepParticleEffectSpawnMode::SpawnAtTraceHitLocation)
	{
		const auto ParticleSystemRotation{
			FootstepRotation * FQuat{
				FootBone == EGarFootBone::Left
					? EffectSettings.ParticleSystemFootLeftRotationOffsetQuaternion
					: EffectSettings.ParticleSystemFootRightRotationOffsetQuaternion
			}
		};

		const auto ParticleSystemLocation{
			FootstepLocation +
			ParticleSystemRotation.RotateVector(FVector{EffectSettings.ParticleSystemLocationOffset} * MeshScale)
		};

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(Mesh->GetWorld(), EffectSettings.ParticleSystem.Get(),
		                                               ParticleSystemLocation, ParticleSystemRotation.Rotator(),
		                                               FVector::OneVector * MeshScale, true, true, ENCPoolMethod::AutoRelease);
	}
	else if (EffectSettings.ParticleSystemSpawnMode == EGarFootstepParticleEffectSpawnMode::SpawnAttachedToFootBone)
	{
		const auto& FootBoneName{FootBone == EGarFootBone::Left
			? FootstepEffectsSettings->BoneNameTable->FootLeftBoneName
			: FootstepEffectsSettings->BoneNameTable->FootRightBoneName};

		UNiagaraFunctionLibrary::SpawnSystemAttached(EffectSettings.ParticleSystem.Get(), Mesh, FootBoneName,
		                                             FVector{EffectSettings.ParticleSystemLocationOffset} * MeshScale,
		                                             FRotator{
			                                             FootBone == EGarFootBone::Left
				                                             ? EffectSettings.ParticleSystemFootLeftRotationOffset
				                                             : EffectSettings.ParticleSystemFootRightRotationOffset
		                                             },
		                                             FVector::OneVector * MeshScale, EAttachLocation::KeepRelativeOffset,
		                                             true, ENCPoolMethod::AutoRelease);
	}
}
