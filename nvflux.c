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

static int exec_capture(char *const argv[], char *outbuf, size_t outlen) {
    int pipefd[2];
    if (pipe(pipefd) < 0) return -1;
    pid_t pid = fork();
    if (pid < 0) { close(pipefd[0]); close(pipefd[1]); return -1; }
    if (pid == 0) {
        // child: set safe env, keep EUID (root) so nvidia-smi runs as root
        // Close read end
        close(pipefd[0]);
        // dup stdout to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        // minimal environment
        char *envp[] = { "PATH=/usr/bin:/usr/local/bin", "LC_ALL=C", NULL };
        // exec
        execve(nvsmipath, argv, envp);
        // if exec fails
        _exit(127);
    }
    // parent
    close(pipefd[1]);
    ssize_t r = 0;
    size_t off = 0;
    while ((r = read(pipefd[0], outbuf + off, outlen - 1 - off)) > 0) {
        off += (size_t)r;
        if (off >= outlen - 1) break;
    }
    outbuf[off] = '\0';
    close(pipefd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

static int parse_clocks(const char *txt, int *clocks, int max) {
    int count = 0;
    const char *p = txt;
    while (*p && count < max) {
        // skip non-digit
        while (*p && (*p < '0' || *p > '9')) p++;
        if (!*p) break;
        long v = strtol(p, (char**)&p, 10);
        clocks[count++] = (int)v;
    }
    // sort descending (simple)
    for (int i = 0; i < count - 1; ++i)
        for (int j = 0; j < count - 1 - i; ++j)
            if (clocks[j] < clocks[j+1]) {
                int t = clocks[j]; clocks[j] = clocks[j+1]; clocks[j+1] = t;
            }
    return count;
}

// get supported memory clocks
static int get_mem_clocks(int *clocks, int max) {
    char out[READ_BUF];
    char *argv[] = { nvsmipath, "--query-supported-clocks=memory", "--format=csv,noheader,nounits", NULL };
    int rc = exec_capture(argv, out, sizeof(out));
    if (rc < 0) return -1;
    return parse_clocks(out, clocks, max);
}

// get current memory clock (single int)
static int get_current_mem_clock(void) {
    char out[READ_BUF];
    char *argv[] = { nvsmipath, "--query-gpu=clocks.mem", "--format=csv,noheader,nounits", NULL };
    int rc = exec_capture(argv, out, sizeof(out));
    if (rc < 0) return -1;
    // parse first int
    const char *p = out;
    while (*p && (*p < '0' || *p > '9')) p++;
    if (!*p) return -1;
    return (int)strtol(p, NULL, 10);
}

static int run_nvsmicmd(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        // child: safe env
        char *envp[] = { "PATH=/usr/bin:/usr/local/bin", "LC_ALL=C", NULL };
        execve(nvsmipath, argv, envp);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

// safe wrappers for specific allowed operations
static int enable_persistence(void) {
    char *argv[] = { nvsmipath, "-pm", "1", NULL };
    return run_nvsmicmd(argv);
}
static int lock_memory_clocks(int memclk) {
    char arg[64];
    snprintf(arg, sizeof(arg), "--lock-memory-clocks=%d,%d", memclk, memclk);
    char *argv[] = { nvsmipath, arg, NULL };
    return run_nvsmicmd(argv);
}
static int reset_memory_clocks(void) {
    char *argv[] = { nvsmipath, "--reset-memory-clocks", NULL };
    return run_nvsmicmd(argv);
}

// check if given string is in allowed list
static int is_allowed(const char *cmd) {
    for (const char **p = allowed_cmds; *p; ++p) if (strcmp(*p, cmd) == 0) return 1;
    return 0;
}

int main(int argc, char **argv) {
    // must be invoked with one arg
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <performance|balanced|powersaver|auto|reset|status|clock|--restore>\n", argv[0]);
        return 1;
    }

    // discover nvidia-smi
    if (find_nvidia_smi() < 0) {
        fprintf(stderr, "Error: nvidia-smi not found in common locations or PATH\n");
        return 2;
    }

    // real user id (the person who invoked the program)
    uid_t real_uid = getuid();   // real uid (not effective)
    uid_t effective_uid = geteuid(); // should be root when installed setuid

    // require that this binary runs with effective uid root (setuid) OR run by root
    if (effective_uid != 0) {
        fprintf(stderr, "Error: this program needs to be installed setuid root (installer will do this).\n");
        return 3;
    }

    const char *cmd = argv[1];
    if (!is_allowed(cmd)) {
        fprintf(stderr, "Unknown or disallowed command: %s\n", cmd);
        return 4;
    }

    // handle status and clock and --restore without changing state
    if (strcmp(cmd, "status") == 0) {
        char mode[128] = {0};
        if (read_state(real_uid, mode, sizeof(mode))) {
            mode[0] = (char)toupper((unsigned char)mode[0]);
            printf("%s\n", mode);
        } else {
            printf("Auto\n");
        }
        return 0;
    }

    if (strcmp(cmd, "clock") == 0) {
        int clk = get_current_mem_clock();
        if (clk <= 0) { printf("Unavailable\n"); return 0; }
        printf("%d MHz\n", clk);
        return 0;
    }

    if (strcmp(cmd, "--restore") == 0) {
        char mode[128] = {0};
        if (!read_state(real_uid, mode, sizeof(mode))) return 0;
        // find clocks
        int clocks[MAX_CLOCKS];
        int count = get_mem_clocks(clocks, MAX_CLOCKS);
        if (count <= 0) { // can't detect, try reset
            if (strcmp(mode, "auto") == 0 || strcmp(mode, "reset") == 0) reset_memory_clocks();
            return 0;
        }
        int max = clocks[0];
        int mid = clocks[count/2];
        int low = clocks[count-1];
        if (strcmp(mode, "performance") == 0) {
            enable_persistence();
            lock_memory_clocks(max);
        } else if (strcmp(mode, "balanced") == 0) {
            enable_persistence();
            lock_memory_clocks(mid);
        } else if (strcmp(mode, "powersaver") == 0) {
            enable_persistence();
            lock_memory_clocks(low);
        } else {
            reset_memory_clocks();
        }
        return 0;
    }

    // For mode-changing commands: determine clocks and execute privileged commands
    int clocks[MAX_CLOCKS];
    int count = get_mem_clocks(clocks, MAX_CLOCKS);
    if (count <= 0) {
        // fail safe: if we can't get supported clocks, only allow reset
        if (strcmp(cmd, "auto") == 0 || strcmp(cmd, "reset") == 0) {
            reset_memory_clocks();
            write_state(real_uid, "auto");
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "Error: unable to query supported clocks (nvidia-smi failed)\n");
        return 5;
    }

    int max = clocks[0];
    int mid = clocks[count/2];
    int low = clocks[count-1];

    if (strcmp(cmd, "performance") == 0) {
        enable_persistence();
        lock_memory_clocks(max);
        write_state(real_uid, "performance");
        printf("OK\n");
        return 0;
    } else if (strcmp(cmd, "balanced") == 0) {
        enable_persistence();
        lock_memory_clocks(mid);
        write_state(real_uid, "balanced");
        printf("OK\n");
        return 0;
    } else if (strcmp(cmd, "powersaver") == 0) {
        enable_persistence();
        lock_memory_clocks(low);
        write_state(real_uid, "powersaver");
        printf("OK\n");
        return 0;
    } else if (strcmp(cmd, "auto") == 0 || strcmp(cmd, "reset") == 0) {
        reset_memory_clocks();
        write_state(real_uid, "auto");
        printf("OK\n");
        return 0;
    }

    // unreachable
    return 0;
}
