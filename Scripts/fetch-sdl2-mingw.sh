#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/ThirdParty/sdl2-mingw"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cd "$TMP"
curl -fsSL -o SDL2.tar.gz https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/SDL2-devel-2.30.11-mingw.tar.gz
curl -fsSL -o SDL2_mixer.tar.gz https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/SDL2_mixer-devel-2.8.0-mingw.tar.gz
curl -fsSL -o SDL2_ttf.tar.gz https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-devel-2.22.0-mingw.tar.gz
tar -xzf SDL2.tar.gz
tar -xzf SDL2_mixer.tar.gz
tar -xzf SDL2_ttf.tar.gz
rm -rf "$DEST"
mkdir -p "$DEST"
cp -a SDL2-*/x86_64-w64-mingw32/. "$DEST/"
cp -a SDL2_mixer-*/x86_64-w64-mingw32/. "$DEST/"
cp -a SDL2_ttf-*/x86_64-w64-mingw32/. "$DEST/"
echo "SDL2 MinGW SDK installed at $DEST"
