#pragma once

#include <stddef.h>

#define NV_MAX_CLOCKS 128

/* Find and cache the nvidia-smi binary path.  Call once at startup.
   Returns 0 on success, -1 if not found. */
int nv_init(void);

/* Fork + execv argv[0], capturing stdout+stderr into buf.
   Returns the process exit code (0 = success), -1 on exec failure. */
int nv_capture(char *const argv[], char *buf, size_t len);

/* Fork + execv argv[0] with inherited stdio.
   Returns the process exit code (0 = success), -1 on exec failure. */
int nv_run(char *const argv[]);

/* Parse all decimal integers from CSV text into clocks[], sorted descending.
   Returns the number of values found (capped at max). */
int nv_parse_clocks(const char *txt, int *clocks, int max);

/* Query supported memory clock tiers, sorted descending.
   Returns count, or -1 on failure. */
int nv_mem_clocks(int *clocks, int max);

/* Query supported GPU core clock values, sorted descending.
   Returns count, or -1 on failure. */
int nv_gpu_clocks(int *clocks, int max);

/* Query the current memory clock in MHz.  Returns -1 on failure. */
int nv_current_mem_clock(void);

/* Enable persistence mode (-pm 1).  Returns 0 on success. */
int nv_persistence_enable(void);

/* Lock memory clocks to exactly mhz MHz.
   Falls back to --lock-memory-clocks-deferred on Hopper+ GPUs.
   Returns 0 on success. */
int nv_lock_mem(int mhz);

/* Reset memory clocks to driver default (tries -rmc then -rmcd).
   Returns 0 on success. */
int nv_reset_mem(void);

/* Lock GPU core clocks to exactly mhz MHz.  Returns 0 on success. */
int nv_lock_gpu(int mhz);

/* Reset GPU core clocks to driver default.  Returns 0 on success. */
int nv_reset_gpu(void);

/* Verify that at least one NVIDIA GPU is accessible.
   Prints an error and returns -1 if the driver is not loaded. */
int nv_check(void);
