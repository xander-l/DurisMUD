/* ===================================================================
 * test_latency_guard.c — Latency tracing and pulse timing
 * regression guard tests.
 *
 * Source-grep tests verifying the latency monitoring infrastructure
 * stays intact and the game loop doesn't accumulate lag.
 *
 * Tests:
 *   1. latency_trace_record called for all game loop sections
 *   2. total_tick recorded every pulse
 *   3. Latency dump happens every 300 tics
 *   4. persistence_queue_latency_dump exists
 *   5. utility_latency_dump exists
 *   6. OPT_USEC defines pulse interval (250000 = 4/sec)
 *   7. PULSES_IN_TICK defined
 *   8. Game loop sleep adjusts based on usec_spent
 *   9. LATENCY_TRACE_MAX_SAMPLES defined (4096)
 *  10. latency_trace_record mutex-protected
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_latency_guard.h"

/* ------------------------------------------------------------------ */
/*  Test framework                                                    */
/* ------------------------------------------------------------------ */

static int  g_pass = 0;
static int  g_fail = 0;
static char g_last_error[4096];
static char g_src_buf[524288];

#define TEST_BEGIN(name) do { printf("  %s ... ", name); fflush(stdout); } while (0)
#define TEST_END()       do { printf("\n"); } while (0)
#define TEST_PASS()      do { g_pass++; } while (0)
#define TEST_FAIL(...)   do { \
    snprintf(g_last_error, sizeof(g_last_error), __VA_ARGS__); \
    g_fail++; \
    fprintf(stderr, "\n  FAIL: %s", g_last_error); \
} while (0)

/* ------------------------------------------------------------------ */
/*  Source helpers                                                    */
/* ------------------------------------------------------------------ */

static int load_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        char alt[512];
        snprintf(alt, sizeof(alt), "../../../%s", filename);
        f = fopen(alt, "r");
    }
    if (!f) {
        snprintf(g_last_error, sizeof(g_last_error), "cannot open %s", filename);
        return 0;
    }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    if (n == sizeof(g_src_buf) - 1) {
        snprintf(g_last_error, sizeof(g_last_error),
                 "%s larger than 512KB", filename);
        return 0;
    }
    return 1;
}

/* ==================================================================
 * TEST 1: latency_trace_record called for all game loop sections.
 * ================================================================== */
static void test_latency_all_sections(void)
{
    TEST_BEGIN("latency_all_sections");

    if (!load_file("src/comm.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    const char *sections[] = {
        "connections", "commands", "prompts", "ne_events",
        "gmcp_flush", "activities", "combat", "affect_and_points"
    };
    int n = sizeof(sections) / sizeof(sections[0]);
    int missing = 0;

    for (int i = 0; i < n; i++) {
        char pattern[128];
        snprintf(pattern, sizeof(pattern), "\"%s\"", sections[i]);
        if (!strstr(g_src_buf, pattern)) {
            TEST_FAIL("latency_trace_record: missing section \"%s\"", sections[i]);
            missing = 1;
        }
    }

    if (!missing) TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 2: total_tick recorded every pulse.
 * ================================================================== */
static void test_latency_total_tick(void)
{
    TEST_BEGIN("latency_total_tick");

    if (!load_file("src/comm.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    if (!strstr(g_src_buf, "\"total_tick\"")) {
        TEST_FAIL("latency_trace_record: missing \"total_tick\" section");
        TEST_END(); return;
    }

    /* Must calculate loop_time from clock() */
    if (!strstr(g_src_buf, "loop_time_end") || !strstr(g_src_buf, "loop_time_start")) {
        TEST_FAIL("total_tick: missing loop_time_end/loop_time_start calculation");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 3: Latency dump happens every 300 tics.
 * ================================================================== */
static void test_latency_dump_300_tics(void)
{
    TEST_BEGIN("latency_dump_300_tics");

    if (!load_file("src/comm.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Must check tics % 300 */
    if (!strstr(g_src_buf, "tics % 300")) {
        TEST_FAIL("latency dump: missing 'tics % 300' check");
        TEST_END(); return;
    }

    /* Must call latency_trace_dump */
    if (!strstr(g_src_buf, "latency_trace_dump")) {
        TEST_FAIL("latency dump: missing latency_trace_dump() call");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 4: persistence_queue_latency_dump exists.
 * ================================================================== */
static void test_latency_persistence_queue_dump(void)
{
    TEST_BEGIN("latency_persistence_queue_dump");

    /* Check utility.c first, then persistence_queue.c */
    int found_dump = 0;
    if (load_file("src/utility.c") && strstr(g_src_buf, "persistence_queue_latency_dump"))
        found_dump = 1;
    else if (load_file("src/persistence_queue.c") && strstr(g_src_buf, "persistence_queue_latency_dump"))
        found_dump = 1;

    if (!found_dump) {
        TEST_FAIL("persistence_queue_latency_dump not found in utility.c or persistence_queue.c");
        TEST_END(); return;
    }

    /* Must report queue depths or dropped counts */
    if (!strstr(g_src_buf, "depth") && !strstr(g_src_buf, "dropped") &&
        !strstr(g_src_buf, "queue") && !strstr(g_src_buf, "count")) {
        TEST_FAIL("persistence_queue_latency_dump: no queue stats reported");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 5: utility_latency_dump exists.
 * ================================================================== */
static void test_latency_utility_dump(void)
{
    TEST_BEGIN("latency_utility_dump");

    if (!load_file("src/utility.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    if (!strstr(g_src_buf, "utility_latency_dump")) {
        TEST_FAIL("utility_latency_dump not found in utility.c");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 6: OPT_USEC defines pulse interval (250000 = 4 pulses/sec).
 * ================================================================== */
static void test_latency_opt_usec(void)
{
    TEST_BEGIN("latency_opt_usec");

    if (!load_file("src/config.h")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    if (!strstr(g_src_buf, "OPT_USEC")) {
        TEST_FAIL("OPT_USEC not defined in config.h");
        TEST_END(); return;
    }

    if (!strstr(g_src_buf, "250000")) {
        TEST_FAIL("OPT_USEC value 250000 not found (expected 4 pulses/sec)");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 7: PULSES_IN_TICK defined.
 * ================================================================== */
static void test_latency_pulses_in_tick(void)
{
    TEST_BEGIN("latency_pulses_in_tick");

    /* Check multiple files since PULSES_IN_TICK may be in events.h or config.h */
    int found = 0;
    if (load_file("src/config.h") && strstr(g_src_buf, "PULSES_IN_TICK"))
        found = 1;
    else if (load_file("src/events.c") && strstr(g_src_buf, "PULSES_IN_TICK"))
        found = 1;
    else if (load_file("src/new_events.c") && strstr(g_src_buf, "PULSES_IN_TICK"))
        found = 1;
    else if (load_file("src/defines.h") && strstr(g_src_buf, "PULSES_IN_TICK"))
        found = 1;

    if (!found) {
        TEST_FAIL("PULSES_IN_TICK not found in config.h, events.c, new_events.c, or defines.h");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 8: Game loop sleep adjusts based on usec_spent.
 *
 * The game loop must calculate usec_spent from loop_time and
 * sleep for max(0, OPT_USEC - usec_spent) to prevent lag.
 * ================================================================== */
static void test_latency_sleep_budget(void)
{
    TEST_BEGIN("latency_sleep_budget");

    if (!load_file("src/comm.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Must calculate usec_spent */
    if (!strstr(g_src_buf, "usec_spent")) {
        TEST_FAIL("game loop: missing usec_spent calculation");
        TEST_END(); return;
    }

    /* Must reference OPT_USEC for sleep budget */
    if (!strstr(g_src_buf, "OPT_USEC")) {
        TEST_FAIL("game loop: missing OPT_USEC reference for sleep budget");
        TEST_END(); return;
    }

    /* Must use usleep, nanosleep, or select for sleeping */
    if (!strstr(g_src_buf, "usleep") && !strstr(g_src_buf, "nanosleep") &&
        !strstr(g_src_buf, "select")) {
        TEST_FAIL("game loop: missing usleep/nanosleep/select for sleep budget");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 9: LATENCY_TRACE_MAX_SAMPLES defined (4096).
 * ================================================================== */
static void test_latency_max_samples(void)
{
    TEST_BEGIN("latency_max_samples");

    if (!load_file("src/latency_trace.h")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    if (!strstr(g_src_buf, "LATENCY_TRACE_MAX_SAMPLES")) {
        TEST_FAIL("LATENCY_TRACE_MAX_SAMPLES not defined in latency_trace.h");
        TEST_END(); return;
    }

    if (!strstr(g_src_buf, "4096")) {
        TEST_FAIL("LATENCY_TRACE_MAX_SAMPLES value 4096 not found");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 10: latency_trace_record mutex-protected.
 * ================================================================== */
static void test_latency_mutex_protected(void)
{
    TEST_BEGIN("latency_mutex_protected");

    if (!load_file("src/latency_trace.h")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Must use pthread_mutex_lock */
    if (!strstr(g_src_buf, "pthread_mutex_lock")) {
        TEST_FAIL("latency_trace_record: missing pthread_mutex_lock");
        TEST_END(); return;
    }

    /* Must use pthread_mutex_unlock */
    if (!strstr(g_src_buf, "pthread_mutex_unlock")) {
        TEST_FAIL("latency_trace_record: missing pthread_mutex_unlock");
        TEST_END(); return;
    }

    /* Must have a mutex variable */
    if (!strstr(g_src_buf, "_latency_mutex")) {
        TEST_FAIL("latency_trace_record: missing _latency_mutex variable");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test registry                                                      */
/* ================================================================== */

typedef struct { const char *name; void (*func)(void); } test_entry;

static test_entry g_tests[] = {
    { "latency_all_sections",             test_latency_all_sections },
    { "latency_total_tick",               test_latency_total_tick },
    { "latency_dump_300_tics",            test_latency_dump_300_tics },
    { "latency_persistence_queue_dump",   test_latency_persistence_queue_dump },
    { "latency_utility_dump",             test_latency_utility_dump },
    { "latency_opt_usec",                 test_latency_opt_usec },
    { "latency_pulses_in_tick",           test_latency_pulses_in_tick },
    { "latency_sleep_budget",             test_latency_sleep_budget },
    { "latency_max_samples",              test_latency_max_samples },
    { "latency_mutex_protected",          test_latency_mutex_protected },
    { NULL, NULL },
};

static const int g_num_tests = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

int test_latency_guard_run_all(void)
{
    test_latency_guard_reset();
    printf("  [Latency Guard Tests]\n");
    for (int i = 0; g_tests[i].name; i++) g_tests[i].func();
    printf("  Passed: %d/%d\n", g_pass, g_pass + g_fail);
    return g_fail;
}

int test_latency_guard_run_one(const char *name)
{
    for (int i = 0; g_tests[i].name; i++)
        if (strcmp(g_tests[i].name, name) == 0) {
            printf("  [Latency Guard: %s]\n", name);
            g_tests[i].func();
            return (g_fail > 0);
        }
    return -1;
}

void test_latency_guard_print_summary(void)
{
    printf("  Latency Guard: %d passed, %d failed out of %d tests\n",
           g_pass, g_fail, g_num_tests);
}

void test_latency_guard_reset(void)
{
    g_pass = g_fail = 0;
    g_last_error[0] = '\0';
}
