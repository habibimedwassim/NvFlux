#!/bin/sh
# Interactive GPU profile switcher using nvflux.
set -e

command -v nvflux >/dev/null 2>&1 || { echo "Error: nvflux not installed." >&2; exit 1; }

printf "Current profile : %s\n" "$(nvflux status)"
printf "Current mem clock: %s\n\n" "$(nvflux clock)"

cat <<'EOF'
  1) Ultra        (GPU + memory at max clocks — PowerMizer: Prefer Max Performance)
  2) Performance  (memory highest tier, GPU clock untouched)
  3) Balanced     (memory mid tier, GPU clock untouched)
  4) Power Save   (memory lowest tier, GPU clock untouched)
  5) Auto         (unlock all clocks — PowerMizer: Adaptive)
  6) Exit
EOF

printf "\nChoice [1-6]: "
read -r choice

case "$choice" in
    1) nvflux ultra       ;;
    2) nvflux performance ;;
    3) nvflux balanced    ;;
    4) nvflux powersave   ;;
    5) nvflux auto        ;;
    6) exit 0             ;;
    *) echo "Invalid choice." >&2; exit 1 ;;
esac

printf "\nNew profile : %s\n" "$(nvflux status)"
printf "New mem clock: %s\n"  "$(nvflux clock)"
