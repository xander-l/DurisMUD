/* ===================================================================
 * test_live_frags.c — Live DB roundtrip tests for frag_leaderboard
 * and account_characters tables using real MySQL.
 *
 * These tests run against a live MySQL instance and verify:
 *   1. account_characters INSERT + SELECT roundtrip
 *   2. frag_leaderboard REPLACE INTO + verify
 *   3. Soft delete (deleted_at) + active-only filter
 *   4. Uniqueness constraint on pid
 *   5. Bulk insert + query performance
 *   6. Frag count update (total_frags increment)
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <mysql.h>

/* ------------------------------------------------------------------ */
/*  Test framework                                                    */
/* ------------------------------------------------------------------ */

static MYSQL  *g_db      = NULL;
static int     g_pass    = 0;
static int     g_fail    = 0;
static char    g_last_error[4096];

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

/* ==================================================================
 * TEST 1: account_characters INSERT + SELECT roundtrip.
 * ================================================================== */
static void test_live_account_insert_select(void)
{
    TEST_BEGIN("live_account_insert_select");
    if (!mysql_ok()) { TEST_END(); return; }

    /* Clean up from previous runs */
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid = 999910");
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid = 999911");

    /* Insert two test characters */
    if (mysql_query(g_db,
        "INSERT INTO account_characters (account_name, pid, char_name) "
        "VALUES ('testaccount', 999910, 'TestHero')")) {
        TEST_FAIL("INSERT failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    if (mysql_query(g_db,
        "INSERT INTO account_characters (account_name, pid, char_name) "
        "VALUES ('testaccount', 999911, 'TestVillain')")) {
        TEST_FAIL("INSERT failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    /* SELECT them back */
    if (mysql_query(g_db, "SELECT pid, char_name, account_name FROM account_characters WHERE pid = 999910")) {
        TEST_FAIL("SELECT failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    MYSQL_RES *res = mysql_store_result(g_db);
    if (!res) { TEST_FAIL("no result"); TEST_END(); return; }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) { TEST_FAIL("no row for pid 999910"); mysql_free_result(res); TEST_END(); return; }

    if (strcmp(row[0], "999910") != 0) TEST_FAIL("pid mismatch: %s", row[0]);
    if (strcmp(row[1], "TestHero") != 0) TEST_FAIL("char_name mismatch: %s", row[1]);
    if (strcmp(row[2], "testaccount") != 0) TEST_FAIL("account_name mismatch: %s", row[2]);

    mysql_free_result(res);

    /* Verify second row */
    if (mysql_query(g_db, "SELECT COUNT(*) FROM account_characters WHERE account_name = 'testaccount'")) {
        TEST_FAIL("count query failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    res = mysql_store_result(g_db);
    row = mysql_fetch_row(res);
    if (!row || atoi(row[0]) < 2) TEST_FAIL("expected >= 2 characters for testaccount, got %s", row ? row[0] : "NULL");
    mysql_free_result(res);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid IN (999910, 999911)");

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 2: frag_leaderboard REPLACE INTO + verify roundtrip.
 * ================================================================== */
static void test_live_frag_leaderboard_roundtrip(void)
{
    TEST_BEGIN("live_frag_leaderboard_roundtrip");
    if (!mysql_ok()) { TEST_END(); return; }

    /* Setup - ensure account exists */
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid = 999920");
    mysql_query(g_db, "INSERT INTO account_characters (account_name, pid, char_name) "
                     "VALUES ('fragaccount', 999920, 'FragLord')");

    /* Clean previous frag data */
    mysql_query(g_db, "DELETE FROM frag_leaderboard WHERE pid = 999920");

    /* REPLACE INTO - matches production pattern */
    if (mysql_query(g_db,
        "REPLACE INTO frag_leaderboard "
        "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
        "VALUES (999920, 'fragaccount', 'FragLord', 50000, 1, 'human', 'warrior', 30, NULL)")) {
        TEST_FAIL("REPLACE INTO failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    /* SELECT back and verify ALL columns */
    if (mysql_query(g_db,
        "SELECT pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at "
        "FROM frag_leaderboard WHERE pid = 999920")) {
        TEST_FAIL("SELECT failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    MYSQL_RES *res = mysql_store_result(g_db);
    if (!res) { TEST_FAIL("no result"); TEST_END(); return; }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) { TEST_FAIL("no row for pid 999920"); mysql_free_result(res); TEST_END(); return; }

    /* Verify all columns match */
    if (strcmp(row[0], "999920") != 0)          TEST_FAIL("pid: %s", row[0]);
    if (strcmp(row[1], "fragaccount") != 0)     TEST_FAIL("account_name: %s", row[1]);
    if (strcmp(row[2], "FragLord") != 0)        TEST_FAIL("char_name: %s", row[2]);
    if (atoi(row[3]) != 50000)                  TEST_FAIL("total_frags: %s", row[3]);
    if (atoi(row[4]) != 1)                      TEST_FAIL("racewar: %s", row[4]);
    if (strcmp(row[5], "human") != 0)           TEST_FAIL("race: %s", row[5]);
    if (strcmp(row[6], "warrior") != 0)         TEST_FAIL("class: %s", row[6]);
    if (atoi(row[7]) != 30)                     TEST_FAIL("level: %s", row[7]);
    if (row[8] != NULL)                         TEST_FAIL("deleted_at should be NULL, got '%s'", row[8] ? row[8] : "NULL");

    mysql_free_result(res);

    /* Now REPLACE with updated frags and verify */
    if (mysql_query(g_db,
        "REPLACE INTO frag_leaderboard "
        "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
        "VALUES (999920, 'fragaccount', 'FragLord', 75000, 1, 'human', 'warrior', 35, NULL)")) {
        TEST_FAIL("REPLACE INTO update failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    /* Verify updated value */
    if (mysql_query(g_db, "SELECT total_frags, level FROM frag_leaderboard WHERE pid = 999920")) {
        TEST_FAIL("SELECT after update failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    res = mysql_store_result(g_db);
    row = mysql_fetch_row(res);
    if (!row || atoi(row[0]) != 75000) TEST_FAIL("total_frags not updated: expected 75000, got %s", row ? row[0] : "NULL");
    if (!row || atoi(row[1]) != 35)    TEST_FAIL("level not updated: expected 35, got %s", row ? row[1] : "NULL");
    mysql_free_result(res);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM frag_leaderboard WHERE pid = 999920");
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid = 999920");

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 3: Soft delete + active-only filter.
 *
 * Production uses: WHERE deleted_at IS NULL to filter active chars.
 * When a char is deleted: UPDATE frag_leaderboard SET deleted_at = NOW() WHERE pid = X
 * ================================================================== */
static void test_live_frag_soft_delete(void)
{
    TEST_BEGIN("live_frag_soft_delete");
    if (!mysql_ok()) { TEST_END(); return; }

    /* Setup */
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid IN (999930, 999931)");
    mysql_query(g_db, "DELETE FROM frag_leaderboard WHERE pid IN (999930, 999931)");

    mysql_query(g_db, "INSERT INTO account_characters (account_name, pid, char_name) "
                     "VALUES ('delaccount', 999930, 'ActiveGuy')");
    mysql_query(g_db, "INSERT INTO account_characters (account_name, pid, char_name) "
                     "VALUES ('delaccount', 999931, 'DeletedGuy')");

    mysql_query(g_db,
        "INSERT INTO frag_leaderboard "
        "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
        "VALUES (999930, 'delaccount', 'ActiveGuy', 100000, 1, 'elf', 'mage', 40, NULL)");
    mysql_query(g_db,
        "INSERT INTO frag_leaderboard "
        "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
        "VALUES (999931, 'delaccount', 'DeletedGuy', 200000, 2, 'drow_elf', 'necromancer', 50, NULL)");

    /* Soft-delete one character */
    if (mysql_query(g_db,
        "UPDATE frag_leaderboard SET deleted_at = NOW() WHERE pid = 999931")) {
        TEST_FAIL("soft delete failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    /* Active-only query should return only pid 999930 */
    if (mysql_query(g_db,
        "SELECT pid FROM frag_leaderboard WHERE account_name = 'delaccount' AND deleted_at IS NULL")) {
        TEST_FAIL("active query failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    MYSQL_RES *res = mysql_store_result(g_db);
    if (!res) { TEST_FAIL("no result"); TEST_END(); return; }

    int num_rows = mysql_num_rows(res);
    if (num_rows != 1) {
        TEST_FAIL("expected 1 active character, got %d", num_rows);
        mysql_free_result(res);
        TEST_END(); return;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (strcmp(row[0], "999930") != 0)
        TEST_FAIL("expected pid 999930 as active, got %s", row[0]);

    mysql_free_result(res);

    /* Verify deleted character IS in the table (just not active) */
    if (mysql_query(g_db,
        "SELECT COUNT(*) FROM frag_leaderboard WHERE pid = 999931")) {
        TEST_FAIL("deleted count query failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    res = mysql_store_result(g_db);
    row = mysql_fetch_row(res);
    if (!row || atoi(row[0]) != 1)
        TEST_FAIL("deleted character should still exist in table (soft delete)");
    mysql_free_result(res);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM frag_leaderboard WHERE pid IN (999930, 999931)");
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid IN (999930, 999931)");

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 4: Uniqueness constraint — duplicate pid should fail
 * or be handled by REPLACE INTO.
 * ================================================================== */
static void test_live_frag_unique_pid(void)
{
    TEST_BEGIN("live_frag_unique_pid");
    if (!mysql_ok()) { TEST_END(); return; }

    mysql_query(g_db, "DELETE FROM account_characters WHERE pid = 999940");
    mysql_query(g_db, "DELETE FROM frag_leaderboard WHERE pid = 999940");

    mysql_query(g_db, "INSERT INTO account_characters (account_name, pid, char_name) "
                     "VALUES ('uniqueacc', 999940, 'UniqueChar')");

    /* First insert */
    mysql_query(g_db,
        "INSERT INTO frag_leaderboard "
        "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
        "VALUES (999940, 'uniqueacc', 'UniqueChar', 30000, 1, 'dwarf', 'cleric', 25, NULL)");

    /* Duplicate INSERT should fail (UNIQUE KEY on pid) */
    int rc = mysql_query(g_db,
        "INSERT INTO frag_leaderboard "
        "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
        "VALUES (999940, 'uniqueacc', 'DupChar', 40000, 2, 'elf', 'thief', 30, NULL)");

    if (rc == 0) {
        /* Some MySQL configs may allow it — check that old data still there */
        if (mysql_query(g_db, "SELECT char_name, total_frags FROM frag_leaderboard WHERE pid = 999940")) {
            TEST_FAIL("SELECT after dup insert: %s", mysql_error(g_db));
            TEST_END(); return;
        }
        MYSQL_RES *res = mysql_store_result(g_db);
        MYSQL_ROW  row = mysql_fetch_row(res);
        /* Either way, data should exist */
        if (!row) TEST_FAIL("no data for pid 999940 after dup insert");
        mysql_free_result(res);
    }
    /* If rc != 0, duplicate was correctly rejected — that's also fine */

    /* Now use REPLACE INTO (production pattern) — should always work */
    if (mysql_query(g_db,
        "REPLACE INTO frag_leaderboard "
        "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
        "VALUES (999940, 'uniqueacc', 'ReplacedChar', 60000, 2, 'ogre', 'warrior', 35, NULL)")) {
        TEST_FAIL("REPLACE INTO failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    /* Verify REPLACE worked */
    if (mysql_query(g_db, "SELECT char_name, total_frags FROM frag_leaderboard WHERE pid = 999940")) {
        TEST_FAIL("SELECT after REPLACE failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    MYSQL_RES *res = mysql_store_result(g_db);
    MYSQL_ROW  row = mysql_fetch_row(res);
    if (!row) TEST_FAIL("no data after REPLACE");
    else {
        if (strcmp(row[0], "ReplacedChar") != 0) TEST_FAIL("REPLACE didn't update char_name: got %s", row[0]);
        if (atoi(row[1]) != 60000)               TEST_FAIL("REPLACE didn't update total_frags: got %s", row[1]);
    }
    mysql_free_result(res);

    /* Verify only ONE row */
    if (mysql_query(g_db, "SELECT COUNT(*) FROM frag_leaderboard WHERE pid = 999940")) {
        TEST_FAIL("count query failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    res = mysql_store_result(g_db);
    row = mysql_fetch_row(res);
    if (!row || atoi(row[0]) != 1)
        TEST_FAIL("expected exactly 1 row for pid 999940, got %s", row ? row[0] : "NULL");
    mysql_free_result(res);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM frag_leaderboard WHERE pid = 999940");
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid = 999940");

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 5: Bulk insert + query performance (100 rows).
 * ================================================================== */
static void test_live_frag_bulk_insert(void)
{
    TEST_BEGIN("live_frag_bulk_insert");
    if (!mysql_ok()) { TEST_END(); return; }

    /* Clean up from previous runs */
    for (int i = 0; i < 100; i++) {
        char q[256];
        snprintf(q, sizeof(q), "DELETE FROM account_characters WHERE pid = %d", 100000 + i);
        mysql_query(g_db, q);
        snprintf(q, sizeof(q), "DELETE FROM frag_leaderboard WHERE pid = %d", 100000 + i);
        mysql_query(g_db, q);
    }

    /* Insert 100 characters with frags */
    clock_t start = clock();

    for (int i = 0; i < 100; i++) {
        char q[512];
        snprintf(q, sizeof(q),
            "INSERT INTO account_characters (account_name, pid, char_name) "
            "VALUES ('bulkacc%d', %d, 'BulkChar%d')",
            i % 5, 100000 + i, i);
        if (mysql_query(g_db, q)) {
            TEST_FAIL("bulk INSERT account failed at i=%d: %s", i, mysql_error(g_db));
            TEST_END(); return;
        }

        snprintf(q, sizeof(q),
            "INSERT INTO frag_leaderboard "
            "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
            "VALUES (%d, 'bulkacc%d', 'BulkChar%d', %d, %d, 'human', 'warrior', %d, NULL)",
            100000 + i, i % 5, i, i * 1000, (i % 4) + 1, 25 + (i % 26));
        if (mysql_query(g_db, q)) {
            TEST_FAIL("bulk INSERT frag failed at i=%d: %s", i, mysql_error(g_db));
            TEST_END(); return;
        }
    }

    clock_t insert_end = clock();
    double insert_sec = (double)(insert_end - start) / CLOCKS_PER_SEC;

    /* Query leaderboard by racewar */
    if (mysql_query(g_db,
        "SELECT pid, total_frags FROM frag_leaderboard "
        "WHERE deleted_at IS NULL AND racewar = 1 "
        "ORDER BY total_frags DESC LIMIT 10")) {
        TEST_FAIL("racewar query failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    MYSQL_RES *res = mysql_store_result(g_db);
    int count = mysql_num_rows(res);
    if (count < 1) TEST_FAIL("racewar=1 query should return rows, got %d", count);
    mysql_free_result(res);

    /* Verify total count */
    if (mysql_query(g_db,
        "SELECT COUNT(*) FROM frag_leaderboard WHERE pid BETWEEN 100000 AND 100099 AND deleted_at IS NULL")) {
        TEST_FAIL("count query failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    res = mysql_store_result(g_db);
    MYSQL_ROW row = mysql_fetch_row(res);
    int total = row ? atoi(row[0]) : 0;
    if (total != 100) TEST_FAIL("expected 100 total rows, got %d", total);
    mysql_free_result(res);

    printf(" (%.2fs for 100 inserts)", insert_sec);

    /* Cleanup */
    for (int i = 0; i < 100; i++) {
        char q[256];
        snprintf(q, sizeof(q), "DELETE FROM frag_leaderboard WHERE pid = %d", 100000 + i);
        mysql_query(g_db, q);
        snprintf(q, sizeof(q), "DELETE FROM account_characters WHERE pid = %d", 100000 + i);
        mysql_query(g_db, q);
    }

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 6: Frag count increment — atomic UPDATE total_frags = total_frags + delta.
 * ================================================================== */
static void test_live_frag_increment(void)
{
    TEST_BEGIN("live_frag_increment");
    if (!mysql_ok()) { TEST_END(); return; }

    mysql_query(g_db, "DELETE FROM account_characters WHERE pid = 999960");
    mysql_query(g_db, "DELETE FROM frag_leaderboard WHERE pid = 999960");

    mysql_query(g_db, "INSERT INTO account_characters (account_name, pid, char_name) "
                     "VALUES ('incaccount', 999960, 'Incrementor')");
    mysql_query(g_db,
        "INSERT INTO frag_leaderboard "
        "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
        "VALUES (999960, 'incaccount', 'Incrementor', 10000, 1, 'human', 'warrior', 30, NULL)");

    /* Increment frags 5 times */
    for (int i = 0; i < 5; i++) {
        if (mysql_query(g_db,
            "UPDATE frag_leaderboard SET total_frags = total_frags + 2500 WHERE pid = 999960")) {
            TEST_FAIL("increment %d failed: %s", i, mysql_error(g_db));
            TEST_END(); return;
        }
    }

    /* Verify total */
    if (mysql_query(g_db, "SELECT total_frags FROM frag_leaderboard WHERE pid = 999960")) {
        TEST_FAIL("SELECT after increments failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    MYSQL_RES *res = mysql_store_result(g_db);
    MYSQL_ROW  row = mysql_fetch_row(res);
    if (!row) TEST_FAIL("no data after increments");
    else if (atoi(row[0]) != 10000 + 5 * 2500)
        TEST_FAIL("expected %d total_frags, got %s", 10000 + 5 * 2500, row[0]);
    mysql_free_result(res);

    /* Cleanup */
    mysql_query(g_db, "DELETE FROM frag_leaderboard WHERE pid = 999960");
    mysql_query(g_db, "DELETE FROM account_characters WHERE pid = 999960");

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test registry & runner                                             */
/* ================================================================== */

typedef struct { const char *name; void (*func)(void); } test_entry;

static test_entry g_tests[] = {
    { "live_account_insert_select",       test_live_account_insert_select },
    { "live_frag_leaderboard_roundtrip",  test_live_frag_leaderboard_roundtrip },
    { "live_frag_soft_delete",            test_live_frag_soft_delete },
    { "live_frag_unique_pid",             test_live_frag_unique_pid },
    { "live_frag_bulk_insert",            test_live_frag_bulk_insert },
    { "live_frag_increment",              test_live_frag_increment },
    { NULL, NULL },
};

int test_live_frags_run(MYSQL *db)
{
    g_db   = db;
    g_pass = g_fail = 0;
    memset(g_last_error, 0, sizeof(g_last_error));

    printf("\n=== Live Frag & Item Transfer Tests ===\n");
    for (int i = 0; g_tests[i].name; i++)
        g_tests[i].func();

    int total = g_pass + g_fail;
    printf("  Live Frag Transfer: %d/%d passed\n", g_pass, total);
    return g_fail;
}
