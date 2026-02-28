#!/usr/bin/env bash
set -euo pipefail

die() { echo "error: $*" >&2; exit 1; }
[ "$(id -u)" -eq 0 ] || die "must run as root (sudo $0)"

remove() {
    if [ -e "$1" ]; then
        rm -f "$1"
        echo "removed $1"
    fi
}

remove /usr/local/bin/nvflux
remove /usr/local/share/man/man1/nvflux.1.gz
remove /etc/bash_completion.d/nvflux
remove /usr/share/fish/vendor_completions.d/nvflux.fish
remove /usr/share/zsh/site-functions/_nvflux

echo ""
echo "nvflux uninstalled."
echo "Note: per-user state files at ~/.local/state/nvflux/ are NOT removed."
