#!/usr/bin/env bash
# nvflux rofi launcher — pick a GPU profile from a rofi menu.
# Designed for i3, Sway, and other tiling WM setups.
#
# Usage:
#   bind it to a keybind in your WM config, e.g.:
#   i3:   bindsym $mod+shift+g exec --no-startup-id /path/to/rofi-launcher.sh
#   sway: bindsym $mod+shift+g exec /path/to/rofi-launcher.sh

set -euo pipefail

# ── options ────────────────────────────────────────────────────────────────────

ROFI_ARGS=(-dmenu -p "GPU profile" -i -lines 5)

declare -A LABEL_MAP=(
    ["⚡  Ultra        — max GPU + memory clocks"]="ultra"
    ["🚀  Performance  — max memory clock"]="performance"
    ["⚖️   Balanced     — mid memory clock"]="balanced"
    ["🍃  Powersave    — min memory clock"]="powersave"
    ["🔄  Auto         — reset clock locks"]="auto"
)

ORDER=(
    "⚡  Ultra        — max GPU + memory clocks"
    "🚀  Performance  — max memory clock"
    "⚖️   Balanced     — mid memory clock"
    "🍃  Powersave    — min memory clock"
    "🔄  Auto         — reset clock locks"
)

# ── helpers ────────────────────────────────────────────────────────────────────

notify() {
    local summary="$1" body="${2:-}"
    if command -v notify-send &>/dev/null; then
        notify-send -a nvflux -t 3000 "$summary" "$body"
    fi
}

current_label() {
    local state="$HOME/.local/state/nvflux/state"
    [[ -f "$state" ]] && echo "(current: $(cat "$state"))" || echo ""
}

# ── guards ─────────────────────────────────────────────────────────────────────

if ! command -v rofi &>/dev/null; then
    notify "nvflux" "rofi not found in PATH."
    exit 1
fi

if ! command -v nvflux &>/dev/null; then
    notify "nvflux" "nvflux not found in PATH."
    exit 1
fi

# ── menu ───────────────────────────────────────────────────────────────────────

menu=$(printf '%s\n' "${ORDER[@]}")

selected=$(echo "$menu" | rofi "${ROFI_ARGS[@]}" -mesg "$(current_label)") || exit 0

profile="${LABEL_MAP[$selected]:-}"
if [[ -z "$profile" ]]; then
    exit 0
fi

# ── apply ──────────────────────────────────────────────────────────────────────

if nvflux "$profile"; then
    notify "nvflux" "Profile set to: $profile"
else
    status=$?
    case $status in
        2) notify "nvflux error" "nvidia-smi not found." ;;
        3) notify "nvflux error" "Root privileges required." ;;
        4) notify "nvflux error" "No NVIDIA GPU detected." ;;
        *) notify "nvflux error" "nvflux exited with status $status." ;;
    esac
    exit "$status"
fi
