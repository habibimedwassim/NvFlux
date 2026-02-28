#include "profile.h"
#include "nvidia.h"
#include "../include/nvflux.h"

#include <stdio.h>
#include <string.h>

int profile_from_str(const char *s, Profile *out)
{
    static const struct { const char *name; Profile val; } map[] = {
        { "ultra",       PROFILE_ULTRA       },
        { "performance", PROFILE_PERFORMANCE },
        { "balanced",    PROFILE_BALANCED    },
        { "powersave",   PROFILE_POWERSAVE   },
        { "auto",        PROFILE_AUTO        },
    };
    for (size_t i = 0; i < sizeof(map) / sizeof(*map); i++) {
        if (strcmp(s, map[i].name) == 0) { *out = map[i].val; return 0; }
    }
    return -1;
}

const char *profile_to_str(Profile p)
{
    switch (p) {
        case PROFILE_ULTRA:       return "ultra";
        case PROFILE_PERFORMANCE: return "performance";
        case PROFILE_BALANCED:    return "balanced";
        case PROFILE_POWERSAVE:   return "powersave";
        case PROFILE_AUTO:        return "auto";
    }
    return "unknown";
}

int profile_apply(Profile p)
{
    /* ── auto: unlock everything ────────────────────────────────────── */
    if (p == PROFILE_AUTO) {
        if (nv_reset_mem() != 0) {
            fprintf(stderr, "error: failed to reset memory clocks\n");
            return -1;
        }
        nv_reset_gpu(); /* best-effort — already unlocked if never locked */
        printf("all clock locks removed (auto / driver-managed)\n");
        return 0;
    }

    /* ── all other profiles: require persistence mode ───────────────── */
    if (nv_persistence_enable() != 0) {
        fprintf(stderr, "error: failed to enable persistence mode\n");
        return -1;
    }

    int mem[NV_MAX_CLOCKS];
    int nm = nv_mem_clocks(mem, NV_MAX_CLOCKS);
    if (nm <= 0) {
        fprintf(stderr, "error: failed to query supported memory clocks\n");
        return -1;
    }

    /* Select memory clock target */
    int mem_target;
    if (p == PROFILE_ULTRA || p == PROFILE_PERFORMANCE)
        mem_target = mem[0];           /* highest tier */
    else if (p == PROFILE_BALANCED)
        mem_target = mem[nm / 2];      /* mid tier     */
    else
        mem_target = mem[nm - 1];      /* lowest tier  */

    if (nv_lock_mem(mem_target) != 0) {
        fprintf(stderr, "error: failed to lock memory clocks to %d MHz\n", mem_target);
        return -1;
    }

    /* ── ultra: also lock GPU core clock ───────────────────────────── */
    if (p == PROFILE_ULTRA) {
        int gpu[NV_MAX_CLOCKS];
        int ng = nv_gpu_clocks(gpu, NV_MAX_CLOCKS);
        if (ng <= 0) {
            fprintf(stderr, "error: failed to query supported GPU core clocks\n");
            return -1;
        }
        if (nv_lock_gpu(gpu[0]) != 0) {
            fprintf(stderr, "error: failed to lock GPU core clocks to %d MHz\n", gpu[0]);
            return -1;
        }
        printf("memory %d MHz · GPU core %d MHz locked (ultra)\n",
               mem_target, gpu[0]);
    } else {
        /* Clear any prior GPU core lock (e.g. coming from ultra) */
        nv_reset_gpu();
        printf("memory %d MHz locked (%s)\n", mem_target, profile_to_str(p));
    }

    return 0;
}
