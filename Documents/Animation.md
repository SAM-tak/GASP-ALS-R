# Animation

## Overview

The animation system is structured as a main `AnimInstance` plus multiple **Linked Animation Layers**, each responsible for a specific concern.  
All state is driven by `FGameplayTag` containers read from `AGarCharacter` via `UGarCharacterMoverComponent`.

---

## UGarAnimationInstance

The primary `UAnimInstance` subclass assigned to `AGarCharacter`'s skeletal mesh.

### Key State Properties

| Property | Type | Description |
| --- | --- | --- |
| `CurrentGameplayTags` | `FGameplayTagContainer` | Tags from GAS, updated every frame |
| `CharacterMovement` | `FGarCharacterMovementState` | Velocity, acceleration, floor info, etc. |
| `PoseState` | `FGarPoseState` | Foot IK, spine lean, etc. |
| `ViewYawAngle` | `float` | Camera yaw offset (deg) |
| `ViewPitchAngle` | `float` | Camera pitch offset (deg) |
| `SpineYawAngle` | `float` | Spine aim yaw (deg) |
| `IdleAdditiveAmount` | `float` | 0–1 blend weight for idle additive layer |
| `bIsActionRunning` | `bool` | True while an action ability is active |

### Linked Instance References

| Property | Class |
| --- | --- |
| `LayeringAnimInstance` | `UGarLayeringAnimInstance` |
| `RagdollingAnimInstance` | `UGarRagdollingAnimInstance` |

---

## Linked Animation Layers

| Class | Role |
| --- | --- |
| `UGarLayeringAnimInstance` | Base layering graph; blends all overlay and override layers |
| `UGarOverlayAnimInstance` | ALS-style overlay (weapon/item in hands) |
| `UGarDeltaOverlayAnimInstance` | Delta overlay — additive correction on top of overlay |
| `UGarOverrideAnimInstance` | Full override of upper body (e.g. aiming, interacting) |
| `UGarRagdollingAnimInstance` | Ragdoll blend-in/out and freeze pose snapshot |
| `UGarRagdollingOverrideAnimInstance` | Override applied during ragdoll recovery |

All linked instances extend `UGarLinkedAnimationInstance` which provides helper utilities and a reference back to `UGarAnimationInstance`.

---

## Animation Blueprint Setup

1. Assign `UGarAnimationInstance` (or a Blueprint subclass) as the Anim Class of the skeletal mesh.
2. Link the layering and ragdolling instances via the Linked Anim Layer nodes inside the main Anim Graph.
3. Overlay and delta-overlay instances are dynamically linked/unlinked at runtime by `UGarOverlayModeComponent` and `UGarDeltaOverlayModeComponent`.

## Related

- [Overlay System](OverlaySystem.md)
- [Physical Animation & Ragdolling](PhysicalAnimationRagdolling.md)
- [Character](Character.md)
