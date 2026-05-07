# Physical Animation & Ragdolling

## Overview

`UGarPhysicalAnimationComponent` extends `UPhysicalAnimationComponent` and manages:

- Physics profile switching based on GAS gameplay tags (via Chooser).
- Per-bone `PhysicsBlendWeight` animation with configurable blend times.
- Full ragdoll simulation state (`FGarRagdollingState`).

### Required Project Settings

- `Project Settings > Physics > Enable Physics Prediction` — ON (for multiplayer)
- `Project Settings > Physics > Substepping` — ON
- `Project Settings > Physics > Substepping Async` — ON (optional)

---

## UGarPhysicalAnimationComponent

### Key Properties

| Property | Description |
| --- | --- |
| `ProfileChooser` | Chooser table that selects physics profiles from gameplay tags |
| `CurveBoneMappings` | Maps animation curve names to bone names for blend weight locking |
| `BlendTimeOfBlendWeightOnActivate` | Time (s) to blend physics weight from 0 → 1 on activate |
| `BlendTimeOfBlendWeightOnDeactivate` | Time (s) to blend physics weight from 1 → 0 on deactivate |
| `TopBoneName` | The root bone of the physics simulation (e.g. `pelvis`) |
| `RagdollingSettingsMap` | Map from `FGameplayTag` to `UGarRagdollingSettings` |

### Profile Selection

Each tick, `NeedsProfileChange()` compares the current and previous tag containers.  
If they differ, `SelectProfile()` calls the Chooser and applies `ApplyPhysicalAnimationProfileBelow` + `SetConstraintProfileForAll` for each selected profile name.

### Physics Blend Weight

`RefreshBodyState()` iterates all bodies each tick:

- Bodies matching an active profile **and** not locked by a curve → simulate physics, blend weight → 1 over `BlendTimeOfBlendWeightOnActivate`.
- Bodies locked by a curve value → blend weight toward `1 - curveValue` at 15 cm/s.
- All others → blend weight → 0 over `BlendTimeOfBlendWeightOnDeactivate`. Physics disabled when weight reaches 0.

---

## FGarRagdollingState

Tracks all transient ragdoll state. Lives inside `UGarPhysicalAnimationComponent`.

| Field | Description |
| --- | --- |
| `bGrounded` | True when the capsule trace detects walkable ground |
| `bFacingUpward` | True when the ragdoll pelvis is facing upward (supine) |
| `bFreezing` | True once the ragdoll has settled; physics simulation stops |
| `ElapsedTime` | Seconds since ragdoll started |
| `LyingDownYawAngleDelta` | Yaw offset used to align the character on get-up |

### Lifecycle

| Method | Trigger |
| --- | --- |
| `Start()` | Called when a ragdoll gameplay tag is first applied |
| `Tick()` | Called every frame while ragdolling |
| `End()` | Called when the ragdoll tag is removed; queues a `FTeleportEffect` to realign the character |

### Freeze Conditions (`UGarRagdollingSettings`)

| Setting | Description |
| --- | --- |
| `TimeAfterGroundedForForceFreezing` | Force-freeze after this many seconds on the ground |
| `TimeAfterGroundedAndStoppedForForceFreezing` | Force-freeze after stopped + grounded for this long |
| `SpeedThresholdToFreeze` | Max bone speed (cm/s) to consider the ragdoll stopped |
| `AngularSpeedThresholdToFreeze` | Max bone angular speed (deg/s) to consider the ragdoll stopped |

### bFacingUpward Detection

Determined every frame from the top bone (`TopBoneName`):

- When the bone's forward vector is nearly parallel to the world up vector (`|dot| > 0.7`), the **right** vector is used to compare against the character's forward vector.
- Otherwise the bone's forward vector is compared directly.

---

## FGarPhysicalAnimationCurveValues

Helper struct that reads named animation curves from `UGarAnimationInstance` each tick and caches their values.  
Used by `RefreshBodyState()` to lock bones while a curve is non-zero (e.g. hand IK weight curves).

## Related

- [Character](Character.md)
- [Mover](Mover.md)
- [Gameplay Abilities](GameplayAbilities.md)
- [Animation](Animation.md)
