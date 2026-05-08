# Game Animation Sample with Modular Overlay Layering and GAS

## Introduction

This project takes [GASPALS](https://github.com/PolygonHive/GASPALS) as its foundation, replaces the ALS portion with a layering system inspired by [ALS-Refactored](https://github.com/Sixze/ALS-Refactored), rewrites all GASP-originated code in C++, and rebuilds the entire implementation using GAS (Gameplay Ability System) and Modular Gameplay.

## Features

- Game Animation Sample
- Modular Overlay layering system built with separate Anim Graphs and Linked Layers and Pawn Component
- works with Mover
- works with GameplayCamera
- Overlay system as Gameplay Ability
- Travarsal Action as Gameplay Ability
- Sliding Action as Gameplay Ability
- Rolling Action as Gameplay Ability
- Ragdolling as Gameplay Ability
- All overlays from ALS
- Basic weapon attach system from ALS
- Basic overlay switcher widget from ALS
- Removed Echo and Twinblast characters and Manny/Quinn 4k textures to lower project size
- Removed Legacy character based on CharacterMovementComponent

## Documentation

- [Character](Documents/Character.md) — `AGarCharacter`, components, stance API
- [Mover](Documents/Mover.md) — `UGarCharacterMoverComponent`, modes, modifiers
- [Animation](Documents/Animation.md) — `UGarAnimationInstance`, linked layers
- [Overlay System](Documents/OverlaySystem.md) — overlay / delta-overlay / override components and abilities
- [Gameplay Abilities](Documents/GameplayAbilities.md) — GAS ability hierarchy, traversal, ragdolling, sliding
- [Physical Animation & Ragdolling](Documents/PhysicalAnimationRagdolling.md) — `UGarPhysicalAnimationComponent`, ragdoll state machine
- [Camera](Documents/Camera.md) — `GarCamera` module, `UGarGameplayCameraStateComponent`
- [Gameplay Tags](Documents/GameplayTags.md) — full tag namespace reference

## Migrating Plugin

To migrate the GASP-ALS-R plugin to your own Unreal Engine project, you can follow these steps:

1. Copy the `GASP-ALS-R` folder to your project's `Plugins` folder and build.
2. In **Project Settings > GameplayTags > Gameplay Tag Table List**, add `/GAR/Example/Core/Data/DT_GarExtra_GameplayTags`.
3. Launch the project, an error message will prompt you to add collision settings, click add at the end of the message.
4. Enjoy :)

## Contributing

Contributions are welcome! I hope that, with the help of the community, we can turn this into a next-gen fully featured locomotion system.

Please follow these steps to contribute:

1. Clone the repository.
2. Create a new branch (`git checkout -b feature/your-feature`).
3. Make your changes.
4. Commit your changes (`git commit -m 'Add some feature'`).
5. Push to the branch (`git push origin feature/your-feature`).
6. Open a pull request.

Please ensure your code follows the project's coding and naming standards.

## License

[UE-Only Content - Licensed for Use Only with Unreal Engine-based Products](https://www.unrealengine.com/en-US/eula/content)

However, the following exceptions apply to source code files:

- **Original C++ source code** in this project and source code derived from **ALS-Refactored** are licensed under the [MIT License](LICENSE-for-cpp-source-codes).
- Source code files derived from **Unreal Engine source** are individually marked with a comment to that effect at the top of the file; those files follow the terms of the Unreal Engine EULA instead of the MIT License.
