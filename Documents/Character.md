# Character

## Overview

`AGarCharacter` is the central actor class of this plugin. It is a `APawn` subclass that implements `IAbilitySystemInterface`, `IGameplayCueInterface`, `IGameplayTagAssetInterface`, and `IMoverInputProducerInterface`.

It owns and coordinates all major subsystems:

| Component | Class | Role |
| --- | --- | --- |
| Capsule | `UCapsuleComponent` | Primary collision capsule |
| ProneCapsule | `UCapsuleComponent` | Alternative capsule used when prone |
| Mesh | `USkeletalMeshComponent` | Main skeletal mesh |
| CharacterMover | `UGarCharacterMoverComponent` | Mover-based movement |
| MotionWarping | `UMotionWarpingComponent` | Motion warping for abilities |
| AbilitySystem | `UGarAbilitySystemComponent` | GAS ability system |
| PhysicalAnimation | `UGarPhysicalAnimationComponent` | Physical animation / ragdolling |
| OverlayModeComponent | `UGarOverlayModeComponent` | Overlay layer management |
| DeltaOverlayModeComponent | `UGarDeltaOverlayModeComponent` | Delta overlay layer management |
| OverrideModeComponent | `UGarOverrideModeComponent` | Override anim layer management |

## Stance API

| Method | Description |
| --- | --- |
| `Crouch()` | Transition to Crouching stance |
| `UnCrouch()` | Transition to Standing stance |
| `Prone()` | Transition to LyingFront (prone) stance |
| `Supine()` | Transition to LyingBack (supine) stance |
| `IsCrouching()` | Returns true when Crouching |
| `CanLie()` | Returns true when lying is permitted |

## Input Stance vs Desired Stance

`SetInputStance()` is the internal setter used by abilities and the ragdoll system.  
`SetDesiredStance()` / `ApplyDesiredStance()` is the player-intent layer that feeds `InputStance` based on desired tags and current capability checks.

## Events (C++ delegates)

| Delegate | Signature |
| --- | --- |
| `OnPossessorChanged` | `(AController*)` |
| `OnSetupPlayerInputComponent` | `(UInputComponent*)` |
| `OnTick` | `(float DeltaTime)` |
| `OnChangeGameplayTag` | `(const FGameplayTag&)` |

## Settings Asset

`UGarCharacterSettings` contains the desired-to-actual tag mapping table and other per-character tuning values.  
Assign it in the Blueprint default `Settings` property.

## Related

- [Mover](Mover.md)
- [Overlay System](OverlaySystem.md)
- [Physical Animation & Ragdolling](PhysicalAnimationRagdolling.md)
- [Gameplay Abilities](GameplayAbilities.md)
