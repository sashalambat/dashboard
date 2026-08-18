# AGENTS.md

## Cursor Cloud specific instructions

SBS Wars is a C++17 / CMake + SDL2 LAN FPS. The playable, buildable target in this
environment is the native "SBS Engine" (the Unreal project under `Source/SBSWars` is
Windows-only and is not built here). Standard build/run/test commands live in
`README.md`, `Docs/BUILD_GUIDE.md`, and `Scripts/build.sh` — use those. The system
dependencies (`libsdl2-dev`, `libsdl2-mixer-dev`, `libsdl2-ttf-dev`, `cmake`, `g++`)
are installed by the environment update script, so they should already be present.

Non-obvious caveats discovered during setup:

- Compiler: the default `/usr/bin/c++` is Clang and fails to link with
  `cannot find -lstdc++`. Build with GCC instead. The simplest path is
  `bash Scripts/build.sh`, which already forces `-DCMAKE_CXX_COMPILER=g++`. A bare
  `cmake -S . -B build` (no compiler override) will fail at the compiler check.
- Lint: there is no separate linter. CMake compiles with `-Wall -Wextra -Wpedantic
  -Werror` (option `SBS_WARNINGS_AS_ERRORS`, ON by default), so a clean build is the
  lint gate. Relax with `-DSBS_WARNINGS_AS_ERRORS=OFF` only for experimentation.
- Tests: `ctest --test-dir build --output-on-failure` (or run `./Binaries/sbswars-tests`
  directly). The suite is headless and fast (66 checks). `Scripts/build.sh` runs it.
- Display: the GUI client (`sbswars`) and `sbswars-launcher` need an X display. This VM
  provides `DISPLAY=:1` — prefix GUI runs with `DISPLAY=:1`. `sbswars-server` and
  `sbswars-tests` are headless and need no display.
- Audio: there is no sound device, so SDL_mixer logs ALSA "Couldn't open audio device"
  warnings on client startup. This is harmless; the game falls back and keeps running.
- LAN discovery: the client's "LAN Browser" uses UDP broadcast, which does not reach a
  local `sbswars-server` inside this single-host container (no real broadcast domain),
  and the GUI client has no connect-by-IP option. For interactive play use
  "Host Listen Server" or "Offline vs Bots" from the main menu. The automated tests
  exercise the discovery/protocol path directly over `127.0.0.1` and pass.
- Match length: on stock balance, Team Deathmatch (the mode started by the menu) reaches
  the score limit within a few seconds because AI bots score quickly, so a
  listen/offline match ends fast and jumps to the "MATCH RESULTS" scoreboard. This is
  expected behavior, not a crash.
- Output locations: build tree is `build/`; executables are written to `Binaries/`.
