#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
if [[ ! -x Binaries/sbswars ]]; then
  Scripts/build.sh
fi
DIST="$ROOT/Dist"
PAYLOAD="$DIST/payload"
rm -rf "$PAYLOAD"
mkdir -p "$PAYLOAD/bin" "$PAYLOAD/share/sbswars" "$DIST"
cp -a Binaries/sbswars Binaries/sbswars-server Binaries/sbswars-launcher "$PAYLOAD/bin/"
cp -a Maps Content Audio UI Docs "$PAYLOAD/share/sbswars/"
cat > "$PAYLOAD/install.sh" << 'EOS'
#!/usr/bin/env bash
set -euo pipefail
PREFIX="${1:-$HOME/SBSWars}"
HERE="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$PREFIX/bin" "$PREFIX/share/sbswars"
cp -a "$HERE/bin/." "$PREFIX/bin/"
cp -a "$HERE/share/sbswars/." "$PREFIX/share/sbswars/"
chmod +x "$PREFIX/bin/"*
mkdir -p "$HOME/.local/bin"
ln -sfn "$PREFIX/bin/sbswars-launcher" "$HOME/.local/bin/SBSWarsLauncher"
ln -sfn "$PREFIX/bin/sbswars" "$HOME/.local/bin/sbswars"
echo "Installed SBS Wars to $PREFIX"
echo "Launch with: $PREFIX/bin/sbswars-launcher"
EOS
chmod +x "$PAYLOAD/install.sh" "$PAYLOAD/bin/"*
# self-extracting installer
INSTALLER="$DIST/SBSWarsInstaller.run"
{
  cat << 'HDR'
#!/usr/bin/env bash
set -euo pipefail
echo "SBS Wars installer"
PREFIX="${1:-$HOME/SBSWars}"
TMP="$(mktemp -d)"
ARCHIVE_LINE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' "$0")
tail -n "+$ARCHIVE_LINE" "$0" | tar -xz -C "$TMP"
bash "$TMP/payload/install.sh" "$PREFIX"
cp -f "$PREFIX/bin/sbswars-launcher" "$PREFIX/SBSWarsLauncher" 2>/dev/null || true
echo "Done. Run: $PREFIX/bin/sbswars-launcher"
exit 0
__ARCHIVE_BELOW__
HDR
  tar -cz -C "$DIST" payload
} > "$INSTALLER"
chmod +x "$INSTALLER"
cp -f "$ROOT/Binaries/sbswars-launcher" "$DIST/SBSWarsLauncher"
echo "Wrote $INSTALLER"
echo "Wrote $DIST/SBSWarsLauncher"
