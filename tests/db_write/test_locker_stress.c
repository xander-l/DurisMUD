/* ===================================================================
 * test_locker_stress.c — Locker save/load, enter/exit, chest access,
 * and multi-player stress regression tests.
 *
 * Source-grep + SQL-pattern tests.  No MySQL or MUD runtime needed.
 * The 10K items/locker live stress test is deferred to Phase 9c.
 *
 * Tests:
 *   1. locker_items INSERT has all columns (v19+material)
 *   2. Locker save uses DELETE-before-INSERT pattern
 *   3. Locker chest save/load exists (not stubs)
 *   4. Locker enter/exit path in storage_lockers.c
 *   5. Locker private chest password verification
 *   6. Locker load two-pass item placement
 *   7. sql_locker_exists checks DB before create
 *   8. Locker transaction wrapping
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_locker_stress.h"

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
 * TEST 1: locker_items INSERT has all columns (v19+material).
 * ================================================================== */
static void test_locker_items_columns(void)
{
    TEST_BEGIN("locker_items_columns");

    if (!load_file("src/sql_player.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    const char *hit = strstr(g_src_buf, "INSERT INTO locker_items");
    if (!hit) {
        TEST_FAIL("INSERT INTO locker_items not found in sql_player.c");
        TEST_END(); return;
    }

    /* v19 columns */
    const char *v19[] = { "wear_flags", "item_type",
        "bitvector1", "bitvector2", "bitvector3", "bitvector4", "bitvector5" };
    int n19 = sizeof(v19) / sizeof(v19[0]);
    for (int i = 0; i < n19; i++) {
        if (!strstr(hit, v19[i])) {
            TEST_FAIL("locker_items INSERT missing v19 column: %s", v19[i]);
            TEST_END(); return;
        }
    }

    /* item_material */
    if (!strstr(hit, "item_material")) {
        TEST_FAIL("locker_items INSERT missing item_material");
        TEST_END(); return;
    }

    /* Standard locker columns */
    const char *std[] = { "locker_id", "chest_id", "vnum", "container_id" };
    int nstd = sizeof(std) / sizeof(std[0]);
    for (int i = 0; i < nstd; i++) {
        if (!strstr(hit, std[i])) {
            TEST_FAIL("locker_items INSERT missing column: %s", std[i]);
            TEST_END(); return;
        }
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 2: Locker save uses DELETE-before-INSERT pattern.
 * ================================================================== */
static void test_locker_delete_before_insert(void)
{
    TEST_BEGIN("locker_delete_before_insert");

    if (!load_file("src/sql_player.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Find sql_save_locker real implementation after #else */
    const char *real = strstr(g_src_buf, "#else");
    if (!real) { TEST_FAIL("#else not found"); TEST_END(); return; }

    const char *func = strstr(real, "sql_save_locker");
    if (!func) {
        TEST_FAIL("sql_save_locker not found after #else");
        TEST_END(); return;
    }

    /* Must have DELETE FROM locker_items */
    if (!strstr(func, "DELETE FROM locker_items")) {
        TEST_FAIL("sql_save_locker: missing DELETE FROM locker_items cleanup");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 3: Locker chest save/load exists (not stubs).
 * ================================================================== */
static void test_locker_chest_save_load(void)
{
    TEST_BEGIN("locker_chest_save_load");

    if (!load_file("src/sql_player.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    const char *real = strstr(g_src_buf, "#else");
    if (!real) { TEST_FAIL("#else not found"); TEST_END(); return; }

    /* Check for chest save function */
    const char *chest_save = strstr(real, "sql_save_private_chest");
    if (!chest_save) {
        TEST_FAIL("sql_save_private_chest not found after #else");
        TEST_END(); return;
    }
    if (!strstr(chest_save, "locker_items")) {
        TEST_FAIL("sql_save_private_chest: missing locker_items reference");
        TEST_END(); return;
    }

    /* Check for chest load function */
    const char *chest_load = strstr(real, "sql_load_private_chest");
    if (!chest_load) {
        TEST_FAIL("sql_load_private_chest not found after #else");
        TEST_END(); return;
    }
    if (!strstr(chest_load, "locker_items")) {
        TEST_FAIL("sql_load_private_chest: missing locker_items reference");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 4: Locker enter/exit path in storage_lockers.c.
 * ================================================================== */
static void test_locker_enter_exit_path(void)
{
    TEST_BEGIN("locker_enter_exit_path");

    if (!load_file("src/storage_lockers.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Must reference locker functions */
    if (!strstr(g_src_buf, "locker")) {
        TEST_FAIL("storage_lockers.c: no locker references found");
        TEST_END(); return;
    }

    /* Must have sql_save_locker or sql_load_locker references */
    if (!strstr(g_src_buf, "sql_save_locker") &&
        !strstr(g_src_buf, "sql_load_locker")) {
        TEST_FAIL("storage_lockers.c: missing sql_save/load_locker reference");
        TEST_END(); return;
    }

    /* Must have room transitions (char_to_room or extract_char) */
    if (!strstr(g_src_buf, "char_to_room") &&
        !strstr(g_src_buf, "extract_char") &&
        !strstr(g_src_buf, "IN_ROOM")) {
        TEST_FAIL("storage_lockers.c: missing room transition (char_to_room/extract_char)");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 5: Locker private chest password verification.
 * ================================================================== */
static void test_locker_chest_password(void)
{
    TEST_BEGIN("locker_chest_password");

    if (!load_file("src/sql_player.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    const char *real = strstr(g_src_buf, "#else");
    if (!real) { TEST_FAIL("#else not found"); TEST_END(); return; }

    /* Look for password verification in chest functions */
    const char *verify = strstr(real, "sql_verify_chest_password");
    if (!verify) verify = strstr(real, "chest_password");
    if (!verify) verify = strstr(real, "SHA2");
    if (!verify) {
        /* Check storage_lockers.c for password handling */
        if (load_file("src/storage_lockers.c")) {
            if (strstr(g_src_buf, "password") || strstr(g_src_buf, "SHA2")) {
                TEST_PASS();
                TEST_END();
                return;
            }
        }
        TEST_FAIL("chest password verification not found (SHA2/verify_chest_password)");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 6: Locker load two-pass item placement.
 * ================================================================== */
static void test_locker_load_two_pass(void)
{
    TEST_BEGIN("locker_load_two_pass");

    if (!load_file("src/sql_player.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    const char *real = strstr(g_src_buf, "#else");
    if (!real) { TEST_FAIL("#else not found"); TEST_END(); return; }

    /* Find locker item load function */
    const char *load = strstr(real, "sql_load_locker_items");
    if (!load) load = strstr(real, "sql_load_locker");
    if (!load) {
        TEST_FAIL("sql_load_locker not found after #else");
        TEST_END(); return;
    }

    /* Should have two-pass pattern */
    if (!strstr(load, "two-pass") && !strstr(load, "two pass") &&
        !strstr(load, "first create all") && !strstr(load, "then place")) {
        /* May use a different pattern — check for temp storage */
        if (!strstr(load, "P_obj") || !strstr(load, "container")) {
            TEST_FAIL("locker load: two-pass item placement pattern not found");
            TEST_END(); return;
        }
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 7: sql_locker_exists checks DB before create.
 * ================================================================== */
static void test_locker_exists_before_create(void)
{
    TEST_BEGIN("locker_exists_before_create");

    if (!load_file("src/sql_player.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    const char *real = strstr(g_src_buf, "#else");
    if (!real) { TEST_FAIL("#else not found"); TEST_END(); return; }

    const char *exists = strstr(real, "sql_locker_exists");
    if (!exists) {
        TEST_FAIL("sql_locker_exists not found after #else");
        TEST_END(); return;
    }

    /* Must run a SELECT query */
    if (!strstr(exists, "SELECT")) {
        TEST_FAIL("sql_locker_exists: missing SELECT query");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 8: Locker transaction wrapping.
 * ================================================================== */
static void test_locker_txn_wrapping(void)
{
    TEST_BEGIN("locker_txn_wrapping");

    if (!load_file("src/sql_player.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    const char *real = strstr(g_src_buf, "#else");
    if (!real) { TEST_FAIL("#else not found"); TEST_END(); return; }

    const char *func = strstr(real, "sql_save_locker");
    if (!func) {
        TEST_FAIL("sql_save_locker not found after #else");
        TEST_END(); return;
    }

    /* Must use sql_begin_transaction or own_txn */
    if (!strstr(func, "sql_begin_transaction") &&
        !strstr(func, "own_txn")) {
        TEST_FAIL("sql_save_locker: missing sql_begin_transaction/own_txn");
        TEST_END(); return;
    }

    /* Must use sql_rollback on error paths */
    if (!strstr(func, "sql_rollback")) {
        TEST_FAIL("sql_save_locker: missing sql_rollback on error paths");
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
    { "locker_items_columns",           test_locker_items_columns },
    { "locker_delete_before_insert",    test_locker_delete_before_insert },
    { "locker_chest_save_load",         test_locker_chest_save_load },
    { "locker_enter_exit_path",         test_locker_enter_exit_path },
    { "locker_chest_password",          test_locker_chest_password },
    { "locker_load_two_pass",           test_locker_load_two_pass },
    { "locker_exists_before_create",    test_locker_exists_before_create },
    { "locker_txn_wrapping",            test_locker_txn_wrapping },
    { NULL, NULL },
};

static const int g_num_tests = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

int test_locker_stress_run_all(void)
{
    test_locker_stress_reset();
    printf("  [Locker Stress Tests]\n");
    for (int i = 0; g_tests[i].name; i++) g_tests[i].func();
    printf("  Passed: %d/%d\n", g_pass, g_pass + g_fail);
    return g_fail;
}

int test_locker_stress_run_one(const char *name)
{
    for (int i = 0; g_tests[i].name; i++)
        if (strcmp(g_tests[i].name, name) == 0) {
            printf("  [Locker Stress: %s]\n", name);
            g_tests[i].func();
            return (g_fail > 0);
        }
    return -1;
}

void test_locker_stress_print_summary(void)
{
    printf("  Locker Stress: %d passed, %d failed out of %d tests\n",
           g_pass, g_fail, g_num_tests);
}

void test_locker_stress_reset(void)
{
    g_pass = g_fail = 0;
    g_last_error[0] = '\0';
}
