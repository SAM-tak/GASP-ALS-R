# Mover

## Overview

`UGarCharacterMoverComponent` extends UE5's `UMoverComponent` and manages all movement simulation for `AGarCharacter`.  
Movement logic is split into **Mover Modes** (one active at a time) and **Mover Modifiers** (zero or more, stacked on top).

## Replicated State

| Property | Type | Description |
| --- | --- | --- |
| `RotationMode` | `FGameplayTag` | Current rotation mode tag |
| `Stance` | `FGameplayTag` | Current stance tag |
| `Gait` | `FGameplayTag` | Current gait tag |
| `bFacingUpward` | `bool` | Whether the ragdoll is facing upward (replicated) |

## Settings

| Property | Description |
| --- | --- |
| `LocomotionModeTags` | Tags that map to locomotion modes |
| `bTeleportPhysicsOnProxy` | When true, physics teleport is applied on simulated proxies |

---

## Mover Modes

Each mode derives from `FLayeredMoveBase` / `UMoverDataSourceBase` and handles a specific locomotion state.

| Class | Mode |
| --- | --- |
| `GarMoverWalkingMode` | Standard walking / running on ground |
| `GarMoverFallingMode` | Airborne (falling / jumping) |
| `GarMoverSlidingMode` | Sliding on the ground |
| `GarMoverTraversalMode` | Traversal action (climb/vault/etc.) |
| `GarMoverRagdollingMode` | Ragdoll physics-driven movement |

---

## Mover Modifiers

Modifiers adjust movement parameters without replacing the active mode.  
Base class: `FGarMoverModifier`.

| Class | Effect |
| --- | --- |
| `FGarMoverGaitModifier` | Adjusts speed caps for the current gait |
| `FGarMoverRotationModifier` | Overrides rotation behaviour |
| `FGarMoverCrouchingModifier` | Applies crouching movement constraints |
| `FGarMoverLyingModifier` | Applies lying/prone movement constraints |
| `FGarMoverStanceModifier` | Base for stance-based modifiers |

---

## Constants

```cpp
static constexpr float MIN_FLOOR_DIST = 1.9f;  // cm above walkable floor
static constexpr float MAX_FLOOR_DIST = 2.4f;  // cm above walkable floor
```

## Related

- [Character](Character.md)
- [Gameplay Abilities](GameplayAbilities.md)
- [Physical Animation & Ragdolling](PhysicalAnimationRagdolling.md)
