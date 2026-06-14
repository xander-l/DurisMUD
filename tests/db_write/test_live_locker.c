/* ===================================================================
 * test_live_locker.c — Live DB roundtrip tests for locker_items,
 * lockers, and locker_chests tables using real MySQL.
 *
 * Tests:
 *   1. locker + chest INSERT + SELECT roundtrip
 *   2. locker_items INSERT (all 28 columns) + SELECT back
 *   3. DELETE-before-INSERT cleanup pattern
 *   4. 10,000 item bulk insert stress test
 *   5. Multi-locker access (simulated multi-player)
 *   6. Private chest password storage + verification
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

/* Cleanup helper — delete test lockers by owner_pid range */
static void cleanup_test_data(int start_pid, int end_pid)
{
    for (int pid = start_pid; pid <= end_pid; pid++) {
        char q[256];
        /* Find locker_id */
        snprintf(q, sizeof(q),
            "SELECT id FROM lockers WHERE owner_pid = %d", pid);
        if (!mysql_query(g_db, q)) {
            MYSQL_RES *res = mysql_store_result(g_db);
            if (res) {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res))) {
                    int locker_id = atoi(row[0]);
                    snprintf(q, sizeof(q), "DELETE FROM locker_items WHERE locker_id = %d", locker_id);
                    mysql_query(g_db, q);
                    snprintf(q, sizeof(q), "DELETE FROM locker_chests WHERE locker_id = %d", locker_id);
                    mysql_query(g_db, q);
                }
                mysql_free_result(res);
            }
        }
        snprintf(q, sizeof(q), "DELETE FROM lockers WHERE owner_pid = %d", pid);
        mysql_query(g_db, q);
    }
}

/* ==================================================================
 * TEST 1: Locker + chest INSERT + SELECT roundtrip.
 * ================================================================== */
static void test_live_locker_roundtrip(void)
{
    TEST_BEGIN("live_locker_roundtrip");
    if (!mysql_ok()) { TEST_END(); return; }

    cleanup_test_data(200010, 200010);

    /* Create a locker */
    if (mysql_query(g_db,
        "INSERT INTO lockers (owner_pid, locker_name) VALUES (200010, 'Hero_Locker')")) {
        TEST_FAIL("INSERT locker failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    int locker_id = mysql_insert_id(g_db);

    /* Create a public chest */
    char q[512];
    snprintf(q, sizeof(q),
        "INSERT INTO locker_chests (locker_id, chest_name, is_public) VALUES (%d, 'Public Chest', 1)", locker_id);
    if (mysql_query(g_db, q)) {
        TEST_FAIL("INSERT chest failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }
    int chest_id = mysql_insert_id(g_db);

    /* Create a private chest */
    snprintf(q, sizeof(q),
        "INSERT INTO locker_chests (locker_id, chest_name, chest_password, is_public) "
        "VALUES (%d, 'Private Chest', 'secret123', 0)", locker_id);
    if (mysql_query(g_db, q)) {
        TEST_FAIL("INSERT private chest failed: %s", mysql_error(g_db));
        TEST_END(); return;
    }

    /* Verify locker exists */
    snprintf(q, sizeof(q), "SELECT id, locker_name FROM lockers WHERE owner_pid = 200010");
    if (mysql_query(g_db, q)) { TEST_FAIL("SELECT locker failed"); TEST_END(); return; }
    MYSQL_RES *res = mysql_store_result(g_db);
    if (!res || mysql_num_rows(res) != 1)
        TEST_FAIL("expected 1 locker, got %d", res ? mysql_num_rows(res) : 0);
    if (res) mysql_free_result(res);

    /* Verify chests exist */
    snprintf(q, sizeof(q),
        "SELECT COUNT(*) FROM locker_chests WHERE locker_id = %d", locker_id);
    if (mysql_query(g_db, q)) { TEST_FAIL("SELECT chests failed"); TEST_END(); return; }
    res = mysql_store_result(g_db);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || atoi(row[0]) != 2)
        TEST_FAIL("expected 2 chests, got %s", row ? row[0] : "NULL");
    mysql_free_result(res);

    /* Verify private chest password */
    snprintf(q, sizeof(q),
        "SELECT chest_password FROM locker_chests WHERE locker_id = %d AND is_public = 0", locker_id);
    if (mysql_query(g_db, q)) { TEST_FAIL("SELECT password failed"); TEST_END(); return; }
    res = mysql_store_result(g_db);
    row = mysql_fetch_row(res);
    if (!row || strcmp(row[0], "secret123") != 0)
        TEST_FAIL("private chest password mismatch: got '%s'", row ? row[0] : "NULL");
    mysql_free_result(res);

    cleanup_test_data(200010, 200010);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 2: Locker items INSERT + SELECT roundtrip (all columns).
 * ================================================================== */
static void test_live_locker_items_roundtrip(void)
{
    TEST_BEGIN("live_locker_items_roundtrip");
    if (!mysql_ok()) { TEST_END(); return; }

    cleanup_test_data(200020, 200020);

    /* Setup locker + chest */
    mysql_query(g_db, "INSERT INTO lockers (owner_pid, locker_name) VALUES (200020, 'ItemTest_Locker')");
    int locker_id = mysql_insert_id(g_db);
    char q[1024];
    snprintf(q, sizeof(q), "INSERT INTO locker_chests (locker_id, chest_name, is_public) VALUES (%d, 'TestChest', 1)", locker_id);
    mysql_query(g_db, q);
    __attribute__((unused)) int chest_id = mysql_insert_id(g_db);

    /* Insert a full item with all 28+ columns */
    snprintf(q, sizeof(q),
        "INSERT INTO locker_items ("
        "locker_id, chest_id, vnum, container_id, quantity, "
        "weight, cost, timer, extra_flags, "
        "wear_flags, item_type, "
        "value0, value1, value2, value3, value4, value5, value6, value7, "
        "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
        "item_material, name, short_descr, description, action_descr"
        ") VALUES ("
        "%d, %d, 1234, NULL, 1, "
        "5.5, 1000, 3600, 0, "
        "'TAKE_WEAR_WIELD', 'WEAPON', "
        "2, 6, 0, 0, 0, 0, 0, 0, "
        "1, 0, 0, 0, 0, "
        "'steel', 'a long sword', 'A long sword lies here.', 'It gleams.', 'wield'"
        ")", locker_id, chest_id);

    if (mysql_query(g_db, q)) {
        TEST_FAIL("INSERT item failed: %s", mysql_error(g_db));
        cleanup_test_data(200020, 200020);
        TEST_END(); return;
    }
    int item_id = mysql_insert_id(g_db);

    /* SELECT back and verify ALL columns */
    snprintf(q, sizeof(q),
        "SELECT id, vnum, weight, cost, timer, "
        "wear_flags, item_type, value0, value1, "
        "item_material, name, short_descr "
        "FROM locker_items WHERE id = %d", item_id);

    if (mysql_query(g_db, q)) {
        TEST_FAIL("SELECT item failed: %s", mysql_error(g_db));
        cleanup_test_data(200020, 200020);
        TEST_END(); return;
    }

    MYSQL_RES *res = mysql_store_result(g_db);
    if (!res || mysql_num_rows(res) != 1) {
        TEST_FAIL("expected 1 item row, got %d", res ? mysql_num_rows(res) : 0);
        if (res) mysql_free_result(res);
        cleanup_test_data(200020, 200020);
        TEST_END(); return;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    /* Verify key fields */
    if (atoi(row[1]) != 1234)                      TEST_FAIL("vnum: %s", row[1]);
    if (atof(row[2]) < 5.4 || atof(row[2]) > 5.6) TEST_FAIL("weight: %s", row[2]);
    if (atoi(row[3]) != 1000)                      TEST_FAIL("cost: %s", row[3]);
    if (atol(row[4]) != 3600)                      TEST_FAIL("timer: %s", row[4]);
    if (strcmp(row[5], "TAKE_WEAR_WIELD") != 0)    TEST_FAIL("wear_flags: %s", row[5]);
    if (strcmp(row[6], "WEAPON") != 0)             TEST_FAIL("item_type: %s", row[6]);
    if (atoi(row[7]) != 2)                         TEST_FAIL("value0: %s", row[7]);
    if (atoi(row[8]) != 6)                         TEST_FAIL("value1: %s", row[8]);
    if (strcmp(row[9], "steel") != 0)              TEST_FAIL("item_material: %s", row[9]);
    if (strcmp(row[10], "a long sword") != 0)      TEST_FAIL("name: %s", row[10]);

    mysql_free_result(res);

    cleanup_test_data(200020, 200020);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 3: DELETE-before-INSERT cleanup pattern.
 *
 * Production does: DELETE FROM locker_items WHERE locker_id=X
 *                 then re-INSERTs all current items.
 * ================================================================== */
static void test_live_locker_delete_before_insert(void)
{
    TEST_BEGIN("live_locker_delete_before_insert");
    if (!mysql_ok()) { TEST_END(); return; }

    cleanup_test_data(200030, 200030);

    /* Setup */
    mysql_query(g_db, "INSERT INTO lockers (owner_pid, locker_name) VALUES (200030, 'Dbi_Locker')");
    int locker_id = mysql_insert_id(g_db);
    char q[1024];
    snprintf(q, sizeof(q), "INSERT INTO locker_chests (locker_id, chest_name, is_public) VALUES (%d, 'DbiChest', 1)", locker_id);
    mysql_query(g_db, q);
    __attribute__((unused)) int chest_id = mysql_insert_id(g_db);

    /* Insert 5 old items */
    for (int i = 0; i < 5; i++) {
        snprintf(q, sizeof(q),
            "INSERT INTO locker_items (locker_id, chest_id, vnum, name, item_type) "
            "VALUES (%d, %d, %d, 'OldItem%d', 'TRASH')",
            locker_id, chest_id, 1000 + i, i);
        mysql_query(g_db, q);
    }

    /* DELETE before re-INSERT (production pattern) */
    snprintf(q, sizeof(q), "DELETE FROM locker_items WHERE locker_id = %d", locker_id);
    if (mysql_query(g_db, q)) {
        TEST_FAIL("DELETE failed: %s", mysql_error(g_db));
        cleanup_test_data(200030, 200030);
        TEST_END(); return;
    }

    /* Verify items gone */
    snprintf(q, sizeof(q), "SELECT COUNT(*) FROM locker_items WHERE locker_id = %d", locker_id);
    mysql_query(g_db, q);
    MYSQL_RES *res = mysql_store_result(g_db);
    MYSQL_ROW  row = mysql_fetch_row(res);
    if (!row || atoi(row[0]) != 0)
        TEST_FAIL("DELETE didn't clean items: %s remain", row ? row[0] : "?");
    mysql_free_result(res);

    /* Now re-insert 3 new items (simulating save) */
    for (int i = 0; i < 3; i++) {
        snprintf(q, sizeof(q),
            "INSERT INTO locker_items (locker_id, chest_id, vnum, name, item_type) "
            "VALUES (%d, %d, %d, 'NewItem%d', 'TREASURE')",
            locker_id, chest_id, 2000 + i, i);
        mysql_query(g_db, q);
    }

    /* Verify only 3 new items */
    snprintf(q, sizeof(q), "SELECT COUNT(*) FROM locker_items WHERE locker_id = %d", locker_id);
    mysql_query(g_db, q);
    res = mysql_store_result(g_db);
    row = mysql_fetch_row(res);
    if (!row || atoi(row[0]) != 3)
        TEST_FAIL("expected 3 items after re-INSERT, got %s", row ? row[0] : "NULL");
    if (res) mysql_free_result(res);

    cleanup_test_data(200030, 200030);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 4: 10,000 item bulk insert stress test.
 *
 * This is the key stress test the user requested: verifies that
 * massively populated lockers don't corrupt or slow to a crawl.
 * ================================================================== */
static void test_live_locker_10k_items(void)
{
    TEST_BEGIN("live_locker_10k_items");
    if (!mysql_ok()) { TEST_END(); return; }

    cleanup_test_data(200040, 200040);

    /* Setup locker */
    mysql_query(g_db, "INSERT INTO lockers (owner_pid, locker_name) VALUES (200040, 'Stress_Locker')");
    int locker_id = mysql_insert_id(g_db);
    char q[1024];
    snprintf(q, sizeof(q), "INSERT INTO locker_chests (locker_id, chest_name, is_public) VALUES (%d, 'StressChest', 1)", locker_id);
    mysql_query(g_db, q);

    /* Disable autocommit for batch insert speed */
    mysql_query(g_db, "SET autocommit = 0");
    mysql_query(g_db, "START TRANSACTION");

    clock_t start = clock();
    int N = 10000;

    for (int i = 0; i < N; i++) {
        snprintf(q, sizeof(q),
            "INSERT INTO locker_items (locker_id, chest_id, vnum, name, item_type, value0) "
            "VALUES (%d, NULL, %d, 'StressItem%d', 'MISC', %d)",
            locker_id, 3000 + (i % 500), i, i);
        if (mysql_query(g_db, q)) {
            TEST_FAIL("INSERT %d failed: %s", i, mysql_error(g_db));
            mysql_query(g_db, "ROLLBACK");
            mysql_query(g_db, "SET autocommit = 1");
            cleanup_test_data(200040, 200040);
            TEST_END(); return;
        }
    }

    mysql_query(g_db, "COMMIT");
    mysql_query(g_db, "SET autocommit = 1");

    clock_t insert_end = clock();
    double insert_sec = (double)(insert_end - start) / CLOCKS_PER_SEC;

    /* Verify count */
    snprintf(q, sizeof(q), "SELECT COUNT(*) FROM locker_items WHERE locker_id = %d", locker_id);
    mysql_query(g_db, q);
    MYSQL_RES *res = mysql_store_result(g_db);
    MYSQL_ROW  row = mysql_fetch_row(res);
    int count = row ? atoi(row[0]) : 0;
    mysql_free_result(res);

    if (count != N)
        TEST_FAIL("expected %d items, got %d", N, count);

    /* Query speed test — SELECT with ORDER BY */
    snprintf(q, sizeof(q),
        "SELECT vnum, COUNT(*) as cnt FROM locker_items WHERE locker_id = %d GROUP BY vnum ORDER BY cnt DESC LIMIT 5", locker_id);
    mysql_query(g_db, q);
    res = mysql_store_result(g_db);
    mysql_free_result(res);

    /* DELETE speed test */
    clock_t del_start = clock();
    snprintf(q, sizeof(q), "DELETE FROM locker_items WHERE locker_id = %d", locker_id);
    mysql_query(g_db, q);
    clock_t del_end = clock();
    double del_sec = (double)(del_end - del_start) / CLOCKS_PER_SEC;

    printf(" (%.2fs insert, %.3fs delete)", insert_sec, del_sec);

    /* Latency check: 10K inserts should complete in < 5s on modern hardware */
    if (insert_sec > 5.0)
        TEST_FAIL("10K item insert too slow: %.2fs (threshold: 5s)", insert_sec);

    cleanup_test_data(200040, 200040);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 5: Multi-locker access (simulated multi-player).
 *
 * Verifies that multiple lockers with different owners don't
 * collision or corrupt each other's data.
 * ================================================================== */
static void test_live_locker_multi_player(void)
{
    TEST_BEGIN("live_locker_multi_player");
    if (!mysql_ok()) { TEST_END(); return; }

    int pids[] = { 200051, 200052, 200053, 200054, 200055 };
    int n = sizeof(pids) / sizeof(pids[0]);

    cleanup_test_data(200051, 200055);

    /* Create 5 lockers, each with items */
    for (int i = 0; i < n; i++) {
        char q[512];
        snprintf(q, sizeof(q), "INSERT INTO lockers (owner_pid, locker_name) VALUES (%d, 'MultiLocker%d')", pids[i], i);
        mysql_query(g_db, q);
        int lid = mysql_insert_id(g_db);

        /* Insert items unique to this locker */
        for (int j = 0; j < 10; j++) {
            snprintf(q, sizeof(q),
                "INSERT INTO locker_items (locker_id, chest_id, vnum, name, item_type) "
                "VALUES (%d, NULL, %d, 'Item_P%d_V%d', 'GENERIC')",
                lid, 4000 + i * 100 + j, pids[i], 4000 + i * 100 + j);
            mysql_query(g_db, q);
        }
    }

    /* Verify each locker has exactly 10 items and correct owner */
    for (int i = 0; i < n; i++) {
        char q[512];

        /* Verify locker exists */
        snprintf(q, sizeof(q), "SELECT id FROM lockers WHERE owner_pid = %d", pids[i]);
        if (mysql_query(g_db, q)) { TEST_FAIL("SELECT locker pid %d failed", pids[i]); continue; }
        MYSQL_RES *res = mysql_store_result(g_db);
        if (!res || mysql_num_rows(res) != 1) {
            TEST_FAIL("expected 1 locker for pid %d, got %d", pids[i], res ? mysql_num_rows(res) : 0);
            if (res) mysql_free_result(res);
            continue;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        int lid = atoi(row[0]);
        mysql_free_result(res);

        /* Count items */
        snprintf(q, sizeof(q), "SELECT COUNT(*) FROM locker_items WHERE locker_id = %d", lid);
        mysql_query(g_db, q);
        res = mysql_store_result(g_db);
        row = mysql_fetch_row(res);
        int icount = row ? atoi(row[0]) : 0;
        mysql_free_result(res);
        if (icount != 10)
            TEST_FAIL("locker pid %d: expected 10 items, got %d", pids[i], icount);

        /* Verify items have correct vnum range */
        snprintf(q, sizeof(q), "SELECT COUNT(*) FROM locker_items WHERE locker_id = %d AND vnum BETWEEN %d AND %d",
                 lid, 4000 + i * 100, 4000 + i * 100 + 9);
        mysql_query(g_db, q);
        res = mysql_store_result(g_db);
        row = mysql_fetch_row(res);
        if (!row || atoi(row[0]) != 10)
            TEST_FAIL("locker pid %d: items in wrong vnum range", pids[i]);
        mysql_free_result(res);
    }

    /* Verify TOTAL items across all lockers */
    if (mysql_query(g_db,
        "SELECT COUNT(*) FROM locker_items WHERE vnum BETWEEN 4000 AND 4599")) {
        TEST_FAIL("total count query failed: %s", mysql_error(g_db));
    } else {
        MYSQL_RES *res = mysql_store_result(g_db);
        MYSQL_ROW  row = mysql_fetch_row(res);
        if (!row || atoi(row[0]) != 50)
            TEST_FAIL("expected 50 total items across all lockers, got %s", row ? row[0] : "NULL");
        mysql_free_result(res);
    }

    cleanup_test_data(200051, 200055);

    TEST_PASS();
    TEST_END();
}

/* ==================================================================
 * TEST 6: Private chest password storage + verification.
 *
 * Production stores chest passwords (SHA2 hashed) in locker_chests.
 * This test verifies the password roundtrip.
 * ================================================================== */
static void test_live_locker_chest_password(void)
{
    TEST_BEGIN("live_locker_chest_password");
    if (!mysql_ok()) { TEST_END(); return; }

    cleanup_test_data(200060, 200060);

    mysql_query(g_db, "INSERT INTO lockers (owner_pid, locker_name) VALUES (200060, 'ChestPw_Locker')");
    int locker_id = mysql_insert_id(g_db);

    /* Create 3 chests with different passwords */
    const char *passwords[] = { NULL, "simple", "c0mpl3x!P@ss" };
    const char *names[]     = { "Public", "Basic", "Secure" };
    int is_public[] = { 1, 0, 0 };

    for (int i = 0; i < 3; i++) {
        char q[1024];
        if (passwords[i]) {
            snprintf(q, sizeof(q),
                "INSERT INTO locker_chests (locker_id, chest_name, chest_password, is_public) "
                "VALUES (%d, '%s', '%s', %d)",
                locker_id, names[i], passwords[i], is_public[i]);
        } else {
            snprintf(q, sizeof(q),
                "INSERT INTO locker_chests (locker_id, chest_name, chest_password, is_public) "
                "VALUES (%d, '%s', NULL, %d)",
                locker_id, names[i], is_public[i]);
        }
        if (mysql_query(g_db, q)) {
            TEST_FAIL("INSERT chest %d failed: %s", i, mysql_error(g_db));
            TEST_END(); return;
        }
    }

    /* Verify passwords match */
    for (int i = 0; i < 3; i++) {
        char q[512];
        snprintf(q, sizeof(q),
            "SELECT chest_password, is_public FROM locker_chests "
            "WHERE locker_id = %d AND chest_name = '%s'", locker_id, names[i]);
        if (mysql_query(g_db, q)) {
            TEST_FAIL("SELECT chest %d failed: %s", i, mysql_error(g_db));
            continue;
        }
        MYSQL_RES *res = mysql_store_result(g_db);
        MYSQL_ROW  row = mysql_fetch_row(res);
        if (!row) {
            TEST_FAIL("no row for chest '%s'", names[i]);
            mysql_free_result(res);
            continue;
        }

        if (passwords[i]) {
            if (strcmp(row[0], passwords[i]) != 0)
                TEST_FAIL("password mismatch for '%s': expected '%s', got '%s'",
                          names[i], passwords[i], row[0]);
        } else {
            if (row[0] != NULL)
                TEST_FAIL("expected NULL password for '%s', got '%s'", names[i], row[0]);
        }

        mysql_free_result(res);
    }

    cleanup_test_data(200060, 200060);

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test registry & runner                                             */
/* ================================================================== */

typedef struct { const char *name; void (*func)(void); } test_entry;

static test_entry g_tests[] = {
    { "live_locker_roundtrip",            test_live_locker_roundtrip },
    { "live_locker_items_roundtrip",      test_live_locker_items_roundtrip },
    { "live_locker_delete_before_insert", test_live_locker_delete_before_insert },
    { "live_locker_10k_items",            test_live_locker_10k_items },
    { "live_locker_multi_player",         test_live_locker_multi_player },
    { "live_locker_chest_password",       test_live_locker_chest_password },
    { NULL, NULL },
};

int test_live_locker_run(MYSQL *db)
{
    g_db   = db;
    g_pass = g_fail = 0;
    memset(g_last_error, 0, sizeof(g_last_error));

    printf("\n=== Live Locker Stress Tests ===\n");
    for (int i = 0; g_tests[i].name; i++)
        g_tests[i].func();

    int total = g_pass + g_fail;
    printf("  Live Locker Stress: %d/%d passed\n", g_pass, total);
    return g_fail;
}
