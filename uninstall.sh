#!/bin/sh
set -e

OUT=/usr/local/bin/nvflux

if [ -f "$OUT" ]; then
    echo "Removing $OUT"
    rm -f "$OUT"
else
    echo "nvflux binary not found at $OUT"
fi

# Remove installer user's state dir
user=${SUDO_USER:-$(logname 2>/dev/null || whoami)}
home_dir=$(eval echo "~$user")
state_dir="$home_dir/.local/state/nvflux"
if [ -d "$state_dir" ]; then
    echo "Removing user state dir $state_dir"
    rm -rf "$state_dir"
fi

echo "Uninstall complete."