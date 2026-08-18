# SBS Wars — Build Guide

## Windows — play now

From a cloned repo on Windows:

1. Double-click `Install-SBSWars.bat`
2. Double-click **SBS Wars** on the Desktop

Or double-click `start.bat` to install and launch in one step.

That copies `Dist\windows\` into `%LOCALAPPDATA%\SBSWars` and creates Desktop + Start Menu shortcuts.

Play without installing:

```bat
Dist\windows\Play-SBSWars.bat
```

### Build on Windows (Visual Studio + vcpkg)

```bat
vcpkg install sdl2 sdl2-mixer sdl2-ttf
cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build-win --config Release
Install-SBSWars.bat
```

Needs Visual Studio 2022 (Desktop C++) and CMake.

### Cross-compile Windows .exe from Linux

```bash
sudo apt-get install -y mingw-w64 cmake
Scripts/fetch-sdl2-mingw.sh
Scripts/build-windows.sh
```

Output: `Dist/windows/` (exes + SDL DLLs). Optional Inno Setup: compile `Packaging/windows/SBSWars.iss`.

## Linux

```bash
sudo apt-get install -y build-essential cmake libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build -j
./Binaries/sbswars-tests
./Binaries/sbswars-launcher
```

```bash
Scripts/build.sh
Packaging/linux/make_installer.sh
bash Dist/SBSWarsInstaller.run
```

Creates `~/Desktop/SBS Wars.desktop`.

### Client flags
`--offline` `--listen` `--dedicated` `--join host:port` `--name` `--session` `--bots N` `--max N` `--port N` `--headless`

```bash
./Binaries/sbswars-server --port 27015 --bots 16 --session "Foundry Night"
```

## Unreal Engine 5.6 (optional)
1. Install UE 5.6 and Visual Studio 2022.
2. Right-click `SBSWars.uproject` → Generate project files.
3. Build `SBSWars` / `SBSWarsServer` and package Windows 64-bit.

## Tests
`Binaries/sbswars-tests` (Linux) or `Binaries/windows/sbswars-tests.exe` (cross-build) must exit 0.
