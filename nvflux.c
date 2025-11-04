#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>

#define MAX_CLOCKS 128
#define READ_BUF 4096

static const char *allowed_cmds[] = {
    "performance", "balanced", "powersaver", "auto", "reset", "status", "clock", "--restore", NULL
};

static char nvsmipath[512] = {0};

static int find_nvidia_smi(void) {
    const char *candidates[] = {
        "/usr/bin/nvidia-smi",
        "/usr/local/bin/nvidia-smi",
        NULL
    };

    for (int i = 0; candidates[i]; ++i) {
        if (access(candidates[i], X_OK) == 0) {
            snprintf(nvsmipath, sizeof(nvsmipath), "%s", candidates[i]);
            return 0;
        }
    }

    const char *path = getenv("PATH");
    if (!path) return -1;

    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);

    for (char *tok = strtok(tmp, ":"); tok; tok = strtok(NULL, ":")) {
        char candidate[512];
        snprintf(candidate, sizeof(candidate), "%s/nvidia-smi", tok);
        if (access(candidate, X_OK) == 0) {
            snprintf(nvsmipath, sizeof(nvsmipath), "%s", candidate);
            return 0;
        }
    }

    return -1;
}

static void get_state_path(uid_t real_uid, char *out, size_t len) {
    struct passwd *pw = getpwuid(real_uid);
    const char *home = pw ? pw->pw_dir : getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(out, len, "%s/.local/state/nvflux/state", home);
}


static int write_state(uid_t real_uid, const char *mode) {
    char path[512];
    get_state_path(real_uid, path, sizeof(path));
    
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    
    ssize_t w = write(fd, mode, strlen(mode));
    (void)w; 
    write(fd, "\n", 1);
    close(fd);

    // chown file to real user
    if (chown(path, real_uid, -1) < 0 && errno != EPERM) {
        // ignore
    }
    return 0;
}

static int read_state(uid_t real_uid, char *buf, size_t len) {
    char path[512];
    get_state_path(real_uid, path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(buf, len, f)) { fclose(f); return 0; }
    buf[strcspn(buf, "\n")] = '\0';
    fclose(f);
    return 1;
}