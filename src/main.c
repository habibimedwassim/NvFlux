#include <stdio.h>
#include <string.h>
#include "nvflux.h"

#ifndef NVFLUX_VERSION
#define NVFLUX_VERSION "dev"
#endif

int main(int argc, char **argv) {
    if (argc >= 2 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
        printf("nvflux version %s\n", NVFLUX_VERSION);
        return 0;
    }
    return nvflux_run(argc, argv);
}