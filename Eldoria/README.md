# Eldoria

Eldoria is a C# 3D fantasy RPG prototype designed with a low-spec desktop target in mind.

## Current playable systems

- Procedural terrain streamed in chunks
- Third-person player movement
- Player level, XP, health and attack progression
- Inventory/items
- Enemy combat
- Quest progress
- Automatic and manual save/load
- Windows x64 build through GitHub Actions
- Legacy Intel macOS build target

## Controls

- WASD: move
- Shift: sprint
- Space: attack nearby enemy
- F5: save
- Escape: quit

## Build

The project is intentionally kept as normal C# source in `Eldoria/`, rather than hiding the game code inside GitHub Actions. The workflow packages the source and produces build artifacts.

The macOS legacy target is experimental because modern CI runners do not run macOS 10.13. The produced binary must be tested on the target iMac.
