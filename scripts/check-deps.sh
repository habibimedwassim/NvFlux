#!/usr/bin/env bash
set -euo pipefail

ok=0
fail=0

check() {
    local name="$1" cmd="$2"
    if command -v "$cmd" &>/dev/null; then
        printf "  %-20s OK\n" "$name"
        ((ok++)) || true
    else
        printf "  %-20s MISSING\n" "$name"
        ((fail++)) || true
    fi
}

echo "Checking runtime dependencies..."
check "nvidia-smi"   nvidia-smi

echo ""
echo "Checking build dependencies..."
check "gcc or clang" gcc || check "clang" clang
check "cmake"        cmake
check "gzip"         gzip
check "make"         make

echo ""
if [ "$fail" -eq 0 ]; then
    echo "All dependencies satisfied."
else
    echo "$fail dependency/dependencies missing. Install with:"
    # Detect distro
    distro=""
    if   [ -f /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
        distro="${ID_LIKE:-${ID:-}}"
    fi
    case "$distro" in
        *arch*|arch)
            echo "  sudo pacman -S nvidia-utils base-devel cmake gzip" ;;
        *debian*|*ubuntu*|debian|ubuntu)
            echo "  sudo apt install build-essential cmake gzip" ;;
        *fedora*|*rhel*|*centos*|fedora)
            echo "  sudo dnf install @development-tools cmake gzip" ;;
        *suse*|opensuse)
            echo "  sudo zypper install -t pattern devel_C_C++ cmake gzip" ;;
        solus)
            echo "  sudo eopkg it -c system.devel" ;;
        *void*|void)
            echo "  sudo xbps-install -S base-devel cmake gzip nvidia-utils" ;;
        *)
            echo "  Install: nvidia-utils (or equivalent), cmake, gcc, gzip" ;;
    esac
    echo ""
    echo "See docs/INSTALLATION.md for distro-specific instructions."
    exit 1
fi
