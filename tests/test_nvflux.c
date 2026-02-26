#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvflux.h"

static int expect(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        return 1;
    }
    return 0;
}

int main(void) {
    int errors = 0;

    // basic parse test: unsorted input -> expect descending sorted output
    const char *txt = "3000\n7000,5000\n";
    int clocks[16] = {0};
    int n = nvflux_parse_clocks(txt, clocks, 16);
    errors += expect(n == 3, "parse count should be 3");
    errors += expect(clocks[0] == 7000, "largest should be first (7000)");
    errors += expect(clocks[1] == 5000, "second should be 5000");
    errors += expect(clocks[2] == 3000, "third should be 3000");

    // boundary test: empty input
    int clocks2[4] = {0};
    n = nvflux_parse_clocks("", clocks2, 4);
    errors += expect(n == 0, "empty input -> 0 clocks");

    // single value
    int clocks3[4] = {0};
    n = nvflux_parse_clocks("1500\n", clocks3, 4);
    errors += expect(n == 1, "single value -> 1 clock");
    errors += expect(clocks3[0] == 1500, "single value should be 1500");

    // graphics-style output with spaces and units stripped
    int clocks4[16] = {0};
    n = nvflux_parse_clocks("2100\n2085\n1980\n1410\n210\n", clocks4, 16);
    errors += expect(n == 5, "graphics clocks count should be 5");
    errors += expect(clocks4[0] == 2100, "graphics max should be 2100");
    errors += expect(clocks4[4] == 210, "graphics min should be 210");

    // duplicates (common in graphics clock queries across mem levels)
    int clocks5[16] = {0};
    n = nvflux_parse_clocks("1800\n1200\n1800\n600\n1200\n", clocks5, 16);
    errors += expect(n == 5, "duplicates: count should be 5");
    errors += expect(clocks5[0] == 1800, "duplicates: first should be 1800");
    errors += expect(clocks5[n-1] == 600, "duplicates: last should be 600");

    // max buffer limit
    int clocks6[2] = {0};
    n = nvflux_parse_clocks("100\n200\n300\n400\n", clocks6, 2);
    errors += expect(n == 2, "max=2 should limit to 2 clocks");

    return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}