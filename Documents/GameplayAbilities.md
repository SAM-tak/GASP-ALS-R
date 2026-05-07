# Gameplay Abilities

## Overview

All character actions are implemented as GAS (`UGameplayAbility`) subclasses.  
The base class hierarchy is:

```text
UGameplayAbility
└── UGarGameplayAbility            — input binding, montage stop on end
    ├── UGarGameplayAbility_Action         — one-shot actions with cooldown support
    │   ├── UGarGameplayAbility_MontageBase    — plays a montage via Mover AnimRootMotion
    │   │   ├── UGarGameplayAbility_Montage        — generic montage ability
    │   │   ├── UGarGameplayAbility_MotionMatchBase — uses Chooser/MotionMatching to select montage
    │   │   │   ├── UGarGameplayAbility_Traversal
    │   │   │   ├── UGarGameplayAbility_Rolling
    │   │   │   ├── UGarGameplayAbility_Sliding
    │   │   │   └── UGarGameplayAbility_Landing
    │   └── UGarGameplayAbility_Ragdolling
    ├── UGarGameplayAbility_OverlayMode    — persistent overlay layer control
    └── UGarGameplayAbility_DeltaOverlay   — persistent delta overlay control
```

---

## UGarGameplayAbility

Base class for all GAR abilities.

| Property | Description |
| --- | --- |
| `bEnableInputBinding` | If true, binds input actions on activate and unbinds on end (default: true) |
| `bStopCurrentMontageOnEndAbility` | Stops the playing montage when the ability ends |
| `OverrideBlendOutTimeOnEndAbility` | Overrides blend-out time on end (`< 0` = use montage default) |

---

## UGarGameplayAbility_Traversal

Traversal (climb / vault / hurdle / mantle) ability using MotionMatching montage selection and MotionWarping.

### Key Features

- **`TryActivateTraversal()`** — static helper called from Blueprint instead of `TryActivateAbilityByClass`.  
  Runs the trace and MotionMatch selection locally on the client, packages the result into `FGarTraversalTargetData`, and sends it to the server via `HandleGameplayEvent`.
- **`FGarTraversalTargetData`** — `FGameplayAbilityTargetData` subclass with `NetSerialize` support for replicating traversal parameters (selected montage, ledge locations, normals, etc.).
- **`TraversalEventTag`** — The gameplay event tag used to trigger this ability. Default: `Gar.Event.Traversal`.
- Warp targets: `FrontLedge`, `BackLedge`, `BackFloor` — updated each frame during the traversal.

### Chooser Inputs (`FGarTraversalChooserInputs`)

| Field | Description |
| --- | --- |
| `bHasFrontLedge` | Front ledge detected |
| `bHasBackLedge` | Back ledge detected |
| `bHasBackFloor` | Floor behind obstacle detected |
| `ObstacleHeight` | Height of the obstacle (cm) |
| `ObstacleDepth` | Depth of the obstacle (cm) |
| `Speed` | Character speed at activation (cm/s) |
| `CurrentGameplayTags` | Current GAS tags for filtering |

---

## UGarGameplayAbility_Ragdolling

Activates ragdoll physics on the character.  
Works together with `UGarPhysicalAnimationComponent` and `GarMoverRagdollingMode`.

---

## UGarGameplayAbility_Sliding

Sliding action with MotionMatching montage selection.  
Uses `GarMoverSlidingMode` during execution.

---

## UGarGameplayAbility_Rolling

Roll dodge with MotionMatching montage selection.

---

## UGarGameplayAbility_Landing

Hard landing reaction with MotionMatching montage selection.

---

## GarAbilitySet

`UGarAbilitySet` is a data asset that batches ability grants (abilities + effects + attribute sets).  
Assign to the character's `AbilitySet` property to auto-grant on `BeginPlay`.

---

## UGarAbilitySystemComponent

Extends `UAbilitySystemComponent` with helpers used throughout the plugin.  
Handles loose gameplay tag replication for stance, gait, and rotation mode states.

## Related

- [Character](Character.md)
- [Mover](Mover.md)
- [Overlay System](OverlaySystem.md)
- [Physical Animation & Ragdolling](PhysicalAnimationRagdolling.md)
