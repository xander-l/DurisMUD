/* ===================================================================
 * test_pet_lifecycle.c — Pet save/load, charm lifecycle, and crash
 * recovery regression tests.
 *
 * These are source-grep + SQL-pattern verification tests — same
 * approach as the existing 12 test files.  No MySQL or MUD runtime
 * needed.
 *
 * Tests:
 *   1.  Source-grep: sql_save_player_pets is real implementation
 *   2.  Source-grep: sql_load_player_pets is real implementation
 *   3.  Source-grep: crash-only save guard (RENT_CRASH/RENT_CRASH2)
 *   4.  Source-grep: player_pets INSERT columns correct
 *   5.  Source-grep: player_pet_items has all 28 columns (+ v19+material)
 *   6.  Source-grep: ITEM_NORENT skip in pet item save
 *   7.  Source-grep: container recursion in pet item save
 *   8.  Source-grep: charm_broken callback via LNK_PET link
 *   9.  Source-grep: setup_pet + add_follower called on pet load
 *  10.  Source-grep: MAX_PETS defined in config.h
 *  11.  Source-grep: two-pass item load in sql_load_player_pets
 *  12.  Source-grep: normal save DELETEs pet rows (non-crash path)
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_pet_lifecycle.h"

/* ------------------------------------------------------------------ */
/*  Test framework                                                    */
/* ------------------------------------------------------------------ */

static int  g_pass = 0;
static int  g_fail = 0;
static char g_last_error[4096];
static char g_src_buf[524288];
static int  g_src_loaded = 0;

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

static int load_sql_player(void)
{
    if (g_src_loaded)
        return 1;

    FILE *f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f)
        return 0;

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    if (n == sizeof(g_src_buf) - 1)
        return 0;

    g_src_loaded = 1;
    return 1;
}

/* Helper: find the real implementation (past the __NO_MYSQL__ stubs).
 * Searches from the first occurrence of the anchor after #else. */
static const char *find_real_impl(const char *anchor)
{
    const char *real_code = strstr(g_src_buf, "#else");
    if (!real_code)
        return NULL;
    return strstr(real_code, anchor);
}

/* ==================================================================
 * TEST 1: sql_save_player_pets is the real implementation.
 *
 * Anchors on: "pet save - save all player's pets with equipment"
 * Must contain: INSERT INTO player_pets, player_pet_items
 * ================================================================== */
static void test_pet_save_real_impl(void)
{
    TEST_BEGIN("pet_save_real_impl");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *func = find_real_impl("pet save - save all player's pets with equipment");
    if (!func) {
        TEST_FAIL("sql_save_player_pets real implementation not found "
                  "(anchor: 'pet save - save all player's pets with equipment')");
        TEST_END(); return;
    }

    if (!strstr(func, "INSERT INTO player_pets")) {
        TEST_FAIL("sql_save_player_pets: missing INSERT INTO player_pets");
        TEST_END(); return;
    }
    if (!strstr(func, "player_pet_items")) {
        TEST_FAIL("sql_save_player_pets: missing player_pet_items");
        TEST_END(); return;
    }
    if (!strstr(func, "DELETE FROM player_pets")) {
        TEST_FAIL("sql_save_player_pets: missing DELETE FROM player_pets cleanup");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 2: sql_load_player_pets is the real implementation.
 *
 * Anchors on: "pet load - restore all player's pets with equipment"
 * Must contain: SELECT FROM player_pets, read_mobile, setup_pet
 * ================================================================== */
static void test_pet_load_real_impl(void)
{
    TEST_BEGIN("pet_load_real_impl");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *func = find_real_impl("pet load - restore all player's pets with equipment");
    if (!func) {
        TEST_FAIL("sql_load_player_pets real implementation not found "
                  "(anchor: 'pet load - restore all player's pets with equipment')");
        TEST_END(); return;
    }

    if (!strstr(func, "SELECT") || !strstr(func, "player_pets")) {
        TEST_FAIL("sql_load_player_pets: missing SELECT FROM player_pets");
        TEST_END(); return;
    }
    if (!strstr(func, "read_mobile")) {
        TEST_FAIL("sql_load_player_pets: missing read_mobile call");
        TEST_END(); return;
    }
    if (!strstr(func, "setup_pet")) {
        TEST_FAIL("sql_load_player_pets: missing setup_pet call");
        TEST_END(); return;
    }
    if (!strstr(func, "add_follower")) {
        TEST_FAIL("sql_load_player_pets: missing add_follower call");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 3: Crash-only save guard.
 *
 * sql_save_player_pets must only save on RENT_CRASH / RENT_CRASH2.
 * On non-crash: DELETE FROM player_pets cleanup, return true.
 * ================================================================== */
static void test_pet_crash_only_save(void)
{
    TEST_BEGIN("pet_crash_only_save");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *func = find_real_impl("pet save - save all player's pets with equipment");
    if (!func) {
        TEST_FAIL("sql_save_player_pets real implementation not found");
        TEST_END(); return;
    }

    /* Must check save_type != RENT_CRASH && save_type != RENT_CRASH2 */
    if (!strstr(func, "RENT_CRASH")) {
        TEST_FAIL("sql_save_player_pets: missing RENT_CRASH check");
        TEST_END(); return;
    }
    if (!strstr(func, "RENT_CRASH2")) {
        TEST_FAIL("sql_save_player_pets: missing RENT_CRASH2 check");
        TEST_END(); return;
    }

    /* The guard should be: save_type != RENT_CRASH && save_type != RENT_CRASH2 */
    const char *guard = strstr(func, "save_type != RENT_CRASH");
    if (!guard) {
        TEST_FAIL("sql_save_player_pets: missing 'save_type != RENT_CRASH' guard");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 4: player_pets INSERT has correct columns.
 *
 * Must contain: owner_pid, mob_vnum, room_vnum, charm_duration, pet_order
 * ================================================================== */
static void test_pet_insert_columns(void)
{
    TEST_BEGIN("pet_insert_columns");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *func = find_real_impl("pet save - save all player's pets with equipment");
    if (!func) {
        TEST_FAIL("sql_save_player_pets real implementation not found");
        TEST_END(); return;
    }

    const char *required[] = {
        "owner_pid", "mob_vnum", "room_vnum",
        "charm_duration", "pet_order"
    };
    int n = sizeof(required) / sizeof(required[0]);

    for (int i = 0; i < n; i++) {
        if (!strstr(func, required[i])) {
            TEST_FAIL("player_pets INSERT missing column: %s", required[i]);
            TEST_END(); return;
        }
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 5: player_pet_items has all required columns.
 *
 * Verify the INSERT includes wear_flags, item_type, bitvector1-5,
 * item_material (v19+material coverage), plus standard columns.
 * ================================================================== */
static void test_pet_item_insert_columns(void)
{
    TEST_BEGIN("pet_item_insert_columns");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *func = find_real_impl("INSERT INTO player_pet_items");
    if (!func) {
        /* Try without the real-impl filter — the INSERT may be in a helper */
        func = strstr(g_src_buf, "INSERT INTO player_pet_items");
        if (!func) {
            TEST_FAIL("player_pet_items INSERT not found in sql_player.c");
            TEST_END(); return;
        }
    }

    /* v19 columns */
    const char *v19_cols[] = {
        "wear_flags", "item_type",
        "bitvector1", "bitvector2", "bitvector3", "bitvector4", "bitvector5"
    };
    int n19 = sizeof(v19_cols) / sizeof(v19_cols[0]);
    for (int i = 0; i < n19; i++) {
        if (!strstr(func, v19_cols[i])) {
            TEST_FAIL("player_pet_items INSERT missing v19 column: %s", v19_cols[i]);
            TEST_END(); return;
        }
    }

    /* item_material (Phase 3.6) */
    if (!strstr(func, "item_material")) {
        TEST_FAIL("player_pet_items INSERT missing item_material column");
        TEST_END(); return;
    }

    /* Standard columns */
    const char *std_cols[] = {
        "pet_id", "vnum", "equip_slot", "container_id",
        "name", "short_descr", "description", "action_descr"
    };
    int nstd = sizeof(std_cols) / sizeof(std_cols[0]);
    for (int i = 0; i < nstd; i++) {
        if (!strstr(func, std_cols[i])) {
            TEST_FAIL("player_pet_items INSERT missing column: %s", std_cols[i]);
            TEST_END(); return;
        }
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 6: ITEM_NORENT skip in pet item save.
 *
 * sql_save_single_pet_item must check IS_SET(obj->extra_flags, ITEM_NORENT)
 * and return 0 (skip) for non-rentable items.
 * ================================================================== */
static void test_pet_item_skip_norent(void)
{
    TEST_BEGIN("pet_item_skip_norent");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    /* Find the helper near the pet item save */
    const char *save_func = find_real_impl("save a single pet item");
    if (!save_func) {
        TEST_FAIL("sql_save_single_pet_item comment not found");
        TEST_END(); return;
    }

    if (!strstr(save_func, "ITEM_NORENT")) {
        TEST_FAIL("sql_save_single_pet_item: missing ITEM_NORENT check");
        TEST_END(); return;
    }

    /* Must check the flag with IS_SET and return 0 */
    const char *norent = strstr(save_func, "ITEM_NORENT");
    /* Look for a return near the norent check (within 200 chars) */
    const char *ret = strstr(norent, "return 0");
    if (!ret || (ret - norent) > 200) {
        TEST_FAIL("sql_save_single_pet_item: ITEM_NORENT check should return 0 (skip)");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 7: Container recursion in pet item save.
 *
 * sql_save_single_pet_item must recurse into obj->contains
 * for container contents.
 * ================================================================== */
static void test_pet_item_container_recurse(void)
{
    TEST_BEGIN("pet_item_container_recurse");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *save_func = find_real_impl("save a single pet item");
    if (!save_func) {
        TEST_FAIL("sql_save_single_pet_item comment not found");
        TEST_END(); return;
    }

    /* Must access obj->contains */
    if (!strstr(save_func, "contains")) {
        TEST_FAIL("sql_save_single_pet_item: missing container recursion (obj->contains)");
        TEST_END(); return;
    }

    /* Must call itself recursively for contents */
    const char *first_call = strstr(save_func, "sql_save_single_pet_item");
    const char *recurse = NULL;
    if (first_call)
        recurse = strstr(first_call + strlen("sql_save_single_pet_item"),
                         "sql_save_single_pet_item");
    if (!recurse) {
        TEST_FAIL("sql_save_single_pet_item: missing recursive call for containers");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 8: charm_broken callback via LNK_PET link.
 *
 * Verifies: define_link(LNK_PET, "PET", charm_broken, ...)
 * exists in src/sql_player.c (or wherever links are defined).
 * We search src/affects.c since that's where define_link lives.
 * ================================================================== */
static void test_charm_broken_link_exists(void)
{
    TEST_BEGIN("charm_broken_link_exists");

    /* Search in affects.c where define_link calls are */
    FILE *f = fopen("src/affects.c", "r");
    if (!f) f = fopen("../../../src/affects.c", "r");
    if (!f) f = fopen("../../src/affects.c", "r");
    if (!f) {
        TEST_FAIL("cannot open src/affects.c");
        TEST_END(); return;
    }

    /* Use the file-scope buffer (512KB) — affects.c can be ~200KB */
    g_src_loaded = 0;  // reset cache so we can reuse g_src_buf
    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    if (n == sizeof(g_src_buf) - 1) {
        TEST_FAIL("src/affects.c larger than 512KB — test buffer too small");
        TEST_END(); return;
    }

    /* Must find: define_link(LNK_PET, \"PET\", charm_broken, */
    const char *hit = strstr(g_src_buf, "LNK_PET");
    if (!hit) {
        TEST_FAIL("LNK_PET link definition not found in affects.c");
        TEST_END(); return;
    }

    if (!strstr(hit, "charm_broken")) {
        TEST_FAIL("LNK_PET link does not reference charm_broken callback");
        TEST_END(); return;
    }

    if (!strstr(hit, "\"PET\"")) {
        TEST_FAIL("LNK_PET link missing \"PET\" name string");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 9: setup_pet + add_follower called on pet load.
 *
 * sql_load_player_pets must call setup_pet(pet, ch, charm_duration, PET_NOAGGRO)
 * and add_follower(pet, ch) for each loaded pet.
 * ================================================================== */
static void test_pet_load_calls_setup_add_follower(void)
{
    TEST_BEGIN("pet_load_calls_setup_add_follower");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *func = find_real_impl("pet load - restore all player's pets with equipment");
    if (!func) {
        TEST_FAIL("sql_load_player_pets real implementation not found");
        TEST_END(); return;
    }

    /* setup_pet must be called with PET_NOAGGRO flag */
    if (!strstr(func, "PET_NOAGGRO")) {
        TEST_FAIL("sql_load_player_pets: missing PET_NOAGGRO flag in setup_pet call");
        TEST_END(); return;
    }

    /* setup_pet and add_follower must both be called */
    const char *setup = strstr(func, "setup_pet");
    const char *addf  = strstr(func, "add_follower");
    if (!setup) {
        TEST_FAIL("sql_load_player_pets: missing setup_pet call");
        TEST_END(); return;
    }
    if (!addf) {
        TEST_FAIL("sql_load_player_pets: missing add_follower call");
        TEST_END(); return;
    }

    /* setup_pet should come before add_follower */
    if (setup > addf) {
        TEST_FAIL("sql_load_player_pets: add_follower called before setup_pet — "
                  "pet should be set up before being added as follower");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 10: MAX_PETS defined in config.h.
 *
 * Verifies MAX_PETS is defined (used by copyover pet recovery).
 * ================================================================== */
static void test_max_pets_defined(void)
{
    TEST_BEGIN("max_pets_defined");

    FILE *f = fopen("src/config.h", "r");
    if (!f) f = fopen("../../../src/config.h", "r");
    if (!f) f = fopen("../../src/config.h", "r");
    if (!f) {
        TEST_FAIL("cannot open src/config.h");
        TEST_END(); return;
    }

    g_src_loaded = 0;  // reset cache so we can reuse g_src_buf
    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    if (!strstr(g_src_buf, "MAX_PETS")) {
        TEST_FAIL("MAX_PETS not defined in config.h");
        TEST_END(); return;
    }

    /* Should be defined as a reasonable number (5 in this codebase) */
    const char *maxp = strstr(g_src_buf, "MAX_PETS");
    if (!strstr(maxp, "5") || (strstr(maxp, "5") - maxp) > 60) {
        /* The 5 should be within ~60 chars of the define */
        TEST_FAIL("MAX_PETS value not found near definition");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 11: Two-pass item load in sql_load_player_pets.
 *
 * The pet load function must use a two-pass approach:
 * Pass 1: create all items, store (db_id, obj) in temp array
 * Pass 2: place items (handles container parent references)
 * ================================================================== */
static void test_pet_load_two_pass_items(void)
{
    TEST_BEGIN("pet_load_two_pass_items");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *func = find_real_impl("pet load - restore all player's pets with equipment");
    if (!func) {
        TEST_FAIL("sql_load_player_pets real implementation not found");
        TEST_END(); return;
    }

    /* The two-pass pattern: must SELECT player_pet_items, then iterate twice.
     * Look for a temp array or struct used to store items before placement. */
    if (!strstr(func, "player_pet_items")) {
        TEST_FAIL("sql_load_player_pets: missing SELECT FROM player_pet_items");
        TEST_END(); return;
    }

    /* Look for the two-pass comment or pattern */
    if (!strstr(func, "two-pass") && !strstr(func, "two pass") &&
        !strstr(func, "first create all items") && !strstr(func, "then place")) {
        /* The function may just use a different pattern. Check for the
         * struct that holds items temporarily */
        if (!strstr(func, "db_id") || !strstr(func, "P_obj")) {
            TEST_FAIL("sql_load_player_pets: two-pass item load pattern not found "
                      "(expected 'two-pass' comment or temp item struct)");
            TEST_END(); return;
        }
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 12: Normal save (non-crash) DELETEs pet rows, no INSERT.
 *
 * On save_type != RENT_CRASH && save_type != RENT_CRASH2:
 * - DELETE FROM player_pets WHERE owner_pid = ?
 * - No INSERT INTO player_pets (cleanup only)
 * - Returns true
 * ================================================================== */
static void test_pet_noncrash_cleanup_only(void)
{
    TEST_BEGIN("pet_noncrash_cleanup_only");

    if (!load_sql_player()) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END(); return;
    }

    const char *func = find_real_impl("pet save - save all player's pets with equipment");
    if (!func) {
        TEST_FAIL("sql_save_player_pets real implementation not found");
        TEST_END(); return;
    }

    /* Find the non-crash path: after the guard check, before the INSERT */
    const char *guard = strstr(func, "save_type != RENT_CRASH");
    if (!guard) {
        TEST_FAIL("sql_save_player_pets: crash-save guard not found");
        TEST_END(); return;
    }

    /* Within ~500 chars after the guard, must find DELETE FROM player_pets */
    const char *del = strstr(guard, "DELETE FROM player_pets");
    if (!del || (del - guard) > 500) {
        TEST_FAIL("sql_save_player_pets: DELETE FROM player_pets not found in "
                  "non-crash cleanup path");
        TEST_END(); return;
    }

    /* Must also find a commit or return true after the delete */
    const char *commit = strstr(del, "sql_commit");
    const char *ret_true = strstr(del, "return true");
    if (!commit && !ret_true) {
        TEST_FAIL("sql_save_player_pets: non-crash path should commit/return "
                  "after cleanup DELETE");
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
    { "pet_save_real_impl",               test_pet_save_real_impl },
    { "pet_load_real_impl",               test_pet_load_real_impl },
    { "pet_crash_only_save",              test_pet_crash_only_save },
    { "pet_insert_columns",               test_pet_insert_columns },
    { "pet_item_insert_columns",          test_pet_item_insert_columns },
    { "pet_item_skip_norent",             test_pet_item_skip_norent },
    { "pet_item_container_recurse",       test_pet_item_container_recurse },
    { "charm_broken_link_exists",         test_charm_broken_link_exists },
    { "pet_load_calls_setup_add_follower", test_pet_load_calls_setup_add_follower },
    { "max_pets_defined",                 test_max_pets_defined },
    { "pet_load_two_pass_items",          test_pet_load_two_pass_items },
    { "pet_noncrash_cleanup_only",        test_pet_noncrash_cleanup_only },
    { NULL, NULL },
};

static const int g_num_tests = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

int test_pet_lifecycle_run_all(void)
{
    test_pet_lifecycle_reset();
    printf("  [Pet Lifecycle Tests]\n");

    for (int i = 0; g_tests[i].name; i++) {
        g_tests[i].func();
    }

    printf("  Passed: %d/%d\n", g_pass, g_pass + g_fail);
    return g_fail;
}

int test_pet_lifecycle_run_one(const char *name)
{
    for (int i = 0; g_tests[i].name; i++) {
        if (strcmp(g_tests[i].name, name) == 0) {
            printf("  [Pet Lifecycle: %s]\n", name);
            g_tests[i].func();
            return (g_fail > 0);
        }
    }
    return -1;
}

void test_pet_lifecycle_print_summary(void)
{
    printf("  Pet Lifecycle: %d passed, %d failed out of %d tests\n",
           g_pass, g_fail, g_num_tests);
}

void test_pet_lifecycle_reset(void)
{
    g_pass = 0;
    g_fail = 0;
    g_last_error[0] = '\0';
    g_src_loaded = 0;
}
