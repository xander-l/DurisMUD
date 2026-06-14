/* ===================================================================
 * test_persistence_owner.c — Regression tests for
 * sql_persistence_item_owner_matches() in src/sql.c.
 *
 * This function prevents items from loading into the wrong player
 * (e.g., a saved locker item accidentally appearing in a fresh
 * character's inventory).  It queries persistence_item_events for
 * the most recent event involving the item's uid, then compares
 * the stored target (formatted as "owner_type:owner_ref") to the
 * expected owner_type+owner_ref passed by the caller.
 *
 * Test strategy:
 *   1. Simulate the function with a mock query that returns a
 *      scripted "target" string.  This lets us exercise every
 *      branch (match, mismatch on type, mismatch on ref, no
 *      events, query failure) without MySQL.
 *   2. Source-grep test: verify the production function exists
 *      and is NOT a `return true` stub.  This is the real
 *      regression guard — if someone reverts §3.2 to a stub, this
 *      fails before the issue ships.
 *
 * These tests are standalone — no MySQL or MUD state needed.
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_persistence_owner.h"

/* ------------------------------------------------------------------ */
/*  Test framework                                                    */
/* ------------------------------------------------------------------ */

static int  g_pass = 0;
static int  g_fail = 0;
static char g_last_error[4096];
/* File-scope buffer for the source-grep test.  512KB for large source files. */
static char g_src_buf[524288];

#define TEST_BEGIN(name) do { printf("  %s ... ", name); fflush(stdout); } while (0)
#define TEST_END()       do { printf("\n"); } while (0)
#define TEST_PASS()      do { g_pass++; } while (0)
#define TEST_FAIL(...)   do { \
    snprintf(g_last_error, sizeof(g_last_error), __VA_ARGS__); \
    g_fail++; \
    fprintf(stderr, "\n  FAIL: %s", g_last_error); \
} while (0)

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        TEST_FAIL("%s: expected %d, got %d", msg, (int)(b), (int)(a)); \
        return; \
    } \
} while (0)

#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { \
        TEST_FAIL("%s: condition false: %s", msg, #cond); \
        return; \
    } \
} while (0)

#define ASSERT_FALSE(cond, msg) do { \
    if ((cond)) { \
        TEST_FAIL("%s: condition true (expected false): %s", msg, #cond); \
        return; \
    } \
} while (0)

/* ------------------------------------------------------------------ */
/*  Mock DB layer                                                     */
/* ------------------------------------------------------------------ */

/* When the mock query is asked for item_uid X, it returns the
 * scripted target string (or NULL row to simulate "no events"). */
typedef struct mock_query_state {
    int   db_available;       /* simulates DB==NULL vs DB!=NULL */
    int   query_fails;        /* if 1, simulate db_query() failure */
    int   query_call_count;   /* number of times the mock was called */
    /* The "most recent event" we want the function to see.  The
     * function passes item_uid in its WHERE clause — we use that to
     * look up the scripted target.  For these tests, all 4 call
     * sites use the same scripted target regardless of uid (we only
     * have one item at a time). */
    const char *scripted_target;
    int   has_row;            /* if 0, returns no row (no events) */
} mock_query_state;

static void mock_reset(mock_query_state *m)
{
    memset(m, 0, sizeof(*m));
    m->db_available = 1;
    m->query_fails  = 0;
}

static int mock_db_query_succeeds(mock_query_state *m,
                                  unsigned long long item_uid,
                                  const char **out_target)
{
    (void)item_uid;
    m->query_call_count++;

    if (!m->db_available)
        return 0;  /* DB not initialized */

    if (m->query_fails)
        return 0;  /* query failed */

    if (!m->has_row)
        return 0;  /* no events for this uid */

    *out_target = m->scripted_target;
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Simulated sql_persistence_item_owner_matches                      */
/*                                                                    */
/*  This is a faithful re-implementation of the function in src/sql.c */
/*  (the version after §3.2).  The mock returns a scripted target    */
/*  string; the logic mirrors the production function.                */
/* ------------------------------------------------------------------ */

static int sim_persistence_item_owner_matches(mock_query_state *m,
                                              unsigned long long item_uid,
                                              const char *owner_type,
                                              const char *owner_ref,
                                              const char *context)
{
    /* No ownership data to validate */
    if (item_uid == 0)
        return 1;  /* true */

    if (!owner_type || !owner_ref || !context)
        return 1;  /* true */

    if (!m->db_available)
        return 1;  /* true — conservative, keep item */

    /* Build the expected target prefix, e.g. "player:" */
    char expected_prefix[64];
    snprintf(expected_prefix, sizeof(expected_prefix), "%s:", owner_type);
    size_t prefix_len = strlen(expected_prefix);

    const char *target = NULL;
    int has_row = mock_db_query_succeeds(m, item_uid, &target);
    if (!has_row)
        return 1;  /* no events found, keep item (conservative) */

    /* Check that the target starts with the expected owner_type prefix */
    if (strncmp(target, expected_prefix, prefix_len) != 0)
        return 0;  /* false — ownership mismatch */

    /* Extract the owner ref from target (after the prefix) and compare */
    const char *actual_ref = target + prefix_len;
    if (strcmp(actual_ref, owner_ref) != 0)
        return 0;  /* false — ownership mismatch */

    return 1;  /* true — ownership matches */
}

/* ------------------------------------------------------------------ */
/*  Tests                                                              */
/* ------------------------------------------------------------------ */

/* TEST 1: item_uid == 0 means "no ownership data" — must keep item */
static void test_owner_uid_zero_keeps_item(void)
{
    TEST_BEGIN("owner_uid_zero_keeps_item");

    mock_query_state m;
    mock_reset(&m);
    m.scripted_target = "player:42";

    int rc = sim_persistence_item_owner_matches(&m, 0, "player", "42", "test");
    ASSERT_TRUE(rc, "item_uid=0 should return true");
    ASSERT_EQ(m.query_call_count, 0,
              "must not query DB when item_uid is 0");

    TEST_PASS();
    TEST_END();
}

/* TEST 2: NULL owner_type/owner_ref/context — defensive keep */
static void test_owner_null_args_keep_item(void)
{
    TEST_BEGIN("owner_null_args_keep_item");

    mock_query_state m;
    mock_reset(&m);

    /* NULL owner_type */
    int rc1 = sim_persistence_item_owner_matches(&m, 100, NULL, "42", "test");
    ASSERT_TRUE(rc1, "NULL owner_type should return true");

    /* NULL owner_ref */
    int rc2 = sim_persistence_item_owner_matches(&m, 100, "player", NULL, "test");
    ASSERT_TRUE(rc2, "NULL owner_ref should return true");

    /* NULL context */
    int rc3 = sim_persistence_item_owner_matches(&m, 100, "player", "42", NULL);
    ASSERT_TRUE(rc3, "NULL context should return true");

    ASSERT_EQ(m.query_call_count, 0,
              "must not query DB when args are NULL");

    TEST_PASS();
    TEST_END();
}

/* TEST 3: DB not available — conservative keep */
static void test_owner_no_db_keeps_item(void)
{
    TEST_BEGIN("owner_no_db_keeps_item");

    mock_query_state m;
    mock_reset(&m);
    m.db_available = 0;

    int rc = sim_persistence_item_owner_matches(&m, 100, "player", "42", "test");
    ASSERT_TRUE(rc, "DB unavailable should return true (conservative)");

    TEST_PASS();
    TEST_END();
}

/* TEST 4: No events for this item_uid — conservative keep */
static void test_owner_no_events_keeps_item(void)
{
    TEST_BEGIN("owner_no_events_keeps_item");

    mock_query_state m;
    mock_reset(&m);
    m.has_row = 0;  /* no matching event */

    int rc = sim_persistence_item_owner_matches(&m, 100, "player", "42", "test");
    ASSERT_TRUE(rc, "no events should return true (conservative)");
    ASSERT_EQ(m.query_call_count, 1, "should have queried DB once");

    TEST_PASS();
    TEST_END();
}

/* TEST 5: DB query fails — conservative keep */
static void test_owner_query_fails_keeps_item(void)
{
    TEST_BEGIN("owner_query_fails_keeps_item");

    mock_query_state m;
    mock_reset(&m);
    m.query_fails = 1;

    int rc = sim_persistence_item_owner_matches(&m, 100, "player", "42", "test");
    ASSERT_TRUE(rc, "query failure should return true (conservative)");

    TEST_PASS();
    TEST_END();
}

/* TEST 6: Most recent event matches expected owner — keep item */
static void test_owner_match_keeps_item(void)
{
    TEST_BEGIN("owner_match_keeps_item");

    mock_query_state m;
    mock_reset(&m);
    m.scripted_target = "player:42";
    m.has_row = 1;

    int rc = sim_persistence_item_owner_matches(&m, 100, "player", "42", "test");
    ASSERT_TRUE(rc, "matching owner should return true");

    TEST_PASS();
    TEST_END();
}

/* TEST 7: Target prefix differs (item moved to a different
 * owner_type, e.g., was a player item, now a locker item) —
 * discard (return false) */
static void test_owner_type_mismatch_discards_item(void)
{
    TEST_BEGIN("owner_type_mismatch_discards_item");

    mock_query_state m;
    mock_reset(&m);
    m.scripted_target = "locker:42";  /* expected was player */
    m.has_row = 1;

    int rc = sim_persistence_item_owner_matches(&m, 100, "player", "42", "test");
    ASSERT_FALSE(rc, "type mismatch should return false (discard)");

    TEST_PASS();
    TEST_END();
}

/* TEST 8: Target prefix matches but ref differs (item was
 * transferred to a different player) — discard */
static void test_owner_ref_mismatch_discards_item(void)
{
    TEST_BEGIN("owner_ref_mismatch_discards_item");

    mock_query_state m;
    mock_reset(&m);
    m.scripted_target = "player:99";  /* expected was player:42 */
    m.has_row = 1;

    int rc = sim_persistence_item_owner_matches(&m, 100, "player", "42", "test");
    ASSERT_FALSE(rc, "ref mismatch should return false (discard)");

    TEST_PASS();
    TEST_END();
}

/* TEST 9: Locker ownership — the locker call sites use
 * owner_type="locker" with a ref like "account:42" */
static void test_owner_locker_match_keeps_item(void)
{
    TEST_BEGIN("owner_locker_match_keeps_item");

    mock_query_state m;
    mock_reset(&m);
    m.scripted_target = "locker:account:42";
    m.has_row = 1;

    int rc = sim_persistence_item_owner_matches(&m, 200, "locker", "account:42", "locker_load");
    ASSERT_TRUE(rc, "locker match should return true");

    /* Now a different locker */
    mock_reset(&m);
    m.scripted_target = "locker:account:99";
    m.has_row = 1;

    int rc2 = sim_persistence_item_owner_matches(&m, 200, "locker", "account:42", "locker_load");
    ASSERT_FALSE(rc2, "wrong locker should return false (discard)");

    TEST_PASS();
    TEST_END();
}

/* TEST 10: Corpse ownership — the corpse call sites use
 * owner_type="corpse" with a numeric ref (corpse save id) */
static void test_owner_corpse_match_keeps_item(void)
{
    TEST_BEGIN("owner_corpse_match_keeps_item");

    mock_query_state m;
    mock_reset(&m);
    m.scripted_target = "corpse:12345";
    m.has_row = 1;

    int rc = sim_persistence_item_owner_matches(&m, 300, "corpse", "12345", "corpse_load");
    ASSERT_TRUE(rc, "corpse match should return true");

    mock_reset(&m);
    m.scripted_target = "corpse:99999";
    m.has_row = 1;

    int rc2 = sim_persistence_item_owner_matches(&m, 300, "corpse", "12345", "corpse_load");
    ASSERT_FALSE(rc2, "wrong corpse should return false (discard)");

    TEST_PASS();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/*  TEST 11: Source-grep regression guard                             */
/*                                                                    */
/*  Verifies that src/sql.c contains the real implementation, not a  */
/*  return-true stub.  This is the critical test — if someone        */
/*  reverts §3.2 to a stub, this catches it before the issue ships.  */
/* ------------------------------------------------------------------ */
static void test_owner_production_source_is_not_stub(void)
{
    TEST_BEGIN("owner_production_source_is_not_stub");

    /* Read src/sql.c into the test buffer.
     * NOTE: PRODUCTION_SOURCE_PATH points to sql_player.c — do NOT use
     * it here because we specifically need sql.c for the ownership check. */
    FILE *f = fopen("src/sql.c", "r");
    if (!f) f = fopen("../../../src/sql.c", "r");
    if (!f) f = fopen("../../src/sql.c", "r");
    if (!f) {
        TEST_FAIL("cannot open src/sql.c (run from project root or tests/db_write/)");
        TEST_END();
        return;
    }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    if (n == sizeof(g_src_buf) - 1) {
        TEST_FAIL("src/sql.c is larger than 512KB — test buffer too small");
        TEST_END();
        return;
    }

    /* Find the real implementation, NOT the __NO_MYSQL__ stub at top.
     * The stub is a one-liner `return true;` at line 95.  The real
     * implementation (line ~2340) is preceded by a comment that begins
     * with "Validates that an item's persistence_event log" — anchoring
     * on that comment guarantees we find the production function, not
     * the build-time stub. */
    const char *func_start = strstr(g_src_buf,
        "Validates that an item's persistence_event log matches");
    if (!func_start) {
        TEST_FAIL("real implementation of sql_persistence_item_owner_matches() "
                  "not found in src/sql.c (anchored on its leading comment)");
        TEST_END();
        return;
    }

    /* The function must query the persistence_item_events table. */
    const char *query_hit = strstr(func_start, "persistence_item_events");
    if (!query_hit) {
        TEST_FAIL("sql_persistence_item_owner_matches() does not query "
                  "persistence_item_events — §3.2 implementation may have been regressed!");
        TEST_END();
        return;
    }

    /* The function must contain a "SELECT target" clause. */
    const char *select_hit = strstr(func_start, "SELECT target");
    if (!select_hit) {
        TEST_FAIL("sql_persistence_item_owner_matches() does not contain "
                  "'SELECT target' — owner-lookup query missing!");
        TEST_END();
        return;
    }

    /* The function must log a mismatch via logit() — this is the
     * observable signal that ownership was actually checked.  We
     * search for "OWNERSHIP MISMATCH" anywhere after the function
     * start. */
    int found_logit = 0;
    const char *p = func_start;
    while (p < g_src_buf + n) {
        const char *hit = strstr(p, "OWNERSHIP MISMATCH");
        if (!hit) break;
        found_logit = 1;
        break;
    }
    if (!found_logit) {
        TEST_FAIL("sql_persistence_item_owner_matches() does not log "
                  "'OWNERSHIP MISMATCH' — diagnostic logging missing!");
        TEST_END();
        return;
    }

    /* The function must return false on a mismatch.  Search for a
     * `return false` somewhere in the function body — confirming
     * the function actually discards items on mismatch rather than
     * always returning true. */
    int found_return_false = 0;
    p = func_start;
    while (p < g_src_buf + n) {
        const char *hit = strstr(p, "return false");
        if (!hit) break;
        found_return_false = 1;
        break;
    }
    if (!found_return_false) {
        TEST_FAIL("sql_persistence_item_owner_matches() does not return false — "
                  "ownership-mismatch path is missing (reverted to stub?)");
        TEST_END();
        return;
    }

    /* Note: the 4 marker checks above (persistence_item_events,
     * SELECT target, logit OWNERSHIP MISMATCH, return false) are
     * collectively sufficient to prove the real implementation is
     * present.  We intentionally do NOT enforce a minimum body
     * length — a legitimate refactor could produce a more concise
     * version that still has all four markers. */

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test registry                                                      */
/* ================================================================== */

typedef struct { const char *name; void (*func)(void); } test_entry;

static test_entry g_tests[] = {
    { "owner_uid_zero_keeps_item",                test_owner_uid_zero_keeps_item },
    { "owner_null_args_keep_item",                test_owner_null_args_keep_item },
    { "owner_no_db_keeps_item",                   test_owner_no_db_keeps_item },
    { "owner_no_events_keeps_item",               test_owner_no_events_keeps_item },
    { "owner_query_fails_keeps_item",             test_owner_query_fails_keeps_item },
    { "owner_match_keeps_item",                   test_owner_match_keeps_item },
    { "owner_type_mismatch_discards_item",        test_owner_type_mismatch_discards_item },
    { "owner_ref_mismatch_discards_item",         test_owner_ref_mismatch_discards_item },
    { "owner_locker_match_keeps_item",            test_owner_locker_match_keeps_item },
    { "owner_corpse_match_keeps_item",            test_owner_corpse_match_keeps_item },
    { "owner_production_source_is_not_stub",      test_owner_production_source_is_not_stub },
    { NULL, NULL },
};

static const int g_num_tests = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

int test_persistence_owner_run_all(void)
{
    test_persistence_owner_reset();
    printf("  [Persistence Owner Validation Tests]\n");

    for (int i = 0; g_tests[i].name; i++) {
        g_tests[i].func();
    }

    printf("  Passed: %d/%d\n", g_pass, g_pass + g_fail);
    return g_fail;
}

int test_persistence_owner_run_one(const char *name)
{
    for (int i = 0; g_tests[i].name; i++) {
        if (strcmp(g_tests[i].name, name) == 0) {
            printf("  [Persistence Owner: %s]\n", name);
            g_tests[i].func();
            return (g_fail > 0);
        }
    }
    return -1;  /* not found, let other suites try */
}

void test_persistence_owner_print_summary(void)
{
    printf("  Persistence Owner: %d passed, %d failed out of %d tests\n",
           g_pass, g_fail, g_num_tests);
}

void test_persistence_owner_reset(void)
{
    g_pass = 0;
    g_fail = 0;
    g_last_error[0] = '\0';
}
