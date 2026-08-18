@echo off
setlocal
cd /d "%~dp0"
echo Installing SBS Wars and creating a Desktop shortcut...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Packaging\windows\install.ps1"
if errorlevel 1 (
  echo Install failed. See Docs\BUILD_GUIDE.md
  pause
  exit /b 1
)
echo.
echo Done. Double-click "SBS Wars" on your Desktop to play.
pause
