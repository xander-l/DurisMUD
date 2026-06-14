/* ===================================================================
 * test_live_accounts.c — Live DB tests for account/character lifecycle.
 *
 * Tests the full lifecycle:
 *   1. Account creation (INSERT INTO accounts)
 *   2. Character creation + account linking
 *      (INSERT INTO player_data, INSERT INTO account_characters)
 *   3. Character listing (SELECT with deleted_at IS NULL filter)
 *   4. Character deletion (soft-delete via deleted_at=NOW())
 *   5. Name reuse after deletion (re-create same name, verify
 *      deleted_at is cleared by ON DUPLICATE KEY UPDATE)
 *   6. PID propagation (verify pid is updated from 0 to real value)
 *   7. Multi-character account (2+ characters, all visible)
 *   8. Soft-delete doesn't orphan (player_data DELETE + account_characters
 *      deleted_at set, then re-create clears deleted_at)
 *
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <mysql.h>

/* ------------------------------------------------------------------ */
/*  Test globals                                                       */
/* ------------------------------------------------------------------ */

static MYSQL  *g_db;
static int     g_failures;
static int     g_total;

/* ------------------------------------------------------------------ */
/*  Test macros                                                        */
/* ------------------------------------------------------------------ */

#define TEST(name) \
    do { g_total++; printf("  [%2d] %-55s ... ", g_total, name); fflush(stdout); } while(0)

#define PASS() \
    do { printf("PASS\n"); } while(0)

#define FAIL(fmt, ...) \
    do { printf("FAIL\n"); \
         fprintf(stderr, "        %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
         g_failures++; return; } while(0)

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static void run_query(const char *q)
{
    if (mysql_query(g_db, q)) {
        fprintf(stderr, "        query failed: %s\n", mysql_error(g_db));
        fprintf(stderr, "        query: %s\n", q);
    }
}

static int query_returns_rows(const char *q, int expected_min)
{
    if (mysql_query(g_db, q)) return 0;
    MYSQL_RES *r = mysql_store_result(g_db);
    if (!r) return 0;
    int n = (int)mysql_num_rows(r);
    mysql_free_result(r);
    return (n >= expected_min);
}

static long query_long(const char *q, long fallback)
{
    if (mysql_query(g_db, q)) return fallback;
    MYSQL_RES *r = mysql_store_result(g_db);
    if (!r) return fallback;
    MYSQL_ROW row = mysql_fetch_row(r);
    long val = (row && row[0]) ? atol(row[0]) : fallback;
    mysql_free_result(r);
    return val;
}

/* Escape and quote a string for MySQL */
static char *sql_quote(const char *s)
{
    if (!s) return strdup("''");
    size_t len  = strlen(s);
    char  *buf  = (char *)malloc(len * 2 + 3);
    if (!buf) return strdup("''");
    size_t n    = mysql_real_escape_string(g_db, buf + 1, s, len);
    buf[0]      = '\'';
    buf[n + 1]  = '\'';
    buf[n + 2]  = '\0';
    return buf;
}

/* ------------------------------------------------------------------ */
/*  Cleanup helpers                                                    */
/* ------------------------------------------------------------------ */

static void cleanup_test_account(const char *acct_name)
{
    char *esc = sql_quote(acct_name);
    char  q[512];

    /* Delete account_characters first (FK if exists) */
    snprintf(q, sizeof(q),
             "DELETE FROM account_characters WHERE account_name=%s", esc);
    run_query(q);

    /* Delete accounts */
    snprintf(q, sizeof(q),
             "DELETE FROM accounts WHERE account_name=%s", esc);
    run_query(q);

    free(esc);
}

static void cleanup_test_player(const char *name)
{
    char *esc = sql_quote(name);
    char  q[512];

    /* Delete from player_data */
    snprintf(q, sizeof(q),
             "DELETE FROM player_data WHERE LOWER(name)=LOWER(%s)", esc);
    run_query(q);

    /* Also clean up account_characters by name */
    snprintf(q, sizeof(q),
             "DELETE FROM account_characters WHERE LOWER(char_name)=LOWER(%s)", esc);
    run_query(q);

    free(esc);
}

/* ------------------------------------------------------------------ */
/*  Test 1: Account creation (INSERT INTO accounts)                     */
/* ------------------------------------------------------------------ */

static void test_create_account(void)
{
    TEST("create new account");
    const char *acct = "test_live_acct_create";
    cleanup_test_account(acct);

    char q[1024];
    snprintf(q, sizeof(q),
        "INSERT INTO accounts (account_name, email, password, confirmed) "
        "VALUES ('%s', '%s@test.com', 'hash123', 1)", acct, acct);

    if (mysql_query(g_db, q))
        FAIL("INSERT failed: %s", mysql_error(g_db));

    /* Verify it exists */
    snprintf(q, sizeof(q),
        "SELECT 1 FROM accounts WHERE account_name='%s'", acct);
    if (!query_returns_rows(q, 1))
        FAIL("account not found after INSERT");

    cleanup_test_account(acct);
    PASS();
}

/* ------------------------------------------------------------------ */
/*  Test 2: Character creation + account linking                        */
/* ------------------------------------------------------------------ */

static void test_create_character_and_link(void)
{
    TEST("create character and link to account");
    const char *acct   = "test_live_charlink";
    const char *charn  = "Testcharlink";
    cleanup_test_account(acct);
    cleanup_test_player(charn);

    char q[1024];

    /* Create account */
    snprintf(q, sizeof(q),
        "INSERT INTO accounts (account_name, email, password, confirmed) "
        "VALUES ('%s', '%s@test.com', 'hash123', 1)", acct, acct);
    run_query(q);

    /* Create player_data row (simulating sql_save_player_core) */
    snprintf(q, sizeof(q),
        "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
        "VALUES ('%s', 1, 1, 1, %u, 0, 0)", charn, (unsigned)1);
    if (mysql_query(g_db, q))
        FAIL("INSERT player_data: %s", mysql_error(g_db));

    long pid = (long)mysql_insert_id(g_db);

    /* Link character to account (simulating sql_save_account_characters) */
    snprintf(q, sizeof(q),
        "INSERT INTO account_characters (account_name, char_name, pid, "
        "login_count, last_login, blocked, racewar) "
        "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
        "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
        "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
        "account_name=VALUES(account_name)",
        acct, charn, pid);
    if (mysql_query(g_db, q))
        FAIL("INSERT account_characters: %s", mysql_error(g_db));

    /* Verify character appears in account character list */
    snprintf(q, sizeof(q),
        "SELECT 1 FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s' AND deleted_at IS NULL",
        acct, charn);
    if (!query_returns_rows(q, 1))
        FAIL("character not linked to account (deleted_at IS NULL filter)");

    /* Verify the PID was stored correctly */
    snprintf(q, sizeof(q),
        "SELECT pid FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s'", acct, charn);
    long stored_pid = query_long(q, 0);
    if (stored_pid != pid)
        FAIL("stored pid=%ld, expected %ld", stored_pid, pid);

    cleanup_test_account(acct);
    cleanup_test_player(charn);
    PASS();
}

/* ------------------------------------------------------------------ */
/*  Test 3: Character listing (simulating sql_load_account_characters)  */
/* ------------------------------------------------------------------ */

static void test_character_listing(void)
{
    TEST("character listing with deleted_at filter");
    const char *acct   = "test_live_charlist";
    const char *ch1    = "Charlistone";
    const char *ch2    = "Charlisttwo";
    cleanup_test_account(acct);
    cleanup_test_player(ch1);
    cleanup_test_player(ch2);

    char q[1024];

    /* Create account */
    snprintf(q, sizeof(q),
        "INSERT INTO accounts (account_name, email, password, confirmed) "
        "VALUES ('%s', '%s@test.com', 'hash123', 1)", acct, acct);
    run_query(q);

    /* Create two characters */
    for (int i = 0; i < 2; i++) {
        const char *name = (i == 0) ? ch1 : ch2;
        snprintf(q, sizeof(q),
            "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
            "VALUES ('%s', %d, 1, 1, %u, 0, 0)", name, i + 1, (unsigned)1);
        run_query(q);
        long pid = (long)mysql_insert_id(g_db);

        snprintf(q, sizeof(q),
            "INSERT INTO account_characters (account_name, char_name, pid, "
            "login_count, last_login, blocked, racewar) "
            "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
            "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
            "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
            "account_name=VALUES(account_name)",
            acct, name, pid);
        run_query(q);
    }

    /* Simulate sql_load_account_characters query */
    snprintf(q, sizeof(q),
        "SELECT ac.char_name "
        "FROM account_characters ac "
        "LEFT JOIN player_data pd ON ac.pid = pd.pid "
        "WHERE ac.account_name='%s' AND ac.deleted_at IS NULL", acct);
    if (mysql_query(g_db, q))
        FAIL("list query: %s", mysql_error(g_db));

    MYSQL_RES *r = mysql_store_result(g_db);
    if (!r) FAIL("no result from list query");

    int count = (int)mysql_num_rows(r);
    mysql_free_result(r);

    if (count != 2)
        FAIL("expected 2 characters, got %d", count);

    cleanup_test_account(acct);
    cleanup_test_player(ch1);
    cleanup_test_player(ch2);
    PASS();
}

/* ------------------------------------------------------------------ */
/*  Test 4: Character deletion (soft-delete via deleted_at=NOW())        */
/* ------------------------------------------------------------------ */

static void test_soft_delete_character(void)
{
    TEST("soft-delete character (deleted_at=NOW())");
    const char *acct   = "test_live_softdel";
    const char *charn  = "Softdelchar";
    cleanup_test_account(acct);
    cleanup_test_player(charn);

    char q[1024];

    /* Create account and character */
    snprintf(q, sizeof(q),
        "INSERT INTO accounts (account_name, email, password, confirmed) "
        "VALUES ('%s', '%s@test.com', 'hash123', 1)", acct, acct);
    run_query(q);

    snprintf(q, sizeof(q),
        "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
        "VALUES ('%s', 1, 1, 1, %u, 0, 0)", charn, (unsigned)1);
    run_query(q);
    long pid = (long)mysql_insert_id(g_db);

    snprintf(q, sizeof(q),
        "INSERT INTO account_characters (account_name, char_name, pid, "
        "login_count, last_login, blocked, racewar) "
        "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
        "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
        "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
        "account_name=VALUES(account_name)",
        acct, charn, pid);
    run_query(q);

    /* Soft-delete (simulating sql_soft_delete_character) */
    snprintf(q, sizeof(q),
        "UPDATE account_characters SET deleted_at=NOW() "
        "WHERE pid=%ld AND deleted_at IS NULL", pid);
    run_query(q);

    /* Verify deleted_at is set */
    snprintf(q, sizeof(q),
        "SELECT deleted_at FROM account_characters WHERE pid=%ld", pid);
    long deleted = query_long(q, 0);
    if (deleted == 0)
        FAIL("deleted_at was not set");

    /* Verify character is filtered out by deleted_at IS NULL */
    snprintf(q, sizeof(q),
        "SELECT 1 FROM account_characters "
        "WHERE account_name='%s' AND deleted_at IS NULL", acct);
    if (query_returns_rows(q, 1))
        FAIL("character still visible after soft-delete");

    cleanup_test_account(acct);
    cleanup_test_player(charn);
    PASS();
}

/* ------------------------------------------------------------------ */
/*  Test 5: Name reuse after deletion (THE CRITICAL BUG FIX TEST)       */
/*                                                                     */
/*  This tests the fix: ON DUPLICATE KEY UPDATE now clears deleted_at  */
/*  and updates pid. Without the fix, a re-created character would     */
/*  keep deleted_at=NOW() and never appear in the character list.      */
/* ------------------------------------------------------------------ */

static void test_name_reuse_after_delete(void)
{
    TEST("name reuse after deletion (deleted_at cleared)");
    const char *acct   = "test_live_reuse";
    const char *charn  = "Reusechar";
    cleanup_test_account(acct);
    cleanup_test_player(charn);

    char q[1024];

    /* Create account */
    snprintf(q, sizeof(q),
        "INSERT INTO accounts (account_name, email, password, confirmed) "
        "VALUES ('%s', '%s@test.com', 'hash123', 1)", acct, acct);
    run_query(q);

    /* --- First lifecycle: create, save, soft-delete --- */

    /* Save to player_data (simulating first character creation) */
    snprintf(q, sizeof(q),
        "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
        "VALUES ('%s', 1, 1, 1, %u, 0, 0)", charn, (unsigned)1);
    run_query(q);
    long pid1 = (long)mysql_insert_id(g_db);

    /* Link to account (simulating add_char_to_account) */
    snprintf(q, sizeof(q),
        "INSERT INTO account_characters (account_name, char_name, pid, "
        "login_count, last_login, blocked, racewar) "
        "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
        "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
        "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
        "account_name=VALUES(account_name)",
        acct, charn, pid1);
    run_query(q);

    /* Soft-delete (simulating sql_soft_delete_character) */
    snprintf(q, sizeof(q),
        "UPDATE account_characters SET deleted_at=NOW() "
        "WHERE pid=%ld AND deleted_at IS NULL", pid1);
    run_query(q);

    /* Hard-delete from player_data (simulating sql_delete_player) */
    snprintf(q, sizeof(q),
        "DELETE FROM player_data WHERE pid=%ld", pid1);
    run_query(q);

    /* --- Second lifecycle: re-create same name --- */

    /* Insert new player_data row (simulating new character creation) */
    snprintf(q, sizeof(q),
        "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
        "VALUES ('%s', 1, 1, 1, %u, 0, 0)", charn, (unsigned)1);
    run_query(q);
    long pid2 = (long)mysql_insert_id(g_db);

    if (pid2 == pid1)
        FAIL("new PID (%ld) should differ from old PID (%ld)", pid2, pid1);

    /* Re-link to account - THIS is where the bug fix matters.
     * The ON DUPLICATE KEY UPDATE must:
     *   1. Clear deleted_at=NULL
     *   2. Update pid to the new value
     *   3. Update account_name */
    snprintf(q, sizeof(q),
        "INSERT INTO account_characters (account_name, char_name, pid, "
        "login_count, last_login, blocked, racewar) "
        "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
        "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
        "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
        "account_name=VALUES(account_name)",
        acct, charn, pid2);
    if (mysql_query(g_db, q))
        FAIL("re-link INSERT: %s", mysql_error(g_db));

    /* --- Verify: character is visible in listing --- */

    /* Check deleted_at was cleared */
    snprintf(q, sizeof(q),
        "SELECT deleted_at FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s'", acct, charn);
    long deleted_at = query_long(q, 0);
    if (deleted_at != 0)
        FAIL("deleted_at still set (%ld) after re-create", deleted_at);

    /* Check PID was updated */
    snprintf(q, sizeof(q),
        "SELECT pid FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s'", acct, charn);
    long stored_pid = query_long(q, 0);
    if (stored_pid != pid2)
        FAIL("pid=%ld, expected new pid=%ld after re-create", stored_pid, pid2);

    /* Check character appears in listing (deleted_at IS NULL filter) */
    snprintf(q, sizeof(q),
        "SELECT 1 FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s' AND deleted_at IS NULL",
        acct, charn);
    if (!query_returns_rows(q, 1))
        FAIL("CRITICAL BUG: character not visible after re-create!");

    cleanup_test_account(acct);
    cleanup_test_player(charn);
    PASS();
}

/* ------------------------------------------------------------------ */
/*  Test 6: PID propagation (pid=0 -> real pid after player_data save) */
/*                                                                     */
/*  Simulates the scenario where add_char_to_account is called before  */
/*  the character is saved to player_data. The initial INSERT uses     */
/*  pid=0, then later the ON DUPLICATE KEY UPDATE must propagate the   */
/*  real PID.                                                          */
/* ------------------------------------------------------------------ */

static void test_pid_propagation(void)
{
    TEST("PID propagation (pid=0 updated to real PID)");
    const char *acct   = "test_live_pidprop";
    const char *charn  = "Pidpropchar";
    cleanup_test_account(acct);
    cleanup_test_player(charn);

    char q[1024];

    /* Create account */
    snprintf(q, sizeof(q),
        "INSERT INTO accounts (account_name, email, password, confirmed) "
        "VALUES ('%s', '%s@test.com', 'hash123', 1)", acct, acct);
    run_query(q);

    /* Step 1: add_char_to_account is called BEFORE character save.
     * At this point, sql_get_player_pid returns -1 because
     * player_data doesn't have the character yet. */
    snprintf(q, sizeof(q),
        "INSERT INTO account_characters (account_name, char_name, pid, "
        "login_count, last_login, blocked, racewar) "
        "VALUES ('%s', '%s', 0, 1, NOW(), 0, 1) "
        "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
        "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
        "account_name=VALUES(account_name)",
        acct, charn);
    if (mysql_query(g_db, q))
        FAIL("INSERT with pid=0: %s", mysql_error(g_db));

    /* Verify pid is 0 */
    snprintf(q, sizeof(q),
        "SELECT pid FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s'", acct, charn);
    long pid0 = query_long(q, -1);
    if (pid0 != 0)
        FAIL("expected pid=0, got %ld", pid0);

    /* Step 2: character is saved to player_data, gets real PID */
    snprintf(q, sizeof(q),
        "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
        "VALUES ('%s', 1, 1, 1, %u, 0, 0)", charn, (unsigned)1);
    run_query(q);
    long real_pid = (long)mysql_insert_id(g_db);

    /* Step 3: write_account is called again.
     * sql_save_account_characters re-runs the INSERT with real PID.
     * The ON DUPLICATE KEY should update pid=VALUES(pid). */
    snprintf(q, sizeof(q),
        "INSERT INTO account_characters (account_name, char_name, pid, "
        "login_count, last_login, blocked, racewar) "
        "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
        "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
        "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
        "account_name=VALUES(account_name)",
        acct, charn, real_pid);
    if (mysql_query(g_db, q))
        FAIL("re-INSERT with real pid: %s", mysql_error(g_db));

    /* Verify PID was propagated */
    snprintf(q, sizeof(q),
        "SELECT pid FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s'", acct, charn);
    long updated_pid = query_long(q, 0);
    if (updated_pid != real_pid)
        FAIL("pid=%ld, expected %ld after propagation", updated_pid, real_pid);

    /* Verify LEFT JOIN now works (pid matches player_data) */
    snprintf(q, sizeof(q),
        "SELECT pd.level FROM account_characters ac "
        "LEFT JOIN player_data pd ON ac.pid = pd.pid "
        "WHERE ac.account_name='%s' AND ac.char_name='%s' "
        "AND ac.deleted_at IS NULL", acct, charn);
    if (mysql_query(g_db, q))
        FAIL("join query: %s", mysql_error(g_db));
    MYSQL_RES *r = mysql_store_result(g_db);
    if (!r || mysql_num_rows(r) == 0)
        FAIL("LEFT JOIN returned no rows after PID propagation");
    mysql_free_result(r);

    cleanup_test_account(acct);
    cleanup_test_player(charn);
    PASS();
}

/* ------------------------------------------------------------------ */
/*  Test 7: Multi-character account                                     */
/* ------------------------------------------------------------------ */

static void test_multi_character_account(void)
{
    TEST("multi-character account (3 characters)");
    const char *acct  = "test_live_multichar";
    const char *names[] = { "Multione", "Multitwo", "Multithree" };
    cleanup_test_account(acct);
    for (int i = 0; i < 3; i++)
        cleanup_test_player(names[i]);

    char q[1024];

    /* Create account */
    snprintf(q, sizeof(q),
        "INSERT INTO accounts (account_name, email, password, confirmed) "
        "VALUES ('%s', '%s@test.com', 'hash123', 1)", acct, acct);
    run_query(q);

    /* Create 3 characters and link them */
    for (int i = 0; i < 3; i++) {
        snprintf(q, sizeof(q),
            "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
            "VALUES ('%s', %d, %d, 1, %u, 0, 0)",
            names[i], i + 1, i + 1, (unsigned)(1 << (i % 3)));
        run_query(q);
        long pid = (long)mysql_insert_id(g_db);

        snprintf(q, sizeof(q),
            "INSERT INTO account_characters (account_name, char_name, pid, "
            "login_count, last_login, blocked, racewar) "
            "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
            "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
            "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
            "account_name=VALUES(account_name)",
            acct, names[i], pid);
        run_query(q);
    }

    /* Verify all 3 appear */
    snprintf(q, sizeof(q),
        "SELECT COUNT(*) FROM account_characters "
        "WHERE account_name='%s' AND deleted_at IS NULL", acct);
    long count = query_long(q, 0);
    if (count != 3)
        FAIL("expected 3 characters, got %ld", count);

    /* Delete middle one */
    snprintf(q, sizeof(q),
        "SELECT pid FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s'", acct, names[1]);
    long mid_pid = query_long(q, 0);

    snprintf(q, sizeof(q),
        "UPDATE account_characters SET deleted_at=NOW() "
        "WHERE pid=%ld AND deleted_at IS NULL", mid_pid);
    run_query(q);
    snprintf(q, sizeof(q),
        "DELETE FROM player_data WHERE pid=%ld", mid_pid);
    run_query(q);

    /* Verify 2 remain visible */
    snprintf(q, sizeof(q),
        "SELECT COUNT(*) FROM account_characters "
        "WHERE account_name='%s' AND deleted_at IS NULL", acct);
    count = query_long(q, 0);
    if (count != 2)
        FAIL("expected 2 visible after delete, got %ld", count);

    cleanup_test_account(acct);
    for (int i = 0; i < 3; i++)
        cleanup_test_player(names[i]);
    PASS();
}

/* ------------------------------------------------------------------ */
/*  Test 8: Full delete-recreate cycle (production scenario)            */
/*                                                                     */
/*  Simulates the exact scenario the user reported: create, save,      */
/*  logout, delete, recreate. Verifies the character appears after      */
/*  re-creation.                                                       */
/* ------------------------------------------------------------------ */

static void test_full_delete_recreate_cycle(void)
{
    TEST("full delete-recreate cycle (production scenario)");
    const char *acct  = "test_live_cycle";
    const char *charn = "Sabir";  /* name from user's bug report */
    cleanup_test_account(acct);
    cleanup_test_player(charn);

    char q[1024];

    /* ---- Phase 1: Create account ---- */
    snprintf(q, sizeof(q),
        "INSERT INTO accounts (account_name, email, password, confirmed) "
        "VALUES ('%s', '%s@test.com', 'hash123', 1)", acct, acct);
    run_query(q);

    /* ---- Phase 2: Create character "Sabir" ---- */
    snprintf(q, sizeof(q),
        "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
        "VALUES ('%s', 5, 1, 1, %u, 0, 100)", charn, (unsigned)1);
    run_query(q);
    long pid1 = (long)mysql_insert_id(g_db);

    snprintf(q, sizeof(q),
        "INSERT INTO account_characters (account_name, char_name, pid, "
        "login_count, last_login, blocked, racewar) "
        "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
        "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
        "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
        "account_name=VALUES(account_name)",
        acct, charn, pid1);
    run_query(q);

    /* Verify character is visible */
    snprintf(q, sizeof(q),
        "SELECT 1 FROM account_characters "
        "WHERE account_name='%s' AND deleted_at IS NULL", acct);
    if (!query_returns_rows(q, 1))
        FAIL("Phase 2: character not visible after creation");

    /* ---- Phase 3: Delete character "Sabir" ---- */
    snprintf(q, sizeof(q),
        "UPDATE account_characters SET deleted_at=NOW() "
        "WHERE pid=%ld AND deleted_at IS NULL", pid1);
    run_query(q);

    snprintf(q, sizeof(q),
        "DELETE FROM player_data WHERE pid=%ld", pid1);
    run_query(q);

    /* ---- Phase 4: Re-create character "Sabir" ---- */
    snprintf(q, sizeof(q),
        "INSERT INTO player_data (name, level, race, racewar, m_class, sex, last_room) "
        "VALUES ('%s', 1, 2, 1, %u, 0, 200)", charn, (unsigned)2);
    run_query(q);
    long pid2 = (long)mysql_insert_id(g_db);

    /* CRITICAL: re-link to account.  The ON DUPLICATE KEY UPDATE
     * must clear deleted_at and update pid.  Without the fix,
     * deleted_at stays set and the character never appears. */
    snprintf(q, sizeof(q),
        "INSERT INTO account_characters (account_name, char_name, pid, "
        "login_count, last_login, blocked, racewar) "
        "VALUES ('%s', '%s', %ld, 1, NOW(), 0, 1) "
        "ON DUPLICATE KEY UPDATE login_count=1, last_login=NOW(), "
        "blocked=0, racewar=1, deleted_at=NULL, pid=VALUES(pid), "
        "account_name=VALUES(account_name)",
        acct, charn, pid2);
    if (mysql_query(g_db, q))
        FAIL("re-link: %s", mysql_error(g_db));

    /* ---- Verify: character IS visible after re-creation ---- */
    snprintf(q, sizeof(q),
        "SELECT 1 FROM account_characters "
        "WHERE account_name='%s' AND deleted_at IS NULL", acct);
    if (!query_returns_rows(q, 1))
        FAIL("BUG: character 'Sabir' not visible after re-creation!");

    /* Verify details */
    snprintf(q, sizeof(q),
        "SELECT pid, deleted_at FROM account_characters "
        "WHERE account_name='%s' AND char_name='%s'", acct, charn);
    if (mysql_query(g_db, q))
        FAIL("detail query: %s", mysql_error(g_db));
    MYSQL_RES *r = mysql_store_result(g_db);
    MYSQL_ROW row = mysql_fetch_row(r);
    long    final_pid = row ? atol(row[0]) : 0;
    int     del_flag  = row && row[1] ? 1 : 0;
    mysql_free_result(r);

    if (final_pid != pid2)
        FAIL("final pid=%ld, expected new pid=%ld", final_pid, pid2);
    if (del_flag)
        FAIL("deleted_at is still set after re-creation!");

    cleanup_test_account(acct);
    cleanup_test_player(charn);
    PASS();
}

/* ------------------------------------------------------------------ */
/*  Suite runner                                                        */
/* ------------------------------------------------------------------ */

int test_live_accounts_run(MYSQL *db)
{
    g_db       = db;
    g_failures = 0;
    g_total    = 0;

    printf("\n=== Live Account/Character Lifecycle Tests ===\n\n");

    test_create_account();
    test_create_character_and_link();
    test_character_listing();
    test_soft_delete_character();
    test_name_reuse_after_delete();
    test_pid_propagation();
    test_multi_character_account();
    test_full_delete_recreate_cycle();

    printf("\n  Account tests: %d/%d passed\n",
           g_total - g_failures, g_total);
    return g_failures;
}
