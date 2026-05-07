# Overlay System

## Overview

The overlay system is an ALS-inspired modular layering architecture implemented entirely in C++ using GAS.  
Three independent component types manage different kinds of animation layer overrides on `AGarCharacter`.

---

## Components

### UGarOverlayModeComponent

Manages **overlay layers** — typically weapon/item-in-hands pose blending (e.g. pistol, rifle, bow).

- Maintains a `TMap<FGameplayTag, TSubclassOf<UGarOverlayTask>>` registry.
- Activates/deactivates `UGarOverlayTask` instances based on the currently active overlay GAS ability.
- Dynamically links/unlinks `UGarOverlayAnimInstance` subclasses into the main Anim Graph.

```cpp
// Register an overlay in C++ or Blueprint
OverlayModeComponent->RegisterOverlayTask(OverlayTag, UMyOverlayTask::StaticClass());
```

### UGarDeltaOverlayModeComponent

Manages **delta overlay layers** — additive corrections layered on top of the base overlay.  
Useful for small pose adjustments (e.g. different grip when aiming).

- Same task/tag registry pattern as `UGarOverlayModeComponent`.
- Links `UGarDeltaOverlayAnimInstance` subclasses.

### UGarOverrideModeComponent

Manages **override layers** — full upper-body replacement animations (e.g. full aim pose, interaction animations).

- Links `UGarOverrideAnimInstance` subclasses.

---

## GAS Abilities

| Class | Role |
| --- | --- |
| `UGarGameplayAbility_OverlayMode` | Activates an overlay mode; keeps alive until cancelled |
| `UGarGameplayAbility_DeltaOverlay` | Activates a delta overlay; keeps alive until cancelled |

---

## Gameplay Tags

Overlay identity is expressed entirely with `FGameplayTag`.  
Define your own overlay tags under the `Gar.Overlay.*` hierarchy and register them with the component.

---

## Workflow

1. Define a `FGameplayTag` for each overlay mode.
2. Create a `UGarOverlayTask` Blueprint subclass per overlay.
3. Call `RegisterOverlayTask()` on `UGarOverlayModeComponent` (e.g. in `BeginPlay` of a subclass or a GameFeature action).
4. Grant `UGarGameplayAbility_OverlayMode` with the matching tag to the character's GAS component.
5. Activate the ability to switch overlays at runtime.

## Related

- [Animation](Animation.md)
- [Gameplay Abilities](GameplayAbilities.md)
- [Character](Character.md)
