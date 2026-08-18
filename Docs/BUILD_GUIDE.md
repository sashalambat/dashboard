# SBS Wars — Build Guide

## Native engine (this repo, Linux/Windows)

Dependencies: C++17 compiler, CMake 3.16+, SDL2, SDL2_mixer, SDL2_ttf.

```bash
sudo apt-get install -y build-essential cmake libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Binaries land in `Binaries/`.

### Client flags
`--offline` `--listen` `--dedicated` `--join host:port` `--name` `--session` `--bots N` `--max N` `--port N` `--headless`

### Dedicated
```bash
./Binaries/sbswars-server --port 27015 --bots 16 --session "Foundry Night"
```

## Installer
```bash
Scripts/build.sh
Packaging/linux/make_installer.sh
```
Produces `Dist/SBSWarsInstaller.run` and copies `SBSWarsLauncher`.

Windows: compile with MSVC + vcpkg SDL2, then compile `Packaging/windows/SBSWars.iss` with Inno Setup.

## Unreal Engine 5.6 (optional high-fidelity)
1. Install UE 5.6 and Visual Studio 2022.
2. Right-click `SBSWars.uproject` → Generate project files.
3. Build target `SBSWarsEditor` then `SBSWars` / `SBSWarsServer`.
4. Package Windows 64-bit from the editor (File → Package Project).
Gameplay C++ in `Source/SBSWars` mirrors native types (factions, classes, modes). Map `.umap` assets are placeholders until authored in-editor.

## Tests
`Binaries/sbswars-tests` runs map load, nav, weapons, modes, spectator, save, protocol, and dedicated host simulation. Must exit 0.
