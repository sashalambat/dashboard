@echo off
setlocal
cd /d "%~dp0"
echo SBS Wars — Windows
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Packaging\windows\Start-SBSWars.ps1"
if errorlevel 1 (
  echo.
  echo If the game did not start, install:
  echo   1. Visual Studio 2022 with Desktop C++ workload
  echo   2. CMake
  echo   3. vcpkg, then: vcpkg install sdl2 sdl2-mixer sdl2-ttf
  echo See Docs\BUILD_GUIDE.md
  pause
)
