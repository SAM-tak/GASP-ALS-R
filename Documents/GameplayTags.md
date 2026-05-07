# Gameplay Tags

## Overview

All state in GASP-ALS-R is communicated through `FGameplayTag`s defined in `GarGameplayTags.h` (and `GarCameraGameplayTags.h` for the camera module).  
Tags are registered via `UE_DEFINE_GAMEPLAY_TAG` in `GarGameplayTags.cpp` and follow the `Gar.*` hierarchy.

---

## Tag Namespaces

### Desired State (player intent, set by input / abilities)

| Namespace | Tags | Path |
| --- | --- | --- |
| `GarDesiredRotationModeTags` | `VelocityDirection`, `ViewDirection` | `Gar.Desired.RotationMode.*` |
| `GarDesiredStanceTags` | `Standing`, `Crouching`, `LyingFront`, `LyingBack` | `Gar.Desired.Stance.*` |
| `GarDesiredGaitTags` | `Walking`, `Running`, `Sprinting` | `Gar.Desired.Gait.*` |

> `Lying` is kept for backward compatibility but `LyingFront` / `LyingBack` are the canonical tags.

---

### Locomotion State (actual current state, loose tags on GAS)

| Namespace | Tags | Path |
| --- | --- | --- |
| `GarLocomotionModeTags` | `Grounded`, `InAir` | `Gar.LocomotionMode.*` |
| `GarRotationModeTags` | `VelocityDirection`, `ViewDirection`, `Aiming` | `Gar.RotationMode.*` |
| `GarStanceTags` | `Standing`, `Crouching`, `LyingFront`, `LyingBack` | `Gar.Stance.*` |
| `GarGaitTags` | `Walking`, `Running`, `Sprinting` | `Gar.Gait.*` |
| `GarPerspectiveTags` | `FirstPerson`, `ThirdPerson` | `Gar.Perspective.*` |

---

### Action Tags (applied for the duration of an action ability)

| Namespace | Tags | Path |
| --- | --- | --- |
| `GarLocomotionActionTags` | `Rolling`, `Landing`, `Traversal`, `GettingUp`, `GettingDown`, `FreeFalling`, `Unconsious`, `Dying`, `Sliding` | `Gar.LocomotionAction.*` |
| `GarTraversalActionTags` | `Vault`, `Hurdle`, `Mantle` | `Gar.TraversalAction.*` |
| `GarSlidingActionTags` | `KneesOut` | `Gar.SlidingAction.*` |

---

### Event Tags

| Namespace | Tags | Description |
| --- | --- | --- |
| `GarEventTags` | `Traversal` | Gameplay event used to trigger traversal ability remotely |

---

### State Flag Tags (transient flags, loose tags)

| Namespace | Tags | Description |
| --- | --- | --- |
| `GarStateFlagTags` | `RotationLocked`, `BlockUpdateCapsuleSize`, `FacingUpward` | Internal state flags |

---

### Aiming Mode Tags

| Namespace | Tags | Path |
| --- | --- | --- |
| `GarAimingModeTags` | `AimDownSight`, `HipFire`, `Firing` | `Gar.AimingMode.*` |

---

## Adding Custom Tags

Declare in a header:

```cpp
namespace MyTags
{
    GAR_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MyCustomTag)
}
```

Define in a `.cpp`:

```cpp
namespace MyTags
{
    UE_DEFINE_GAMEPLAY_TAG(MyCustomTag, "Gar.MyCategory.MyCustomTag")
}
```

Register the DataTable containing your tags in **Project Settings > GameplayTags > Gameplay Tag Table List**.

## Related

- [Character](Character.md)
- [Gameplay Abilities](GameplayAbilities.md)
- [Camera](Camera.md)
