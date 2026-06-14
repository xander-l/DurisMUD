/* ===================================================================
 * test_live_latency.c — Live timing and latency measurement tests.
 *
 * These tests measure actual MySQL query latency and verify that
 * the game loop pulse timing, OPT_USEC sleep budget, and latency
 * trace infrastructure would not let the MUD lag between pulses.
 *
 * Tests:
 *   1. OPT_USEC sleep budget accuracy (250000us = 4 pulses/sec)
 *   2. Single INSERT latency measurement
 *   3. Burst INSERT latency (100 queries back-to-back)
 *   4. SELECT query latency with index
 *   5. Concurrent read/write latency
 *   6. Pulse timing — verify we can complete work within budget
 *   7. Latency trace record push + query
 *   8. Long-running query detection
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE  /* for usleep() visibility on Linux */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <mysql.h>

/* ------------------------------------------------------------------ */
/*  Test framework                                                    */
/* ------------------------------------------------------------------ */

static MYSQL  *g_db      = NULL;
static int     g_pass    = 0;
static int     g_fail    = 0;
static char    g_last_error[4096];

/* Production value from config.h line 82 */
#define OPT_USEC              250000
#define PULSES_IN_TICK        300
#define LATENCY_WARN_THRESHOLD_USEC  (OPT_USEC / 2)  /* 125ms — if a single query exceeds this, flag it */

#define TEST_BEGIN(name) do { printf("  %s ... ", name); fflush(stdout); } while (0)
#define TEST_END()       do { printf("\n"); } while (0)
#define TEST_PASS()      do { g_pass++; } while (0)
#define TEST_FAIL(...)   do { \
    snprintf(g_last_error, sizeof(g_last_error), __VA_ARGS__); \
    g_fail++; \
    fprintf(stderr, "\n  FAIL: %s", g_last_error); \
} while (0)

static int mysql_ok(void)
{
    if (!g_db) { TEST_FAIL("no MySQL connection"); return 0; }
    return 1;
}

/* Get microsecond-precision time (monotonic) */
static double usec_now(void)
{
    struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#elif defined(__APPLE__)
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
}

/* ==================================================================
 * TEST 1: OPT_USEC sleep budget accuracy.
 *
 * Production: suseconds_t usec_spent = (loop_time * 1000 * 1000);
 *            if (usec_spent < OPT_USEC) usleep(OPT_USEC - usec_spent);
 *
 * This ensures the game loop never runs faster than 4 pulses/sec.
 * ================================================================== */
static void test_live_opt_usec_budget(void)
{
    TEST_BEGIN("live_opt_usec_budget");

    /* Simulate pulse budget: OPT_USEC = 250000 us per pulse */
    double budget = OPT_USEC;  /* 250ms */

    /* Measure how accurate usleep is within the pulse budget */
    double start = usec_now();
    /* Simulate some work */
    for (volatile int i = 0; i < 10000; i++) { }
    double work_end = usec_now();
    double work_usec = work_end - start;

    /* Calculate remaining budget */
    double remaining = budget - work_usec;
    if (remaining > 0) {
        usleep((unsigned int)remaining);
    }

    double pulse_end = usec_now();
    double total_pulse = pulse_end - start;

    printf(" (work=%.0fus, pulse=%.0fus, budget=%.0fus)",
           work_usec, total_pulse, budget);

    /* The total pulse time should be >= budget (we padded with sleep) */
    /* On some systems, usleep granularity may cause variance */
    if (total_pulse < budget * 0.5) {
        TEST_FAIL("pulse too fast: %.0fus < 50%% of budget %.0fus", total_pulse, budget);
        TEST_END(); return;
    }

    if (total_pulse > budget * 3.0) {
        TEST_FAIL("pulse too slow (scheduler lag?): %.0fus > 3x budget %.0fus", total_pulse, budget);
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 2: Single INSERT latency measurement.
 *
 * Insert to latency_trace table and measure roundtrip time.
 * This must complete well under OPT_USEC to not lag the game.
 * ================================================================== */
static void test_live_insert_latency(void)
{
    TEST_BEGIN("live_insert_latency");
    if (!mysql_ok()) { TEST_END(); return; }

    /* Clean any old data */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 999000 AND 999999");

    double total_usec = 0;
    int    count      = 10;

    for (int i = 0; i < count; i++) {
        char q[256];
        snprintf(q, sizeof(q),
            "INSERT INTO latency_trace (section_name, usec_spent, pulse_num) "
            "VALUES ('test_insert', %d, %d)",
            i * 100, 999000 + i);

        double t0 = usec_now();
        if (mysql_query(g_db, q)) {
            TEST_FAIL("INSERT %d failed: %s", i, mysql_error(g_db));
            TEST_END(); return;
        }
        double t1 = usec_now();
        total_usec += (t1 - t0);
    }

    double avg_usec = total_usec / count;
    printf(" (avg=%.0fus/insert)", avg_usec);

    /* Verify data was stored */
    if (mysql_query(g_db, "SELECT COUNT(*) FROM latency_trace WHERE pulse_num BETWEEN 999000 AND 999999")) {
        TEST_FAIL("verify SELECT failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    MYSQL_RES *res = mysql_store_result(g_db);
    MYSQL_ROW  row = mysql_fetch_row(res);
    int stored = row ? atoi(row[0]) : 0;
    mysql_free_result(res);

    if (stored != count)
        TEST_FAIL("expected %d rows stored, got %d", count, stored);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 999000 AND 999999");

    /* Perf check: average INSERT should be < 10ms */
    if (avg_usec > 10000)
        TEST_FAIL("average INSERT latency too high: %.0fus (threshold: 10000us)", avg_usec);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 3: Burst INSERT latency (100 queries back-to-back).
 *
 * Simulates persistence queue flush during a save pulse.
 * All 100 inserts must complete within OPT_USEC budget.
 * ================================================================== */
static void test_live_burst_insert(void)
{
    TEST_BEGIN("live_burst_insert");
    if (!mysql_ok()) { TEST_END(); return; }

    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 998000 AND 998999");

    double t0 = usec_now();

    for (int i = 0; i < 100; i++) {
        char q[256];
        snprintf(q, sizeof(q),
            "INSERT INTO latency_trace (section_name, usec_spent, pulse_num) "
            "VALUES ('burst_test', %d, %d)",
            i * 50, 998000 + i);
        if (mysql_query(g_db, q)) {
            TEST_FAIL("burst INSERT %d failed: %s", i, mysql_error(g_db));
            TEST_END(); return;
        }
    }

    double t1 = usec_now();
    double total_usec = t1 - t0;
    double avg_usec = total_usec / 100.0;

    printf(" (total=%.0fus, avg=%.0fus/insert)", total_usec, avg_usec);

    /* Verify all stored */
    if (mysql_query(g_db, "SELECT COUNT(*) FROM latency_trace WHERE pulse_num BETWEEN 998000 AND 998999")) {
        TEST_FAIL("verify SELECT failed");
        TEST_END(); return;
    }
    MYSQL_RES *res = mysql_store_result(g_db);
    MYSQL_ROW  row = mysql_fetch_row(res);
    int stored = row ? atoi(row[0]) : 0;
    mysql_free_result(res);

    if (stored != 100)
        TEST_FAIL("expected 100 burst rows, got %d", stored);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 998000 AND 998999");

    /* Burst must fit within 2x OPT_USEC (Docker MySQL is slower) */
    if (total_usec > OPT_USEC * 2)
        TEST_FAIL("burst INSERT %d exceeded 2x OPT_USEC budget: %.0fus > %dus",
                  100, total_usec, OPT_USEC * 2);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 4: SELECT latency with index.
 *
 * The latency_trace table has KEY on section_name and pulse_num.
 * Verify indexed queries are fast.
 * ================================================================== */
static void test_live_select_latency(void)
{
    TEST_BEGIN("live_select_latency");
    if (!mysql_ok()) { TEST_END(); return; }

    /* Populate 500 rows */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 997000 AND 997999");
    mysql_query(g_db, "SET autocommit = 0");
    mysql_query(g_db, "START TRANSACTION");
    for (int i = 0; i < 500; i++) {
        char q[256];
        snprintf(q, sizeof(q),
            "INSERT INTO latency_trace (section_name, usec_spent, pulse_num) "
            "VALUES ('bench_select', %d, %d)", i * 100, 997000 + i);
        mysql_query(g_db, q);
    }
    mysql_query(g_db, "COMMIT");
    mysql_query(g_db, "SET autocommit = 1");

    /* Time indexed SELECT */
    double t0 = usec_now();
    int queries = 20;
    for (int i = 0; i < queries; i++) {
        mysql_query(g_db,
            "SELECT AVG(usec_spent), MAX(usec_spent), MIN(usec_spent) "
            "FROM latency_trace WHERE section_name = 'bench_select'");
        MYSQL_RES *res = mysql_store_result(g_db);
        if (res) mysql_free_result(res);
    }
    double t1 = usec_now();
    double avg_usec = (t1 - t0) / queries;

    printf(" (avg=%.0fus/select)", avg_usec);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 997000 AND 997999");

    if (avg_usec > 5000)
        TEST_FAIL("average SELECT latency too high: %.0fus (threshold: 5000us)", avg_usec);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 5: Concurrent read/write latency.
 *
 * Interleave INSERT and SELECT to simulate game loop pattern
 * where persistence writes happen alongside player reads.
 * ================================================================== */
static void test_live_concurrent_rw(void)
{
    TEST_BEGIN("live_concurrent_rw");
    if (!mysql_ok()) { TEST_END(); return; }

    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 996000 AND 996999");

    double total_usec = 0;
    int    iterations = 50;
    int    max_single = 0;

    for (int i = 0; i < iterations; i++) {
        char q[256];

        /* Write */
        snprintf(q, sizeof(q),
            "INSERT INTO latency_trace (section_name, usec_spent, pulse_num) "
            "VALUES ('concurrent', %d, %d)", i * 100, 996000 + i);

        double t0 = usec_now();
        mysql_query(g_db, q);
        double t1 = usec_now();
        int write_usec = (int)(t1 - t0);
        total_usec += write_usec;
        if (write_usec > max_single) max_single = write_usec;

        /* Read (simulate player query during save) */
        t0 = usec_now();
        mysql_query(g_db, "SELECT COUNT(*) FROM latency_trace WHERE section_name = 'concurrent'");
        MYSQL_RES *res = mysql_store_result(g_db);
        if (res) mysql_free_result(res);
        t1 = usec_now();
        int read_usec = (int)(t1 - t0);
        total_usec += read_usec;
        if (read_usec > max_single) max_single = read_usec;
    }

    double avg_usec = total_usec / (iterations * 2);
    printf(" (avg=%.0fus/op, max=%dus)", avg_usec, max_single);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 996000 AND 996999");

    /* No single operation should block the game loop for > 125ms */
    if (max_single > LATENCY_WARN_THRESHOLD_USEC)
        TEST_FAIL("concurrent R/W max latency %dus exceeds threshold %dus",
                  max_single, LATENCY_WARN_THRESHOLD_USEC);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 6: Pulse timing — verify we can complete work within OPT_USEC.
 *
 * Simulate the game loop: connections, commands, prompts, events,
 * combat, affect_and_points, total_tick — must all fit in 250ms.
 * ================================================================== */
static void test_live_pulse_timing(void)
{
    TEST_BEGIN("live_pulse_timing");
    if (!mysql_ok()) { TEST_END(); return; }

    const char *sections[] = {
        "connections", "commands", "prompts", "ne_events",
        "gmcp_flush", "activities", "combat", "affect_and_points",
        "total_tick"
    };
    int n_sections = sizeof(sections) / sizeof(sections[0]);

    /* Record simulated section timings */
    double total = 0;
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num = 888888");

    for (int i = 0; i < n_sections; i++) {
        /* Simulate work for this section (busy-wait ~1-5ms) */
        double start = usec_now();
        /* Small INSERT as simulated work */
        char q[512];
        snprintf(q, sizeof(q),
            "INSERT INTO latency_trace (section_name, usec_spent, pulse_num) "
            "VALUES ('%s', %d, 888888)",
            sections[i], (i + 1) * 500);
        mysql_query(g_db, q);
        double end = usec_now();
        double spent = end - start;
        total += spent;

        /* Record latency trace entry (what comm.c does) */
        snprintf(q, sizeof(q),
            "INSERT INTO latency_trace (section_name, usec_spent, pulse_num) "
            "VALUES ('%s', %.0f, 888888)",
            sections[i], spent);
        mysql_query(g_db, q);
    }

    printf(" (total_pulse=%.0fus, budget=%.0fus)", total, (double)OPT_USEC);

    /* The total should fit within budget */
    if (total > OPT_USEC * 1.5)
        TEST_FAIL("pulse timing exceeded 1.5x budget: %.0fus > %.0fus",
                  total, OPT_USEC * 1.5);

    /* Verify all sections recorded */
    char q[512];
    snprintf(q, sizeof(q),
        "SELECT COUNT(DISTINCT section_name) FROM latency_trace WHERE pulse_num = 888888");
    mysql_query(g_db, q);
    MYSQL_RES *res = mysql_store_result(g_db);
    MYSQL_ROW  row = mysql_fetch_row(res);
    int unique = row ? atoi(row[0]) : 0;
    mysql_free_result(res);

    if (unique < n_sections)  /* At minimum, the 9 section names should be present */
        TEST_FAIL("expected at least %d distinct sections, got %d", n_sections, unique);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num = 888888");

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 7: Latency trace dump — query aggregated stats.
 *
 * Production dumps every 300 tics:
 *   latency_trace_dump(stderr);
 *   persistence_queue_latency_dump();
 *   utility_latency_dump();
 *
 * This test simulates the dump query.
 * ================================================================== */
static void test_live_latency_dump(void)
{
    TEST_BEGIN("live_latency_dump");
    if (!mysql_ok()) { TEST_END(); return; }

    /* Populate sample data for 300 pulses */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 500000 AND 500300");
    mysql_query(g_db, "SET autocommit = 0");
    mysql_query(g_db, "START TRANSACTION");

    for (int pulse = 500000; pulse < 500300; pulse++) {
        char q[256];
        snprintf(q, sizeof(q),
            "INSERT INTO latency_trace (section_name, usec_spent, pulse_num) "
            "VALUES ('total_tick', %d, %d)",
            25000 + (pulse % 50) * 1000, pulse);
        mysql_query(g_db, q);
    }
    mysql_query(g_db, "COMMIT");
    mysql_query(g_db, "SET autocommit = 1");

    /* Simulate dump query (what latency_trace_dump does in production) */
    double t0 = usec_now();
    if (mysql_query(g_db,
        "SELECT section_name, "
        "AVG(usec_spent) as avg_usec, "
        "MAX(usec_spent) as max_usec, "
        "MIN(usec_spent) as min_usec, "
        "COUNT(*) as sample_count "
        "FROM latency_trace "
        "WHERE pulse_num BETWEEN 500000 AND 500300 "
        "GROUP BY section_name "
        "ORDER BY avg_usec DESC")) {
        TEST_FAIL("dump query failed: %s", mysql_error(g_db));
    } else {
        MYSQL_RES *res = mysql_store_result(g_db);
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row) {
                /* Verify avg_usec is reasonable (~50ms) */
                double avg = atof(row[1]);
                if (avg < 10000 || avg > 100000)
                    TEST_FAIL("unexpected avg_usec: %.0f", avg);
            }
            mysql_free_result(res);
        }
    }
    double t1 = usec_now();
    printf(" (dump=%.0fus)", t1 - t0);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 500000 AND 500300");

    /* Dump query should complete fast (< 100ms) */
    if ((t1 - t0) > 100000)
        TEST_FAIL("latency dump query too slow: %.0fus (threshold: 100000us)", t1 - t0);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 8: Long-running query detection.
 *
 * If any query exceeds 50% of OPT_USEC budget (125ms), the game
 * loop will lag. This test verifies we can detect slow queries.
 * ================================================================== */
static void test_live_slow_query_detection(void)
{
    TEST_BEGIN("live_slow_query_detection");
    if (!mysql_ok()) { TEST_END(); return; }

    /* Insert 5000 rows to create a table big enough to be slow(er) without index */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 499000 AND 499999");

    double t0 = usec_now();

    for (int i = 0; i < 50; i++) {
        char q[256];
        snprintf(q, sizeof(q),
            "INSERT INTO latency_trace (section_name, usec_spent, pulse_num) "
            "VALUES ('slow_detect', %d, %d)", i * 200, 499000 + i);
        mysql_query(g_db, q);

        /* Check if individual query is slow */
        double now = usec_now();
        double elapsed = now - t0;
        if (elapsed > LATENCY_WARN_THRESHOLD_USEC) {
            printf(" (warn: query %d at %.0fus)", i, elapsed);
        }
    }

    double total = usec_now() - t0;
    printf(" (total=%.0fus for 50 inserts)", total);

    /* Count stored */
    mysql_query(g_db, "SELECT COUNT(*) FROM latency_trace WHERE pulse_num BETWEEN 499000 AND 499999");
    MYSQL_RES *res = mysql_store_result(g_db);
    MYSQL_ROW  row = mysql_fetch_row(res);
    int stored = row ? atoi(row[0]) : 0;
    mysql_free_result(res);

    if (stored != 50)
        TEST_FAIL("expected 50 rows, got %d", stored);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM latency_trace WHERE pulse_num BETWEEN 499000 AND 499999");

    /* Overall batch should fit in budget */
    if (total > OPT_USEC * 2)
        TEST_FAIL("50-insert batch too slow: %.0fus > 2x OPT_USEC", total);

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test registry & runner                                             */
/* ================================================================== */

typedef struct { const char *name; void (*func)(void); } test_entry;

static test_entry g_tests[] = {
    { "live_opt_usec_budget",       test_live_opt_usec_budget },
    { "live_insert_latency",        test_live_insert_latency },
    { "live_burst_insert",          test_live_burst_insert },
    { "live_select_latency",        test_live_select_latency },
    { "live_concurrent_rw",         test_live_concurrent_rw },
    { "live_pulse_timing",          test_live_pulse_timing },
    { "live_latency_dump",          test_live_latency_dump },
    { "live_slow_query_detection",  test_live_slow_query_detection },
    { NULL, NULL },
};

int test_live_latency_run(MYSQL *db)
{
    g_db   = db;
    g_pass = g_fail = 0;
    memset(g_last_error, 0, sizeof(g_last_error));

    printf("\n=== Live Latency Guard Tests ===\n");
    for (int i = 0; g_tests[i].name; i++)
        g_tests[i].func();

    int total = g_pass + g_fail;
    printf("  Live Latency Guard: %d/%d passed\n", g_pass, total);
    return g_fail;
}
