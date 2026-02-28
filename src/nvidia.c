#include "nvidia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <sys/wait.h>

/* Cached path to the nvidia-smi binary. */
static char nvsmi[PATH_MAX];

/* --------------------------------------------------------------------------
 * nv_init – locate nvidia-smi
 * ----------------------------------------------------------------------- */
int nv_init(void)
{
    static const char *const known[] = {
        "/usr/bin/nvidia-smi",
        "/usr/local/bin/nvidia-smi",
        "/opt/nvidia/bin/nvidia-smi",
        NULL,
    };
    for (int i = 0; known[i]; i++) {
        if (access(known[i], X_OK) == 0) {
            snprintf(nvsmi, sizeof(nvsmi), "%s", known[i]);
            return 0;
        }
    }

    /* Walk PATH */
    const char *env = getenv("PATH");
    if (!env) return -1;
    char tmp[PATH_MAX];
    strncpy(tmp, env, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    for (char *seg = tmp, *end; seg; seg = end ? end + 1 : NULL) {
        end = strchr(seg, ':');
        if (end) *end = '\0';
        char cand[PATH_MAX];
        int n = snprintf(cand, sizeof(cand), "%s/nvidia-smi", seg);
        if (n > 0 && n < (int)sizeof(cand) && access(cand, X_OK) == 0) {
            snprintf(nvsmi, sizeof(nvsmi), "%s", cand);
            return 0;
        }
        if (!end) break;
    }
    return -1;
}

/* --------------------------------------------------------------------------
 * nv_capture / nv_run – fork + execv wrappers (no shell)
 * ----------------------------------------------------------------------- */
int nv_capture(char *const argv[], char *buf, size_t len)
{
    int fd[2];
    if (pipe(fd) < 0) return -1;

    pid_t pid = fork();
    if (pid < 0) { close(fd[0]); close(fd[1]); return -1; }
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);
        execv(argv[0], argv);
        _exit(127);
    }
    close(fd[1]);

    size_t total = 0;
    ssize_t r;
    while (total + 1 < len &&
           (r = read(fd[0], buf + total, len - total - 1)) > 0)
        total += (size_t)r;
    close(fd[0]);
    buf[total] = '\0';

    int st = 0;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

int nv_run(char *const argv[])
{
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) { execv(argv[0], argv); _exit(127); }
    int st = 0;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

/* --------------------------------------------------------------------------
 * nv_parse_clocks – extract integers from CSV driver output
 * ----------------------------------------------------------------------- */
int nv_parse_clocks(const char *txt, int *clocks, int max)
{
    int n = 0;
    for (const char *p = txt; *p && n < max; ) {
        while (*p && !isdigit((unsigned char)*p)) p++;
        if (!*p) break;
        char *end;
        clocks[n++] = (int)strtol(p, &end, 10);
        p = end;
    }
    /* Selection sort – descending */
    for (int i = 0; i < n; i++) {
        int hi = i;
        for (int j = i + 1; j < n; j++)
            if (clocks[j] > clocks[hi]) hi = j;
        if (hi != i) { int t = clocks[i]; clocks[i] = clocks[hi]; clocks[hi] = t; }
    }
    return n;
}

/* --------------------------------------------------------------------------
 * Clock queries
 * ----------------------------------------------------------------------- */
static int query_clocks(const char *type, int *clocks, int max)
{
    char buf[4096], qarg[64];
    snprintf(qarg, sizeof(qarg), "--query-supported-clocks=%s", type);
    char *argv[] = { nvsmi, qarg, "--format=csv,noheader,nounits", NULL };
    if (nv_capture(argv, buf, sizeof(buf)) != 0) return -1;
    return nv_parse_clocks(buf, clocks, max);
}

int nv_mem_clocks(int *clocks, int max) { return query_clocks("memory",   clocks, max); }
int nv_gpu_clocks(int *clocks, int max) { return query_clocks("graphics", clocks, max); }

int nv_current_mem_clock(void)
{
    char buf[256];
    /* Try both query keys – the name changed between driver generations */
    const char *keys[] = { "clocks.mem", "memory.clock", NULL };
    for (int i = 0; keys[i]; i++) {
        char q[64];
        snprintf(q, sizeof(q), "--query-gpu=%s", keys[i]);
        char *argv[] = { nvsmi, q, "--format=csv,noheader,nounits", NULL };
        if (nv_capture(argv, buf, sizeof(buf)) != 0) continue;
        const char *p = buf;
        while (*p && !isdigit((unsigned char)*p)) p++;
        if (*p) return (int)strtol(p, NULL, 10);
    }
    return -1;
}

int nv_check(void)
{
    char buf[512];
    char *argv[] = { nvsmi, "--query-gpu=name", "--format=csv,noheader", NULL };
    if (nv_capture(argv, buf, sizeof(buf)) != 0) {
        fprintf(stderr, "error: cannot execute %s\n", nvsmi);
        return -1;
    }
    if (!buf[0] || strstr(buf, "No devices")) {
        fprintf(stderr,
            "error: no NVIDIA GPU found or driver not loaded\n"
            "hint:  sudo modprobe nvidia\n");
        return -1;
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * Clock controls
 * ----------------------------------------------------------------------- */
int nv_persistence_enable(void)
{
    char *argv[] = { nvsmi, "-pm", "1", NULL };
    return nv_run(argv) == 0 ? 0 : -1;
}

int nv_lock_mem(int mhz)
{
    char arg[64];
    /* Standard path: supported on Maxwell through Ada (Volta+) */
    snprintf(arg, sizeof(arg), "--lock-memory-clocks=%d,%d", mhz, mhz);
    char *argv[] = { nvsmi, arg, NULL };
    if (nv_run(argv) == 0) return 0;

    /* Hopper+ requires deferred mode – takes effect after driver reload */
    snprintf(arg, sizeof(arg), "--lock-memory-clocks-deferred=%d", mhz);
    char *argv2[] = { nvsmi, arg, NULL };
    if (nv_run(argv2) == 0) {
        fprintf(stderr,
            "note: used --lock-memory-clocks-deferred (Hopper+ GPU);\n"
            "      setting takes effect after a driver reload:\n"
            "      sudo rmmod nvidia_uvm nvidia_drm nvidia_modeset nvidia\n"
            "      sudo modprobe nvidia\n");
        return 0;
    }
    return -1;
}

int nv_reset_mem(void)
{
    /* Try standard flags first, then Hopper deferred variants */
    const char *opts[] = {
        "--reset-memory-clocks",
        "-rmc",
        "--reset-memory-clocks-deferred",
        "-rmcd",
        NULL,
    };
    for (const char **opt = opts; *opt; opt++) {
        char *argv[] = { nvsmi, (char *)*opt, NULL };
        if (nv_run(argv) == 0) return 0;
    }
    return -1;
}

int nv_lock_gpu(int mhz)
{
    char arg[64];
    snprintf(arg, sizeof(arg), "--lock-gpu-clocks=%d,%d", mhz, mhz);
    char *argv[] = { nvsmi, arg, NULL };
    return nv_run(argv) == 0 ? 0 : -1;
}

int nv_reset_gpu(void)
{
    char *try1[] = { nvsmi, "--reset-gpu-clocks", NULL };
    char *try2[] = { nvsmi, "-rgc",               NULL };
    if (nv_run(try1) == 0) return 0;
    return nv_run(try2) == 0 ? 0 : -1;
}
