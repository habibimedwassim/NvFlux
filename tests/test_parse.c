#include "nvidia.h"
#include "../include/nvflux.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Always-active assertion — not silenced by -DNDEBUG (Release builds). */
#define CHECK(expr) \
    do { if (!(expr)) { \
        fprintf(stderr, "FAIL  %s:%d  %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } } while (0)

#define RUN(name)  do { test_##name(); printf("  %-36s PASS\n", #name); } while (0)

/* ── nv_parse_clocks ────────────────────────────────────────────────────── */
static void test_parse_basic_sort(void)
{
    int c[10];
    int n = nv_parse_clocks("5760\n3504\n3004\n810\n", c, 10);
    CHECK(n == 4);
    CHECK(c[0] == 5760);
    CHECK(c[1] == 3504);
    CHECK(c[2] == 3004);
    CHECK(c[3] == 810);
}

static void test_parse_csv_with_units(void)
{
    /* nvidia-smi --format=csv,noheader (nounits off) produces "1234 MHz" */
    int c[10];
    int n = nv_parse_clocks("5760 MHz\n3504 MHz\n810 MHz\n", c, 10);
    CHECK(n == 3);
    CHECK(c[0] == 5760);
    CHECK(c[2] == 810);
}

static void test_parse_empty(void)
{
    int c[10];
    CHECK(nv_parse_clocks("",     c, 10) == 0);
    CHECK(nv_parse_clocks("\n\n", c, 10) == 0);
    CHECK(nv_parse_clocks("N/A",  c, 10) == 0);
}

static void test_parse_single(void)
{
    int c[10];
    int n = nv_parse_clocks("4006\n", c, 10);
    CHECK(n == 1);
    CHECK(c[0] == 4006);
}

static void test_parse_cap_at_max(void)
{
    int c[3];
    /* Collects the first 3 integers found, then sorts descending.
       From "100 200 300 400 500", first 3 are [100,200,300] → sorted [300,200,100]. */
    int n = nv_parse_clocks("100\n200\n300\n400\n500\n", c, 3);
    CHECK(n == 3);
    CHECK(c[0] == 300);
    CHECK(c[1] == 200);
    CHECK(c[2] == 100);
}

static void test_parse_already_sorted(void)
{
    int c[5];
    int n = nv_parse_clocks("900\n700\n500\n300\n100\n", c, 5);
    CHECK(n == 5);
    CHECK(c[0] == 900 && c[4] == 100);
}

static void test_parse_duplicates(void)
{
    int c[5];
    int n = nv_parse_clocks("1000\n1000\n1000\n", c, 5);
    CHECK(n == 3);
    CHECK(c[0] == 1000 && c[2] == 1000);
}

/* ── profile_from_str / profile_to_str ─────────────────────────────────── */
static void test_profile_roundtrip(void)
{
    Profile p;
    CHECK(profile_from_str("ultra",       &p) == 0 && p == PROFILE_ULTRA);
    CHECK(profile_from_str("performance", &p) == 0 && p == PROFILE_PERFORMANCE);
    CHECK(profile_from_str("balanced",    &p) == 0 && p == PROFILE_BALANCED);
    CHECK(profile_from_str("powersave",   &p) == 0 && p == PROFILE_POWERSAVE);
    CHECK(profile_from_str("auto",        &p) == 0 && p == PROFILE_AUTO);
    CHECK(profile_from_str("garbage",     &p) != 0);
}

static void test_profile_to_str(void)
{
    CHECK(strcmp(profile_to_str(PROFILE_ULTRA),       "ultra")       == 0);
    CHECK(strcmp(profile_to_str(PROFILE_PERFORMANCE), "performance") == 0);
    CHECK(strcmp(profile_to_str(PROFILE_BALANCED),    "balanced")    == 0);
    CHECK(strcmp(profile_to_str(PROFILE_POWERSAVE),   "powersave")   == 0);
    CHECK(strcmp(profile_to_str(PROFILE_AUTO),        "auto")        == 0);
}

/* ── entry point ─────────────────────────────────────────────────────────── */
int main(void)
{
    printf("nvflux unit tests\n\n");

    RUN(parse_basic_sort);
    RUN(parse_csv_with_units);
    RUN(parse_empty);
    RUN(parse_single);
    RUN(parse_cap_at_max);
    RUN(parse_already_sorted);
    RUN(parse_duplicates);
    RUN(profile_roundtrip);
    RUN(profile_to_str);

    printf("\nall tests passed.\n");
    return 0;
}
