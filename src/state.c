/*
 * state.c — NvFlux persistent state file read / write
 *
 * State is stored as a plain key=value text file at:
 *   $HOME/.local/state/nvflux/state   (normal users)
 *   /root/.local/state/nvflux/state   (root)
 *
 * Example content:
 *   mode=powersave
 *   timestamp=2026-02-26T03:15:22
 *   memory_mhz=810
 *   graphics_mhz=487
 *   temp_c=58
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>

#include "../include/state.h"

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

/*
 * state_path — write the full path to the state file for real_uid
 * into buf (size len).  Returns 0 on success, -1 if home dir unknown.
 */
static int state_path(uid_t real_uid, char *buf, size_t len)
{
    const char *home = NULL;

    if (real_uid == getuid()) {
        home = getenv("HOME");
    }

    if (!home || home[0] == '\0') {
        struct passwd *pw = getpwuid(real_uid);
        if (pw)
            home = pw->pw_dir;
    }

    if (!home || home[0] == '\0')
        return -1;

    snprintf(buf, len, "%s/.local/state/nvflux/state", home);
    return 0;
}

/*
 * mkdirp — create all path components up to but not including the
 * last component (the filename).  Tolerates pre-existing dirs.
 */
static void mkdirp(const char *filepath)
{
    char tmp[512];
    strncpy(tmp, filepath, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    /* find last '/' — everything before is the directory */
    char *slash = strrchr(tmp, '/');
    if (!slash)
        return;
    *slash = '\0';

    /* walk and create each component */
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

int state_write(uid_t real_uid, const nvflux_state_t *s)
{
    char path[512];
    if (state_path(real_uid, path, sizeof(path)) < 0) {
        fprintf(stderr, "nvflux: cannot determine home directory for uid %d\n",
                (int)real_uid);
        return -1;
    }

    mkdirp(path);

    /* Build the timestamp now */
    char ts[32] = "";
    time_t now = time(NULL);
    struct tm *tm_local = localtime(&now);
    if (tm_local)
        strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", tm_local);

    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "nvflux: cannot write state file %s: %s\n",
                path, strerror(errno));
        return -1;
    }

    fprintf(f, "mode=%s\n",         s->mode);
    fprintf(f, "timestamp=%s\n",    ts);
    fprintf(f, "memory_mhz=%d\n",   s->memory_mhz);
    fprintf(f, "graphics_mhz=%d\n", s->graphics_mhz);
    fprintf(f, "temp_c=%d\n",       s->temp_c);

    fflush(f);
    fclose(f);

    /* hand ownership back to the real user if running setuid */
    if (real_uid != geteuid()) {
        if (chown(path, real_uid, (gid_t)-1) < 0) {
            /* non-fatal: ownership transfer failed, but data is written */
            fprintf(stderr,
                    "nvflux: warning: chown(%s, %d) failed: %s\n",
                    path, (int)real_uid, strerror(errno));
        }
    }
    return 0;
}

int state_read(uid_t real_uid, nvflux_state_t *s)
{
    char path[512];
    if (state_path(real_uid, path, sizeof(path)) < 0)
        return -1;

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    /* zero-initialise so partial files still produce a safe struct */
    memset(s, 0, sizeof(*s));

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        /* strip trailing newline */
        size_t l = strlen(line);
        while (l > 0 && (line[l-1] == '\n' || line[l-1] == '\r'))
            line[--l] = '\0';

        char *eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = '\0';
        const char *key = line;
        const char *val = eq + 1;

        if (strcmp(key, "mode") == 0)
            strncpy(s->mode, val, sizeof(s->mode) - 1);
        else if (strcmp(key, "timestamp") == 0)
            strncpy(s->timestamp, val, sizeof(s->timestamp) - 1);
        else if (strcmp(key, "memory_mhz") == 0)
            s->memory_mhz = atoi(val);
        else if (strcmp(key, "graphics_mhz") == 0)
            s->graphics_mhz = atoi(val);
        else if (strcmp(key, "temp_c") == 0)
            s->temp_c = atoi(val);
    }

    fclose(f);
    return 0;
}
