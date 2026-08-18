#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

python3 Packaging/linux/generate_icon.py

if [[ ! -x Binaries/sbswars || ! -x Binaries/sbswars-launcher || ! -x Binaries/sbswars-installer ]]; then
  Scripts/build.sh
fi

DIST="$ROOT/Dist"
PAYLOAD="$DIST/payload"
rm -rf "$PAYLOAD"
mkdir -p "$PAYLOAD/bin" \
         "$PAYLOAD/share/sbswars" \
         "$PAYLOAD/share/icons/hicolor/256x256/apps" \
         "$DIST"

cp -a Binaries/sbswars Binaries/sbswars-server Binaries/sbswars-launcher Binaries/sbswars-installer "$PAYLOAD/bin/"
cp -a Maps Content Audio UI Docs "$PAYLOAD/share/sbswars/"
cp -f UI/sbswars.png "$PAYLOAD/share/sbswars/UI/sbswars.png"
cp -f UI/sbswars.png "$PAYLOAD/share/icons/hicolor/256x256/apps/sbswars.png"
cp -f Packaging/linux/install.sh "$PAYLOAD/install.sh"
cp -f Packaging/linux/sbswars.desktop.in "$PAYLOAD/sbswars.desktop.in"
chmod +x "$PAYLOAD/install.sh" "$PAYLOAD/bin/"*

# self-extracting installer: extracts, then runs GUI setup (falls back to CLI)
INSTALLER="$DIST/SBSWarsInstaller.run"
{
  cat << 'HDR'
#!/usr/bin/env bash
set -euo pipefail
echo "SBS Wars Setup"
PREFIX="${1:-$HOME/SBSWars}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
ARCHIVE_LINE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' "$0")
tail -n "+$ARCHIVE_LINE" "$0" | tar -xz -C "$TMP"
PAYLOAD="$TMP/payload"
chmod +x "$PAYLOAD/install.sh" "$PAYLOAD/bin/"* 2>/dev/null || true
if [[ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" && -x "$PAYLOAD/bin/sbswars-installer" && "${SBS_INSTALLER_CLI:-}" != "1" ]]; then
  "$PAYLOAD/bin/sbswars-installer" --payload "$PAYLOAD" --prefix "$PREFIX"
else
  bash "$PAYLOAD/install.sh" --payload "$PAYLOAD" --prefix "$PREFIX" --desktop 1 --menu 1
fi
echo ""
echo "You can launch SBS Wars from:"
echo "  • Desktop shortcut:  SBS Wars"
echo "  • $PREFIX/SBSWarsLauncher"
exit 0
__ARCHIVE_BELOW__
HDR
  tar -cz -C "$DIST" payload
} > "$INSTALLER"
chmod +x "$INSTALLER"
cp -f "$ROOT/Binaries/sbswars-launcher" "$DIST/SBSWarsLauncher"
cp -f "$ROOT/Binaries/sbswars-installer" "$DIST/SBSWarsInstaller"
echo "Wrote $INSTALLER"
echo "Wrote $DIST/SBSWarsLauncher"
echo "Wrote $DIST/SBSWarsInstaller"
