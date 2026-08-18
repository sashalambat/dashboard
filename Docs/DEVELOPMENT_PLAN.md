# SBS Wars — Development Plan

## Engine decision
Unreal Engine 5.6 is the target visual pipeline (Lumen/Nanite content in `Content/`). This environment does not include the Unreal Editor, so the **shipping compile target** is the native C++ SBS Engine (SDL2 software renderer + UDP LAN). Gameplay systems are engine-agnostic (`Source/Game`) and map 1:1 onto Unreal classes in `Source/SBSWars`.

## Workstreams
1. Core simulation — World, weapons, classes, modes
2. AI — navgrid, combat, objectives, difficulty
3. Networking — LAN discover, listen/dedicated, snapshots, input
4. Client — renderer, HUD, menus, audio
5. Persistence — settings and stats
6. Maps & content — six layouts, procedural textures, synthesized SFX
7. QA — headless tests, host/join, regression after each compile
8. Delivery — launcher + installer

## Tick
60 Hz simulation. 20 Hz snapshot send. Client prediction on local listen/offline.

## Performance
- Low quality: 320×180 internal, 60 FPS class target
- High quality: 480×270 internal, 120 FPS cap available
CPU sim is cheap; the software renderer is the GPU/CPU cost center and scales with quality.

## DoD
A change is done when `sbswars-tests` passes and `cmake --build` reports zero errors.
