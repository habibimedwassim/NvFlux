/*
 * state.h — NvFlux persistent state file
 *
 * The state file lives at:
 *   $HOME/.local/state/nvflux/state   (normal users)
 *   /root/.local/state/nvflux/state   (root)
 *
 * Format is a plain key=value text file, one entry per line:
 *   mode=powersave
 *   timestamp=2026-02-26T03:15:22
 *   memory_mhz=810
 *   graphics_mhz=487
 *   temp_c=58
 */

#ifndef NVFLUX_STATE_H
#define NVFLUX_STATE_H

#include <sys/types.h>   /* uid_t */

typedef struct {
    char mode[64];         /* profile name: performance/balanced/powersave/clock */
    char timestamp[32];    /* ISO-8601 local time: YYYY-MM-DDTHH:MM:SS */
    int  memory_mhz;       /* actual memory clock after driver adjustment */
    int  graphics_mhz;     /* actual graphics clock after driver adjustment */
    int  temp_c;           /* GPU temperature at time of lock */
} nvflux_state_t;

/*
 * state_write(real_uid, s)
 *   Create/update the state file owned by real_uid.
 *   Fills s->timestamp automatically from the current local time.
 *   Returns 0 on success, -1 on error.
 */
int state_write(uid_t real_uid, const nvflux_state_t *s);

/*
 * state_read(real_uid, s)
 *   Read the state file belonging to real_uid into *s.
 *   Returns 0 on success, -1 if the file does not exist or is unreadable.
 */
int state_read(uid_t real_uid, nvflux_state_t *s);

#endif /* NVFLUX_STATE_H */
