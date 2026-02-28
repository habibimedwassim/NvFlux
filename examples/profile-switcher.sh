#!/usr/bin/env bash
# Interactive nvflux profile switcher.
# Run from a terminal; select a profile with arrow keys or its number.

set -euo pipefail

PROFILES=(ultra performance balanced powersave auto)
DESCRIPTIONS=(
    "Max clocks — full GPU + memory lock"
    "High clocks — memory lock at maximum"
    "Mid clocks  — memory lock at midpoint"
    "Low clocks  — memory lock at minimum"
    "Default     — reset all clock locks"
)

# ── helpers ────────────────────────────────────────────────────────────────────

current_profile() {
    local state="$HOME/.local/state/nvflux/state"
    [[ -f "$state" ]] && cat "$state" || echo "unknown"
}

print_menu() {
    local current
    current=$(current_profile)
    echo ""
    echo "  nvflux profile switcher"
    echo "  Current: $current"
    echo ""
    for i in "${!PROFILES[@]}"; do
        local marker="  "
        [[ "${PROFILES[$i]}" == "$current" ]] && marker="> "
        printf "  %s[%d] %-14s %s\n" "$marker" $((i+1)) "${PROFILES[$i]}" "${DESCRIPTIONS[$i]}"
    done
    echo ""
    echo "  [q] Quit"
    echo ""
}

apply_profile() {
    local profile="$1"
    echo "  Applying: $profile …"
    if nvflux "$profile"; then
        echo "  Done."
    else
        echo "  Error: nvflux exited with status $?"
    fi
    sleep 1
}

# ── main ───────────────────────────────────────────────────────────────────────

if ! command -v nvflux &>/dev/null; then
    echo "Error: nvflux not found in PATH." >&2
    exit 1
fi

while true; do
    clear
    print_menu
    printf "  Choice: "
    read -r choice

    case "$choice" in
        1|ultra)       apply_profile ultra ;;
        2|performance) apply_profile performance ;;
        3|balanced)    apply_profile balanced ;;
        4|powersave)   apply_profile powersave ;;
        5|auto|reset)  apply_profile auto ;;
        q|Q|quit|exit) echo ""; break ;;
        *) echo "  Invalid choice." ; sleep 0.5 ;;
    esac
done
