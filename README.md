# SBS Wars

LAN-only sci-fi military first-person shooter for Windows PC LAN parties.

SBS Alliance vs Cyber Dominion. 2–64 players, listen or dedicated servers, AI bots, five classes, nine weapons, six maps, and classic objective modes. No accounts, matchmaking, or internet services.

## Start on Windows

1. Clone or download this repo.
2. Double-click **`Install-SBSWars.bat`**
3. Double-click the **SBS Wars** icon on your Desktop.

Or double-click **`start.bat`** to install (if needed) and launch.

Installer files:

| File | What it does |
| --- | --- |
| `Install-SBSWars.bat` | Copies the game to `%LOCALAPPDATA%\SBSWars` and creates a Desktop + Start Menu shortcut |
| `start.bat` | Installs if needed, then starts the launcher |
| `Dist\windows\SBSWarsLauncher.exe` | Game launcher (after a Windows build) |

If Windows Defender asks, allow the local app. The game is LAN-only and does not use the internet.

### Build on Windows (Visual Studio)

```bat
git clone https://github.com/sashalambat/dashboard.git
cd dashboard
git checkout cursor/sbs-wars-5bb3
vcpkg install sdl2 sdl2-mixer sdl2-ttf
cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build-win --config Release
Install-SBSWars.bat
```

You need: Visual Studio 2022 (Desktop C++), CMake, and [vcpkg](https://vcpkg.io).

## Start on Linux

```bash
bash Dist/SBSWarsInstaller.run
```

Or build:

```bash
sudo apt-get install -y build-essential cmake libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build -j
./Binaries/sbswars-launcher
./Binaries/sbswars --offline
```

| Binary | Role |
| --- | --- |
| `SBSWarsLauncher` / `sbswars-launcher` | Application launcher |
| `SBSWars` / `sbswars` | Game client / listen server |
| `SBSWarsServer` / `sbswars-server` | Dedicated LAN server |

Unreal Engine 5.6 project files (`SBSWars.uproject`) are optional. The playable game is the native engine.
