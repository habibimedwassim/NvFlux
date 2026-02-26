#!/bin/bash
# Example: Simple GPU profile switcher menu using nvflux

set -e

# Check if nvflux is installed
if ! command -v nvflux >/dev/null 2>&1; then
    echo "Error: nvflux not found. Please install first." >&2
    exit 1
fi

echo "=== NVIDIA GPU Profile Switcher ==="
echo ""
echo "Current status: $(nvflux status)"
nvflux clock
echo ""
echo "Select profile:"
echo "  1) Performance (highest clocks)"
echo "  2) Balanced (mid-range clocks)"
echo "  3) Power Saver (lowest clocks)"
echo "  4) Auto/Reset (driver managed)"
echo "  5) Exit"
echo ""
read -r -p "Choice [1-5]: " choice

case "$choice" in
    1)
        echo "Switching to Performance profile..."
        nvflux performance
        ;;
    2)
        echo "Switching to Balanced profile..."
        nvflux balanced
        ;;
    3)
        echo "Switching to Power Saver profile..."
        nvflux powersaver
        ;;
    4)
        echo "Resetting to Auto mode..."
        nvflux reset
        ;;
    5)
        echo "Exiting."
        exit 0
        ;;
    *)
        echo "Invalid choice." >&2
        exit 1
        ;;
esac

echo ""
echo "New status: $(nvflux status)"
nvflux clock