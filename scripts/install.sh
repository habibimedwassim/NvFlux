#!/usr/bin/env bash
set -euo pipefail

INSTALL_BIN="/usr/local/bin/nvflux"
INSTALL_MAN="/usr/local/share/man/man1/nvflux.1.gz"

BASH_COMP="/etc/bash_completion.d/nvflux"
FISH_COMP="/usr/share/fish/vendor_completions.d/nvflux.fish"
ZSH_COMP="/usr/share/zsh/site-functions/_nvflux"

SOURCE_DIR="$(cd "$(dirname "$0")/.." && pwd)"

die() { echo "error: $*" >&2; exit 1; }

[ "$(id -u)" -eq 0 ] || die "must run as root (sudo $0)"

# ── Build ────────────────────────────────────────────────────────────────────
echo "==> Building nvflux..."

BUILD_DIR="$SOURCE_DIR/build"
if command -v cmake &>/dev/null; then
    cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF -Wno-dev 2>/dev/null
    cmake --build "$BUILD_DIR" --target nvflux -j"$(nproc)"
    BINARY="$BUILD_DIR/nvflux"
else
    echo "    cmake not found – using gcc fallback"
    BINARY="$BUILD_DIR/nvflux_tmp"
    mkdir -p "$BUILD_DIR"
    gcc -O2 -std=c11 -Wall -I"$SOURCE_DIR/include" \
        "$SOURCE_DIR/src/main.c"    \
        "$SOURCE_DIR/src/nvidia.c"  \
        "$SOURCE_DIR/src/profile.c" \
        "$SOURCE_DIR/src/state.c"   \
        -o "$BINARY"
fi

# ── Install binary (setuid root) ─────────────────────────────────────────────
echo "==> Installing to $INSTALL_BIN"
install -Dm0755 "$BINARY" "$INSTALL_BIN"
chown root:root "$INSTALL_BIN"
chmod 4755      "$INSTALL_BIN"

# ── Install man page ─────────────────────────────────────────────────────────
if [ -f "$SOURCE_DIR/man/nvflux.1" ]; then
    echo "==> Installing man page"
    mkdir -p "$(dirname "$INSTALL_MAN")"
    gzip -c "$SOURCE_DIR/man/nvflux.1" > "$INSTALL_MAN"
fi

# ── Shell completions ─────────────────────────────────────────────────────────
install_completion() {
    local src="$1" dst="$2"
    if [ -f "$src" ]; then
        echo "==> Installing $(basename "$dst")"
        mkdir -p "$(dirname "$dst")"
        install -Dm0644 "$src" "$dst"
    fi
}

install_completion "$SOURCE_DIR/completions/bash/nvflux"      "$BASH_COMP"
install_completion "$SOURCE_DIR/completions/fish/nvflux.fish" "$FISH_COMP"
install_completion "$SOURCE_DIR/completions/zsh/_nvflux"      "$ZSH_COMP"

echo ""
echo "Done! nvflux $("$INSTALL_BIN" --version) installed."
echo "Run 'nvflux --help' to get started."
