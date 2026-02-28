#include "state.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>
#include <pwd.h>

static void state_path(uid_t uid, char *out, size_t len)
{
    const char *home = NULL;
    struct passwd *pw = getpwuid(uid);
    if (pw) home = pw->pw_dir;
    if (!home || !home[0]) home = "/tmp";
    snprintf(out, len, "%s/.local/state/nvflux/state", home);
}

int state_write(uid_t uid, const char *mode)
{
    char path[PATH_MAX];
    state_path(uid, path, sizeof(path));

    /* Ensure parent directory exists */
    char dir[PATH_MAX];
    strncpy(dir, path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    char *sl = strrchr(dir, '/');
    if (sl) { *sl = '\0'; mkdir(dir, 0755); }

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return -1;
    fchown(fd, uid, (gid_t)-1);
    size_t mlen = strlen(mode);
    ssize_t w = write(fd, mode, mlen);
    close(fd);
    return w == (ssize_t)mlen ? 0 : -1;
}

int state_read(uid_t uid, char *buf, size_t len)
{
    char path[PATH_MAX];
    state_path(uid, path, sizeof(path));

    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    ssize_t r = read(fd, buf, len - 1);
    close(fd);
    if (r <= 0) return 0;

    buf[r] = '\0';
    while (r > 0 && isspace((unsigned char)buf[r - 1])) buf[--r] = '\0';
    return r > 0 ? 1 : 0;
}
