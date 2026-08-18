# SBS Wars

LAN-only sci-fi military first-person shooter for Windows PC LAN parties.

SBS Alliance vs Cyber Dominion. 2–64 players, listen or dedicated servers, AI bots, five classes, nine weapons, six maps, and classic objective modes. No accounts, matchmaking, or internet services.

## Quick start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./Binaries/sbswars-tests
./Binaries/sbswars-launcher
```

| Binary | Role |
| --- | --- |
| `sbswars-launcher` | Application launcher |
| `sbswars` | Game client / listen server |
| `sbswars-server` | Dedicated LAN server |
| `sbswars-tests` | Automated simulation suite |

```bash
./Binaries/sbswars --offline
./Binaries/sbswars --listen
./Binaries/sbswars-server --port 27015 --bots 16
```

## Packaging

```bash
Scripts/build.sh
Packaging/linux/make_installer.sh
```

The installer payload is written to `Dist/`. See [Docs/BUILD_GUIDE.md](Docs/BUILD_GUIDE.md).

Unreal Engine 5.6 project files (`SBSWars.uproject`, `Source/SBSWars`) are included for a Windows high-fidelity pipeline. The playable shipping target in this repository is the native SBS Engine, which compiles without the Unreal Editor.
