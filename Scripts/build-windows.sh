#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
SDL="${SBS_MINGW_SDL:-$ROOT/ThirdParty/sdl2-mingw}"
if [[ ! -d "$SDL" ]]; then
    echo "SDL2 MinGW SDK missing. Run Scripts/fetch-sdl2-mingw.sh first." >&2
    exit 1
fi
cmake -S . -B build-win \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/mingw-w64.cmake" \
    -DSBS_MINGW_SDL="$SDL" \
    -DCMAKE_PREFIX_PATH="$SDL"
cmake --build build-win -j"$(nproc)"
python3 Packaging/linux/generate_icon.py
OUT="$ROOT/Dist/windows"
rm -rf "$OUT"
mkdir -p "$OUT/Maps" "$OUT/Content" "$OUT/Audio" "$OUT/UI"
BIN="$ROOT/Binaries/windows"
cp -a "$BIN/SBSWars.exe" "$BIN/SBSWarsLauncher.exe" "$BIN/SBSWarsServer.exe" "$BIN/SBSWarsInstaller.exe" "$OUT/"
cp -a "$SDL/bin/SDL2.dll" "$SDL/bin/SDL2_mixer.dll" "$OUT/"
for cand in \
    /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll \
    /usr/lib/gcc/x86_64-w64-mingw32/13-posix/libwinpthread-1.dll \
    /usr/lib/gcc/x86_64-w64-mingw32/13-win32/libwinpthread-1.dll; do
    if [[ -f "$cand" ]]; then
        cp -a "$cand" "$OUT/"
        break
    fi
done
p="$(x86_64-w64-mingw32-g++-posix -print-file-name=libwinpthread-1.dll 2>/dev/null || true)"
if [[ -n "$p" && -f "$p" ]]; then
    cp -a "$p" "$OUT/"
fi
cp -a "$ROOT/Maps/." "$OUT/Maps/"
cp -a "$ROOT/Content/." "$OUT/Content/"
cp -a "$ROOT/Audio/." "$OUT/Audio/"
cp -a "$ROOT/UI/." "$OUT/UI/"
cp -f "$ROOT/Packaging/windows/sbswars.ico" "$OUT/"
cp -f "$ROOT/Packaging/windows/install.ps1" "$OUT/"
cat > "$OUT/Install-SBSWars.bat" << 'EOF'
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
EOF
cat > "$OUT/Play-SBSWars.bat" << 'EOF'
@echo off
cd /d "%~dp0"
start "" "%~dp0SBSWarsLauncher.exe"
EOF
echo "Windows payload: $OUT"
du -sh "$OUT"
ls -lh "$OUT"
