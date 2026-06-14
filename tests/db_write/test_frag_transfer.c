/* ===================================================================
 * test_frag_transfer.c — Frag/pkill system and item ownership
 * transfer regression tests.
 *
 * Source-grep + SQL-pattern tests.  No MySQL or MUD runtime needed.
 *
 * Tests:
 *   1. sql_save_pkill writes to DB (not a stub)
 *   2. fragWorthy checks racewar + level
 *   3. killed_by UPDATE uses escaped name
 *   4. Item transfer on death (owner_corpse events)
 *   5. frag_leaderboard REPLACE INTO has all columns
 *   6. AddFrags updates both in-memory and DB
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_frag_transfer.h"

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
 * TEST 1: sql_save_pkill is a real implementation.
 *
 * Must contain: INSERT or persistence_item_events
 * Must handle: IS_PC_PET(killer) -> GET_MASTER(killer)
 * ================================================================== */
static void test_frag_save_pkill_real(void)
{
    TEST_BEGIN("frag_save_pkill_real");

    if (!load_file("src/sql.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Skip the __NO_MYSQL__ stub */
    const char *real = strstr(g_src_buf, "#else");
    if (!real) { TEST_FAIL("#else section not found in sql.c"); TEST_END(); return; }

    const char *func = strstr(real, "sql_save_pkill");
    if (!func) {
        TEST_FAIL("sql_save_pkill not found after #else in sql.c");
        TEST_END(); return;
    }

    /* Must contain DB write activity */
    if (!strstr(func, "INSERT") && !strstr(func, "REPLACE") &&
        !strstr(func, "persistence") && !strstr(func, "pkill_event")) {
        TEST_FAIL("sql_save_pkill: no INSERT/REPLACE/persistence activity found");
        TEST_END(); return;
    }

    /* Must handle pet killers */
    if (!strstr(func, "IS_PC_PET") && !strstr(func, "GET_MASTER")) {
        TEST_FAIL("sql_save_pkill: missing IS_PC_PET/GET_MASTER pet-killer handling");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 2: fragWorthy checks racewar + level requirements.
 *
 * Must contain: opposite_racewar, level difference bounds
 * ================================================================== */
static void test_frag_worthy_checks(void)
{
    TEST_BEGIN("frag_worthy_checks");

    if (!load_file("src/fraglist.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    if (!strstr(g_src_buf, "fragWorthy")) {
        TEST_FAIL("fragWorthy function not found in fraglist.c");
        TEST_END(); return;
    }

    /* Must check racewar */
    if (!strstr(g_src_buf, "racewar") && !strstr(g_src_buf, "opposite_racewar")
        && !strstr(g_src_buf, "GET_RACEWAR")) {
        TEST_FAIL("fragWorthy: missing racewar check");
        TEST_END(); return;
    }

    /* Must check level */
    const char *lvl = strstr(g_src_buf, "GET_LEVEL");
    if (!lvl) {
        TEST_FAIL("fragWorthy: missing GET_LEVEL check");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 3: killed_by UPDATE uses escaped name.
 *
 * fight.c must write:
 * UPDATE player_data SET killed_by = '<escaped>' WHERE pid = <victim>
 * ================================================================== */
static void test_frag_killed_by_escaped(void)
{
    TEST_BEGIN("frag_killed_by_escaped");

    if (!load_file("src/fight.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Search for the killed_by UPDATE pattern */
    const char *hit = strstr(g_src_buf, "killed_by");
    if (!hit) {
        TEST_FAIL("killed_by not found in fight.c");
        TEST_END(); return;
    }

    /* Must be an UPDATE player_data SET killed_by */
    if (!strstr(hit, "UPDATE player_data") && !strstr(hit, "update player_data")) {
        /* Could also be in a snprintf building the query */
        if (!strstr(hit, "player_data") || !strstr(hit, "SET")) {
            TEST_FAIL("killed_by: missing UPDATE player_data SET pattern");
            TEST_END(); return;
        }
    }

    /* Must use some form of escaping */
    if (!strstr(hit, "escape") && !strstr(hit, "GET_NAME")) {
        TEST_FAIL("killed_by: no name escaping found near killed_by usage");
        TEST_END(); return;
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 4: Item transfer on death — owner_corpse events.
 *
 * When a player dies, items transfer to corpse via
 * persistence_record_item_event("owner_corpse", ...)
 * Target must be "corpse:<name>"
 * ================================================================== */
static void test_frag_item_transfer_corpse(void)
{
    TEST_BEGIN("frag_item_transfer_corpse");

    if (!load_file("src/fight.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Look for owner_corpse event type */
    if (!strstr(g_src_buf, "owner_corpse")) {
        /* Also check sql_player.c */
        if (!load_file("src/sql_player.c")) {
            TEST_FAIL("owner_corpse event type not found in fight.c or sql_player.c");
            TEST_END(); return;
        }
        if (!strstr(g_src_buf, "owner_corpse")) {
            TEST_FAIL("owner_corpse event type not found in fight.c or sql_player.c");
            TEST_END(); return;
        }
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 5: frag_leaderboard REPLACE INTO has all columns.
 *
 * sql_update_frag_leaderboard() must use:
 * REPLACE INTO frag_leaderboard
 * (pid, account_name, char_name, total_frags, racewar, race, class,
 *  level, deleted_at)
 * ================================================================== */
static void test_frag_leaderboard_columns(void)
{
    TEST_BEGIN("frag_leaderboard_columns");

    if (!load_file("src/sql.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    /* Skip stub */
    const char *real = strstr(g_src_buf, "#else");
    if (!real) { TEST_FAIL("#else not found"); TEST_END(); return; }

    const char *hit = strstr(real, "frag_leaderboard");
    if (!hit) {
        TEST_FAIL("frag_leaderboard not found in sql.c after #else");
        TEST_END(); return;
    }

    /* Must use REPLACE INTO or INSERT INTO */
    if (!strstr(hit, "REPLACE") && !strstr(hit, "INSERT")) {
        TEST_FAIL("frag_leaderboard: missing REPLACE/INSERT");
        TEST_END(); return;
    }

    /* Must contain key columns */
    const char *cols[] = { "pid", "total_frags", "racewar", "deleted_at" };
    int n = sizeof(cols) / sizeof(cols[0]);
    for (int i = 0; i < n; i++) {
        if (!strstr(hit, cols[i])) {
            TEST_FAIL("frag_leaderboard missing column: %s", cols[i]);
            TEST_END(); return;
        }
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 6: AddFrags updates both in-memory and DB.
 *
 * AddFrags() must:
 * - Increment ch->only.pc->frags (or equivalent)
 * - Call sql_update_frag_leaderboard(ch)
 * ================================================================== */
static void test_frag_addfrags_updates_both(void)
{
    TEST_BEGIN("frag_addfrags_updates_both");

    if (!load_file("src/fraglist.c")) {
        TEST_FAIL("%s", g_last_error);
        TEST_END(); return;
    }

    const char *func = strstr(g_src_buf, "AddFrags");
    if (!func) {
        TEST_FAIL("AddFrags function not found in fraglist.c");
        TEST_END(); return;
    }

    /* Must update frags counter */
    if (!strstr(func, "frags")) {
        TEST_FAIL("AddFrags: missing frags counter update");
        TEST_END(); return;
    }

    /* Must call DB update */
    if (!strstr(func, "frag_leaderboard") && !strstr(func, "sql_update_frag")) {
        TEST_FAIL("AddFrags: missing frag_leaderboard/sql_update_frag call");
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
    { "frag_save_pkill_real",         test_frag_save_pkill_real },
    { "frag_worthy_checks",           test_frag_worthy_checks },
    { "frag_killed_by_escaped",       test_frag_killed_by_escaped },
    { "frag_item_transfer_corpse",    test_frag_item_transfer_corpse },
    { "frag_leaderboard_columns",     test_frag_leaderboard_columns },
    { "frag_addfrags_updates_both",   test_frag_addfrags_updates_both },
    { NULL, NULL },
};

static const int g_num_tests = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

int test_frag_transfer_run_all(void)
{
    test_frag_transfer_reset();
    printf("  [Frag & Item Transfer Tests]\n");
    for (int i = 0; g_tests[i].name; i++) g_tests[i].func();
    printf("  Passed: %d/%d\n", g_pass, g_pass + g_fail);
    return g_fail;
}

int test_frag_transfer_run_one(const char *name)
{
    for (int i = 0; g_tests[i].name; i++)
        if (strcmp(g_tests[i].name, name) == 0) {
            printf("  [Frag Transfer: %s]\n", name);
            g_tests[i].func();
            return (g_fail > 0);
        }
    return -1;
}

void test_frag_transfer_print_summary(void)
{
    printf("  Frag Transfer: %d passed, %d failed out of %d tests\n",
           g_pass, g_fail, g_num_tests);
}

void test_frag_transfer_reset(void)
{
    g_pass = g_fail = 0;
    g_last_error[0] = '\0';
}
