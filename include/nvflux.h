#pragma once

#define NVFLUX_VERSION "2.0.0"

/* Clock profiles managed by nvflux. */
typedef enum {
    PROFILE_ULTRA       = 0,   /* lock mem + GPU core to max          */
    PROFILE_PERFORMANCE = 1,   /* lock mem to max,  GPU core adaptive  */
    PROFILE_BALANCED    = 2,   /* lock mem to mid,  GPU core adaptive  */
    PROFILE_POWERSAVE   = 3,   /* lock mem to min,  GPU core adaptive  */
    PROFILE_AUTO        = 4,   /* reset all locks (driver-managed)    */
} Profile;

/* Returns 0 on success, -1 if s is not a recognised profile name. */
int         profile_from_str(const char *s, Profile *out);
const char *profile_to_str(Profile p);
