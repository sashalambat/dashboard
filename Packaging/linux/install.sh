#!/usr/bin/env bash
# SBS Wars installer — copies files and creates desktop / menu shortcuts.
set -euo pipefail

PREFIX="${PREFIX:-}"
PAYLOAD="${PAYLOAD:-}"
CREATE_DESKTOP="${CREATE_DESKTOP:-1}"
CREATE_MENU="${CREATE_MENU:-1}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix) PREFIX="${2:-}"; shift 2 ;;
        --payload) PAYLOAD="${2:-}"; shift 2 ;;
        --desktop) CREATE_DESKTOP="${2:-1}"; shift 2 ;;
        --menu) CREATE_MENU="${2:-1}"; shift 2 ;;
        --no-desktop) CREATE_DESKTOP=0; shift ;;
        --no-menu) CREATE_MENU=0; shift ;;
        *)
            if [[ -z "$PREFIX" && "$1" != --* ]]; then
                PREFIX="$1"
                shift
            else
                echo "Unknown argument: $1" >&2
                exit 2
            fi
            ;;
    esac
done

HERE="$(cd "$(dirname "$0")" && pwd)"
PAYLOAD="${PAYLOAD:-$HERE}"
PREFIX="${PREFIX:-$HOME/SBSWars}"

if [[ ! -x "$PAYLOAD/bin/sbswars-launcher" && ! -x "$PAYLOAD/bin/sbswars" ]]; then
    echo "Payload not found at $PAYLOAD (missing bin/sbswars-launcher)" >&2
    exit 1
fi

mkdir -p "$PREFIX/bin" "$PREFIX/share/sbswars" "$PREFIX/share/icons/hicolor/256x256/apps"
cp -a "$PAYLOAD/bin/." "$PREFIX/bin/"
if [[ -d "$PAYLOAD/share/sbswars" ]]; then
    cp -a "$PAYLOAD/share/sbswars/." "$PREFIX/share/sbswars/"
fi
chmod +x "$PREFIX/bin/"* 2>/dev/null || true

ICON_SRC=""
for cand in \
    "$PAYLOAD/share/icons/hicolor/256x256/apps/sbswars.png" \
    "$PAYLOAD/share/sbswars/UI/sbswars.png" \
    "$PREFIX/share/sbswars/UI/sbswars.png"; do
    if [[ -f "$cand" ]]; then
        ICON_SRC="$cand"
        break
    fi
done
if [[ -n "$ICON_SRC" ]]; then
    cp -f "$ICON_SRC" "$PREFIX/share/icons/hicolor/256x256/apps/sbswars.png"
    cp -f "$ICON_SRC" "$PREFIX/share/icons/sbswars.png"
fi
ICON_PATH="$PREFIX/share/icons/hicolor/256x256/apps/sbswars.png"
if [[ ! -f "$ICON_PATH" ]]; then
    ICON_PATH="applications-games"
fi

LAUNCHER="$PREFIX/bin/sbswars-launcher"
if [[ ! -x "$LAUNCHER" ]]; then
    LAUNCHER="$PREFIX/bin/sbswars"
fi
# Stable name used by the desktop shortcut
cp -f "$LAUNCHER" "$PREFIX/SBSWarsLauncher"
chmod +x "$PREFIX/SBSWarsLauncher"

write_desktop() {
    local dest="$1"
    mkdir -p "$(dirname "$dest")"
    cat > "$dest" << EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=SBS Wars
GenericName=First-Person Shooter
Comment=LAN-only sci-fi military FPS — SBS Alliance vs Cyber Dominion
Exec="${PREFIX}/SBSWarsLauncher"
TryExec=${PREFIX}/SBSWarsLauncher
Icon=${ICON_PATH}
Path=${PREFIX}/bin
Terminal=false
StartupNotify=true
Categories=Game;ActionGame;
Keywords=FPS;LAN;Shooter;SBS;
StartupWMClass=SBS Wars
EOF
    chmod +x "$dest"
}

trust_desktop() {
    local file="$1"
    [[ -f "$file" ]] || return 0
    chmod +x "$file"
    if command -v gio >/dev/null 2>&1; then
        gio set "$file" metadata::trusted true >/dev/null 2>&1 || true
        gio set -t string "$file" metadata::trusted true >/dev/null 2>&1 || true
    fi
}

desktop_dir() {
    local d="${HOME}/Desktop"
    if [[ -f "${HOME}/.config/user-dirs.dirs" ]]; then
        # shellcheck disable=SC1091
        . "${HOME}/.config/user-dirs.dirs"
        if [[ -n "${XDG_DESKTOP_DIR:-}" ]]; then
            d="${XDG_DESKTOP_DIR}"
        fi
    fi
    mkdir -p "$d"
    printf '%s\n' "$d"
}

SHORTCUT=""
if [[ "$CREATE_DESKTOP" == "1" ]]; then
    DESKTOP_DIR="$(desktop_dir)"
    SHORTCUT="${DESKTOP_DIR}/SBS Wars.desktop"
    write_desktop "$SHORTCUT"
    trust_desktop "$SHORTCUT"
    if command -v xdg-desktop-icon >/dev/null 2>&1; then
        xdg-desktop-icon install --novendor "$SHORTCUT" >/dev/null 2>&1 || true
    fi
    echo "Desktop shortcut: $SHORTCUT"
fi

if [[ "$CREATE_MENU" == "1" ]]; then
    MENU="${HOME}/.local/share/applications/sbswars.desktop"
    write_desktop "$MENU"
    trust_desktop "$MENU"
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "${HOME}/.local/share/applications" >/dev/null 2>&1 || true
    fi
    if command -v xdg-desktop-menu >/dev/null 2>&1; then
        xdg-desktop-menu install --novendor "$MENU" >/dev/null 2>&1 || true
    fi
    echo "Applications menu: $MENU"
fi

mkdir -p "${HOME}/.local/bin"
ln -sfn "$PREFIX/SBSWarsLauncher" "${HOME}/.local/bin/SBSWarsLauncher"
ln -sfn "$PREFIX/bin/sbswars" "${HOME}/.local/bin/sbswars" 2>/dev/null || true

cat > "$PREFIX/uninstall.sh" << EOF
#!/usr/bin/env bash
set -euo pipefail
rm -f "${HOME}/Desktop/SBS Wars.desktop" "${HOME}/.local/share/applications/sbswars.desktop"
rm -f "${HOME}/.local/bin/SBSWarsLauncher" "${HOME}/.local/bin/sbswars"
if [[ -f "${HOME}/.config/user-dirs.dirs" ]]; then
  . "${HOME}/.config/user-dirs.dirs"
  rm -f "\${XDG_DESKTOP_DIR:-}/SBS Wars.desktop"
fi
rm -rf "$PREFIX"
echo "SBS Wars uninstalled."
EOF
chmod +x "$PREFIX/uninstall.sh"

echo "Installed SBS Wars to $PREFIX"
echo "Launcher: $PREFIX/SBSWarsLauncher"
if [[ -n "$SHORTCUT" ]]; then
    echo "Double-click the desktop shortcut to play."
fi
