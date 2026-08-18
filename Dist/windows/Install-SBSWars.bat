@echo off
cd /d "%~dp0"
echo Installing SBS Wars and creating a Desktop shortcut...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" -Payload "%~dp0"
if errorlevel 1 (
  echo Install failed.
  pause
  exit /b 1
)
echo.
echo Done. Double-click "SBS Wars" on your Desktop to play.
pause
