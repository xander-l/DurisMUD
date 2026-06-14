/*
 * test_multi_table_consistency.c
 *
 * Source-grep regression guards verifying cross-table consistency
 * across all 7 item tables in sql_player.c.
 *
 * Tests verify:
 *   - All 7 table names are referenced
 *   - DELETE-before-INSERT cleanup exists for each table
 *   - obj_uid is preserved in INSERT statements
 *   - sql_persistence_item_owner_matches is called on item load
 *   - Transaction wrapping exists for multi-table save operations
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_multi_table_consistency.h"

/* ------------------------------------------------------------------ */
/*  Test framework                                                    */
/* ------------------------------------------------------------------ */

static int  g_pass = 0;
static int  g_fail = 0;
static char g_last_error[4096];
static char g_src_buf[524288];  /* 512KB — sql_player.c is ~200KB */
static int  g_src_loaded = 0;   /* cached: read once, reuse */

#define TEST_BEGIN(name) do { printf("  %s ... ", name); fflush(stdout); } while (0)
#define TEST_END()       do { printf("\n"); } while (0)
#define TEST_PASS()      do { g_pass++; } while (0)
#define TEST_FAIL(...)   do { \
    snprintf(g_last_error, sizeof(g_last_error), __VA_ARGS__); \
    g_fail++; \
    fprintf(stderr, "\n  FAIL: %s", g_last_error); \
} while (0)

/* ------------------------------------------------------------------ */
/*  Source reader                                                     */
/* ------------------------------------------------------------------ */

static int load_source(void)
{
    if (g_src_loaded)
        return 1;  /* already cached */

    FILE *f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f)
        return 0;  /* caller reports the error */

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    if (n == sizeof(g_src_buf) - 1)
        return 0;  /* caller reports the error */

    g_src_loaded = 1;
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Test 1: All 7 item table names are referenced.                    */
/* ------------------------------------------------------------------ */

static void test_all_seven_tables_referenced(void)
{
    TEST_BEGIN("all 7 item tables referenced");

    if (!load_source()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *tables[] = {
        "player_items", "player_pet_items", "locker_items",
        "shopkeeper_items", "corpse_items", "saved_items",
        "siege_items"
    };
    int missing = 0;

    for (int i = 0; i < 7; i++) {
        if (!strstr(g_src_buf, tables[i])) {
            TEST_FAIL("table '%s' not referenced in sql_player.c", tables[i]);
            missing = 1;
        }
    }

    if (!missing) TEST_PASS();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/*  Test 2-6: DELETE-before-INSERT for each item table.               */
/* ------------------------------------------------------------------ */

static void check_delete_exists(const char *table, const char *label)
{
    /* Search for DELETE FROM <table> */
    char del_pattern[128];
    snprintf(del_pattern, sizeof(del_pattern), "DELETE FROM %s", table);

    const char *hit = strstr(g_src_buf, del_pattern);
    if (!hit) {
        TEST_FAIL("%s: no DELETE FROM %s found — stale items may accumulate",
                  label, table);
        return;
    }
    TEST_PASS();
}

static void test_delete_player_items_exists(void)
{
    TEST_BEGIN("DELETE FROM player_items exists");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_delete_exists("player_items", "player_items");
    TEST_END();
}

static void test_delete_locker_items_exists(void)
{
    TEST_BEGIN("DELETE FROM locker_items exists");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_delete_exists("locker_items", "locker_items");
    TEST_END();
}

static void test_delete_corpse_items_exists(void)
{
    TEST_BEGIN("DELETE FROM corpse_items exists");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_delete_exists("corpse_items", "corpse_items");
    TEST_END();
}

static void test_delete_saved_items_exists(void)
{
    TEST_BEGIN("DELETE FROM saved_items exists");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_delete_exists("saved_items", "saved_items");
    TEST_END();
}

static void test_delete_siege_items_exists(void)
{
    TEST_BEGIN("DELETE FROM siege_items exists");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_delete_exists("siege_items", "siege_items");
    TEST_END();
}

static void test_delete_player_pet_items_exists(void)
{
    TEST_BEGIN("DELETE FROM player_pet_items exists");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_delete_exists("player_pet_items", "player_pet_items");
    TEST_END();
}

static void test_delete_shopkeeper_items_exists(void)
{
    TEST_BEGIN("DELETE FROM shopkeeper_items exists");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_delete_exists("shopkeeper_items", "shopkeeper_items");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/*  Test 7-8: obj_uid in INSERT for key tables.                       */
/* ------------------------------------------------------------------ */

static void check_obj_uid_in_insert(const char *table_hint, const char *label)
{
    /* Find an INSERT INTO <table_hint> and verify obj_uid after it. */
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "INSERT INTO %s", table_hint);
    const char *ins = strstr(g_src_buf, pattern);
    if (!ins) {
        TEST_FAIL("%s: no INSERT INTO %s found", label, table_hint);
        return;
    }

    /* obj_uid should appear within ~2000 chars after INSERT. */
    const char *uid = strstr(ins, "obj_uid");
    if (!uid || (uid - ins) > 2000) {
        TEST_FAIL("%s: obj_uid not found in INSERT INTO %s columns",
                  label, table_hint);
        return;
    }
    TEST_PASS();
}

static void test_obj_uid_in_player_items_insert(void)
{
    TEST_BEGIN("obj_uid in player_items INSERT");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_obj_uid_in_insert("player_items", "player_items");
    TEST_END();
}

static void test_obj_uid_in_locker_items_insert(void)
{
    TEST_BEGIN("obj_uid in locker_items INSERT");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_obj_uid_in_insert("locker_items", "locker_items");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/*  Test 9-11: sql_persistence_item_owner_matches called on load.     */
/* ------------------------------------------------------------------ */

static void check_owner_matches_called(const char *owner_type,
                                       const char *label)
{
    const char *pattern = "sql_persistence_item_owner_matches";

    /* Find each call site and check for the owner_type string within
     * 500 chars after it. */
    int found = 0;
    const char *p = g_src_buf;
    while ((p = strstr(p, pattern)) != NULL) {
        const char *type_hit = strstr(p, owner_type);
        if (type_hit && (type_hit - p) < 500) {
            found = 1;
            break;
        }
        p += strlen(pattern);  /* advance past this match */
    }

    if (!found) {
        TEST_FAIL("%s: sql_persistence_item_owner_matches not called "
                  "with owner_type=\"%s\"", label, owner_type);
        return;
    }
    TEST_PASS();
}

static void test_owner_matches_for_player_items(void)
{
    TEST_BEGIN("owner_matches called for player items");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_owner_matches_called("player", "player");
    TEST_END();
}

static void test_owner_matches_for_locker_items(void)
{
    TEST_BEGIN("owner_matches called for locker items");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_owner_matches_called("locker", "locker");
    TEST_END();
}

static void test_owner_matches_for_corpse_items(void)
{
    TEST_BEGIN("owner_matches called for corpse items");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_owner_matches_called("corpse", "corpse");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/*  Test 12-14: Transaction wrapping for multi-table saves.           */
/* ------------------------------------------------------------------ */

static void check_txn_wrapping(const char *func_hint, const char *label)
{
    const char *func = strstr(g_src_buf, func_hint);
    if (!func) {
        TEST_FAIL("%s: function pattern '%s' not found", label, func_hint);
        return;
    }

    /* The function should have either sql_begin_transaction or own_txn. */
    const char *txn = strstr(func, "sql_begin_transaction");
    const char *own = strstr(func, "own_txn");

    /* Search within ~3000 chars after function start. */
    if ((!txn || (txn - func) > 3000) &&
        (!own || (own - func) > 3000)) {
        TEST_FAIL("%s: no sql_begin_transaction or own_txn found "
                  "within function body", label);
        return;
    }
    TEST_PASS();
}

static void test_txn_wrapping_save_player_items(void)
{
    TEST_BEGIN("txn wrapping: sql_save_player_items");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_txn_wrapping("sql_save_player_items", "save_player_items");
    TEST_END();
}

static void test_txn_wrapping_save_locker(void)
{
    TEST_BEGIN("txn wrapping: sql_save_locker");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_txn_wrapping("sql_save_locker", "save_locker");
    TEST_END();
}

static void test_txn_wrapping_save_corpse(void)
{
    TEST_BEGIN("txn wrapping: sql_save_corpse");
    if (!load_source()) { TEST_FAIL("cannot open src/sql_player.c"); TEST_END(); return; }
    check_txn_wrapping("sql_save_corpse", "save_corpse");
    TEST_END();
}

/* ================================================================== */
/*  Test registry                                                      */
/* ================================================================== */

typedef struct { const char *name; void (*func)(void); } test_entry;

static test_entry g_tests[] = {
    { "all_seven_tables_referenced",        test_all_seven_tables_referenced },
    { "delete_player_items_exists",         test_delete_player_items_exists },
    { "delete_locker_items_exists",         test_delete_locker_items_exists },
    { "delete_corpse_items_exists",         test_delete_corpse_items_exists },
    { "delete_saved_items_exists",          test_delete_saved_items_exists },
    { "delete_siege_items_exists",          test_delete_siege_items_exists },
    { "delete_player_pet_items_exists",     test_delete_player_pet_items_exists },
    { "delete_shopkeeper_items_exists",     test_delete_shopkeeper_items_exists },
    { "obj_uid_in_player_items_insert",     test_obj_uid_in_player_items_insert },
    { "obj_uid_in_locker_items_insert",     test_obj_uid_in_locker_items_insert },
    { "owner_matches_for_player_items",     test_owner_matches_for_player_items },
    { "owner_matches_for_locker_items",     test_owner_matches_for_locker_items },
    { "owner_matches_for_corpse_items",     test_owner_matches_for_corpse_items },
    { "txn_wrapping_save_player_items",     test_txn_wrapping_save_player_items },
    { "txn_wrapping_save_locker",           test_txn_wrapping_save_locker },
    { "txn_wrapping_save_corpse",           test_txn_wrapping_save_corpse },
    { NULL, NULL },
};

static const int g_num_tests = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

int test_multi_table_consistency_run_all(void)
{
    test_multi_table_consistency_reset();
    printf("  [Multi-Table Consistency Tests]\n");

    for (int i = 0; g_tests[i].name; i++) {
        g_tests[i].func();
    }

    printf("  Passed: %d/%d\n", g_pass, g_pass + g_fail);
    return g_fail;
}

int test_multi_table_consistency_run_one(const char *name)
{
    for (int i = 0; g_tests[i].name; i++) {
        if (strcmp(g_tests[i].name, name) == 0) {
            printf("  [Multi-Table Consistency: %s]\n", name);
            g_tests[i].func();
            return (g_fail > 0);
        }
    }
    return -1;
}

void test_multi_table_consistency_print_summary(void)
{
    printf("  Multi-Table Consistency: %d passed, %d failed out of %d tests\n",
           g_pass, g_fail, g_num_tests);
}

void test_multi_table_consistency_reset(void)
{
    g_pass = 0;
    g_fail = 0;
    g_last_error[0] = '\0';
    g_src_loaded = 0;
}
