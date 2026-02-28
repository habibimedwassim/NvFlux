#pragma once

#include <stddef.h>
#include <sys/types.h>

/* Write mode string to ~/.local/state/nvflux/state, owned by real_uid.
   Returns 0 on success. */
int state_write(uid_t real_uid, const char *mode);

/* Read mode string into buf (len bytes).
   Returns 1 if a saved profile was found, 0 otherwise. */
int state_read(uid_t real_uid, char *buf, size_t len);
