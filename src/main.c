#include "../include/nvflux.h"
#include "nvidia.h"
#include "profile.h"
#include "state.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void print_help(void)
{
    printf(
        "nvflux %s — NVIDIA GPU clock profile manager\n\n"
        "usage: nvflux <command>\n\n"
        "commands:\n"
        "  ultra         lock memory + GPU core to maximum clocks\n"
        "  performance   lock memory to highest supported tier\n"
        "  balanced      lock memory to mid-range supported tier\n"
        "  powersave     lock memory to lowest supported tier\n"
        "  auto          unlock all clocks (driver-managed)\n"
        "  status        show last saved profile (no root needed)\n"
        "  clock         print current memory clock in MHz\n"
        "  --restore     re-apply the last saved profile\n"
        "  -v --version  print version and exit\n"
        "  -h --help     show this help\n\n"
        "notes:\n"
        "  nvflux is installed setuid root; scripts/install.sh does this.\n"
        "  Clock tiers are queried live from the driver — no hard-coded values.\n",
        NVFLUX_VERSION);
}

int main(int argc, char **argv)
{
    if (argc < 2) { print_help(); return 1; }

    const char *cmd = argv[1];

    if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        printf("nvflux %s\n", NVFLUX_VERSION);
        return 0;
    }

    uid_t real_uid = getuid();

    /* ── status: no GPU access, no root needed ──────────────────────── */
    if (strcmp(cmd, "status") == 0) {
        char mode[64] = {0};
        if (state_read(real_uid, mode, sizeof(mode))) {
            /* Capitalise first letter for display */
            if (mode[0] >= 'a' && mode[0] <= 'z') mode[0] += 'A' - 'a';
            printf("%s\n", mode);
        } else {
            printf("Default\n");
        }
        return 0;
    }

    /* ── locate nvidia-smi (required for everything below) ──────────── */
    if (nv_init() != 0) {
        fprintf(stderr,
            "error: nvidia-smi not found — install NVIDIA drivers\n"
            "hint:  see README or docs/INSTALLATION.md\n");
        return 2;
    }

    /* ── clock: read-only query, no root needed ─────────────────────── */
    if (strcmp(cmd, "clock") == 0) {
        int mhz = nv_current_mem_clock();
        if (mhz < 0) { fprintf(stderr, "error: failed to query memory clock\n"); return 1; }
        printf("%d MHz\n", mhz);
        return 0;
    }

    /* ── all remaining commands require root ─────────────────────────── */
    if (geteuid() != 0) {
        fprintf(stderr,
            "error: nvflux requires root privileges\n"
            "hint:  sudo scripts/install.sh sets the setuid bit\n");
        return 3;
    }

    if (nv_check() != 0) return 4;

    /* ── --restore: reload and re-dispatch saved profile ────────────── */
    Profile p;
    if (strcmp(cmd, "--restore") == 0) {
        char mode[64] = {0};
        if (!state_read(real_uid, mode, sizeof(mode))) {
            fprintf(stderr, "error: no saved profile — set one first\n");
            return 1;
        }
        if (profile_from_str(mode, &p) != 0) {
            fprintf(stderr,
                "error: saved profile '%s' is invalid\n"
                "hint:  delete ~/.local/state/nvflux/state and set a new profile\n",
                mode);
            return 1;
        }
        cmd = mode;
    } else if (profile_from_str(cmd, &p) != 0) {
        fprintf(stderr, "error: unknown command '%s'  (try --help)\n", cmd);
        return 5;
    }

    if (profile_apply(p) != 0) return 1;
    state_write(real_uid, profile_to_str(p));
    return 0;
}
