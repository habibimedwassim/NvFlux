#pragma once

#include "../include/nvflux.h"
#include <sys/types.h>

/* Apply profile p, printing a result line to stdout.
   Requires that nv_init() and nv_check() have already succeeded.
   Returns 0 on success, -1 on failure. */
int profile_apply(Profile p);
