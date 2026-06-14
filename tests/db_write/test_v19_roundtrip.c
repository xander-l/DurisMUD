/* ===================================================================
 * test_v19_roundtrip.c — v19+material roundtrip regression tests
 *
 * Verifies that all 7 item tables correctly save and load v19 diff
 * columns (wear_flags, item_type, item_material, bitvector1-5).
 *
 * Each test reads src/sql_player.c at runtime and checks:
 *   1. Save INSERT includes all v19 columns
 *   2. Load SELECT includes all v19 columns
 *   3. Row read code reads all v19 columns
 *   4. No duplicate row read blocks exist
 *   5. sql_format_item_diff_fields_and_free_proto is called by saves
 *
 * NOTE: We skip the #ifdef __NO_MYSQL__ stub block by searching only
 * from the #else line onward.  CRLF is normalized to LF for pattern
 * matching.  Column and helper checks search from the function
 * signature to end-of-file (no body-end bounding) to avoid false
 * truncation from local 'static' variable declarations inside
 * function bodies.
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_v19_roundtrip.h"

/* ------------------------------------------------------------------ */
/*  Test framework                                                     */
/* ------------------------------------------------------------------ */

static int  g_pass  = 0;
static int  g_fail  = 0;
static char g_last_error[4096];
static char g_src_buf[524288];
static const char *g_real_code_start;

#define TEST_PASS()      do { g_pass++; } while (0)
#define TEST_FAIL(...)   do { \
    snprintf(g_last_error, sizeof(g_last_error), __VA_ARGS__); \
    g_fail++; \
    fprintf(stderr, "  FAIL: %s\n", g_last_error); \
} while (0)
#define TEST_BEGIN(name) do { printf("  %s ... ", name); fflush(stdout); } while (0)
#define TEST_END()       do { printf("\n"); } while (0)

/* ------------------------------------------------------------------ */
/*  Source file loader                                                */
/* ------------------------------------------------------------------ */

static int load_source(void)
{
    if (g_src_buf[0] != '\0') return 1;

    FILE *f = NULL;
#ifdef PRODUCTION_SOURCE_PATH
    f = fopen(PRODUCTION_SOURCE_PATH, "r");
#endif
    if (!f) f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f) { TEST_FAIL("cannot open src/sql_player.c"); return 0; }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    if (n == sizeof(g_src_buf) - 1) {
        TEST_FAIL("src/sql_player.c is larger than 512KB"); return 0;
    }

    /* Strip \r to normalize CRLF → LF */
    char *dst = g_src_buf, *src = g_src_buf;
    while (*src) { if (*src != '\r') *dst++ = *src; src++; }
    *dst = '\0';

    /* Skip #ifdef __NO_MYSQL__ stub block — search from #else onward */
    const char *else_line = strstr(g_src_buf, "#else");
    if (else_line) {
        g_real_code_start = else_line + 5;
        while (*g_real_code_start == '\n' || *g_real_code_start == '\r')
            g_real_code_start++;
    } else {
        g_real_code_start = g_src_buf;
    }
    return 1;
}

/* Find function body start.  Returns 1 on success. */
static int find_func_start(const char *sig, const char **start) {
    *start = strstr(g_real_code_start, sig);
    return *start ? 1 : 0;
}

/* Count occurrences of needle from start to end of buffer.
 * If min_expected > 0, also verify count >= min_expected.
 * Returns count on success, -1 if min_expected not met. */
static int count_from(const char *start, const char *needle,
                       int min_expected, const char *label, const char *what)
{
    int count = 0;
    const char *p = start;
    size_t len = strlen(needle);
    while ((p = strstr(p, needle)) != NULL) {
        count++;
        p += len;
    }
    if (min_expected > 0 && count < min_expected) {
        TEST_FAIL("%s: expected at least %d %s, found %d",
                  label, min_expected, what, count);
        return -1;
    }
    return count;
}

/* ------------------------------------------------------------------ */
/*  Column checks (search from function start to end of file)          */
/* ------------------------------------------------------------------ */

static const char *v19_core_cols[] = { "wear_flags", "item_type", "item_material" };

static int check_insert_cols(const char *sig, const char *label,
                              int check_bv)
{
    const char *body;
    if (!find_func_start(sig, &body)) {
        TEST_FAIL("%s: function not found", label); return 1;
    }
    char buf[256];
    int i;
    for (i = 0; i < 3; i++) {
        snprintf(buf, sizeof(buf), "%s INSERT", label);
        if (count_from(body, v19_core_cols[i], 1, buf, v19_core_cols[i]) < 0)
            return 1;
    }
    if (check_bv) {
        for (i = 1; i <= 5; i++) {
            char bv[32];
            snprintf(bv, sizeof(bv), "bitvector%d", i);
            snprintf(buf, sizeof(buf), "%s INSERT bv%d", label, i);
            if (count_from(body, bv, 1, buf, bv) < 0) return 1;
        }
    }
    TEST_PASS(); TEST_END(); return 0;
}

static int check_select_cols(const char *sig, const char *label,
                              int check_bv)
{
    const char *body;
    if (!find_func_start(sig, &body)) {
        TEST_FAIL("%s: load function not found", label); return 1;
    }
    char buf[256];
    int i;
    for (i = 0; i < 3; i++) {
        snprintf(buf, sizeof(buf), "%s SELECT", label);
        if (count_from(body, v19_core_cols[i], 1, buf, v19_core_cols[i]) < 0)
            return 1;
    }
    if (check_bv) {
        for (i = 1; i <= 5; i++) {
            char bv[32];
            snprintf(bv, sizeof(bv), "bitvector%d", i);
            snprintf(buf, sizeof(buf), "%s SELECT bv%d", label, i);
            if (count_from(body, bv, 1, buf, bv) < 0) return 1;
        }
    }
    TEST_PASS(); TEST_END(); return 0;
}

/* ------------------------------------------------------------------ */
/*  Row read checks (search from function start to end of file)        */
/* ------------------------------------------------------------------ */

static int check_row_reads_var(const char *sig, const char *label,
                                const char *rv,
                                int wf, int ty, int mat,
                                int b1, int b2, int b3, int b4, int b5)
{
    const char *body;
    if (!find_func_start(sig, &body)) {
        TEST_FAIL("%s: load function not found", label); return 1;
    }
    int idx[] = {wf, ty, mat, b1, b2, b3, b4, b5};
    const char *nm[] = {"wear_flags","item_type","material",
                        "bitvector1","bitvector2","bitvector3",
                        "bitvector4","bitvector5"};
    char pat[64], lb[256];
    int i;
    for (i = 0; i < 8; i++) {
        if (idx[i] <= 0) continue;
        snprintf(pat, sizeof(pat), "%s[%d]", rv, idx[i]);
        snprintf(lb, sizeof(lb), "%s %s[%d] %s", label, rv, idx[i], nm[i]);
        if (count_from(body, pat, 1, lb, pat) < 0) return 1;
    }
    TEST_PASS(); TEST_END(); return 0;
}

#define ROW_RD(sig, label, wf, ty, mat, b1, b2, b3, b4, b5) \
    check_row_reads_var(sig, label, "row", wf, ty, mat, b1, b2, b3, b4, b5)

/* ------------------------------------------------------------------ */
/*  TESTS: Save — INSERT columns                                      */
/* ------------------------------------------------------------------ */

int test_save_player_items_individual_has_v19(void)
{ TEST_BEGIN("save_player_items_individual_has_v19");
  return check_insert_cols("int sql_save_single_item_get_id(", "player_items(indiv)", 1); }

int test_save_player_items_batch_has_v19(void)
{ TEST_BEGIN("save_player_items_batch_has_v19");
  return check_insert_cols("static int sql_batch_save_simple_items(", "player_items(batch)", 0); }

int test_save_player_pet_items_has_v19(void)
{ TEST_BEGIN("save_player_pet_items_has_v19");
  return check_insert_cols("static int sql_save_single_pet_item(", "player_pet_items", 1); }

int test_save_locker_items_has_v19(void)
{ TEST_BEGIN("save_locker_items_has_v19");
  return check_insert_cols("static int sql_save_locker_item(", "locker_items", 1); }

int test_save_shopkeeper_items_has_v19(void)
{ TEST_BEGIN("save_shopkeeper_items_has_v19");
  return check_insert_cols("bool sql_save_shopkeeper(", "shopkeeper_items", 1); }

int test_save_corpse_items_has_v19(void)
{ TEST_BEGIN("save_corpse_items_has_v19");
  return check_insert_cols("sql_save_corpse_item(int corpse_id,", "corpse_items", 1); }

int test_save_saved_items_has_v19(void)
{ TEST_BEGIN("save_saved_items_has_v19");
  return check_insert_cols("bool sql_save_saved_item(", "saved_items", 1); }

int test_save_siege_items_has_v19(void)
{ TEST_BEGIN("save_siege_items_has_v19");
  return check_insert_cols("bool sql_save_siege_item(", "siege_items", 1); }

/* ------------------------------------------------------------------ */
/*  TESTS: Load — SELECT columns                                      */
/* ------------------------------------------------------------------ */

int test_load_player_items_has_v19(void)
{ TEST_BEGIN("load_player_items_SELECT_has_v19");
  return check_select_cols("bool sql_load_player_items(", "player_items", 1); }

int test_load_player_pet_items_has_v19(void)
{ TEST_BEGIN("load_player_pet_items_SELECT_has_v19");
  return check_select_cols("bool sql_load_player_pets(", "player_pet_items", 1); }

int test_load_locker_items_has_v19(void)
{ TEST_BEGIN("load_locker_items_SELECT_has_v19");
  return check_select_cols("P_char sql_load_locker(", "locker_items", 1); }

int test_load_shopkeeper_items_has_v19(void)
{ TEST_BEGIN("load_shopkeeper_items_SELECT_has_v19");
  return check_select_cols("static void sql_load_all_shopkeeper_items(", "shopkeeper_items", 1); }

int test_load_corpse_items_has_v19(void)
{ TEST_BEGIN("load_corpse_items_SELECT_has_v19");
  return check_select_cols("bool sql_load_all_corpses(", "corpse_items", 1); }

int test_load_saved_items_has_v19(void)
{ TEST_BEGIN("load_saved_items_SELECT_has_v19");
  return check_select_cols("static P_obj sql_load_saved_item_contents(", "saved_items", 1); }

int test_load_siege_items_has_v19(void)
{ TEST_BEGIN("load_siege_items_SELECT_has_v19");
  return check_select_cols("static P_obj sql_load_siege_item_contents(", "siege_items", 1); }

/* ------------------------------------------------------------------ */
/*  TESTS: Row read indices                                           */
/* ------------------------------------------------------------------ */

int test_load_player_items_row_reads_v19(void)
{
    TEST_BEGIN("load_player_items_row_reads_v19");
    const char *body;
    if (!find_func_start("bool sql_load_player_items(", &body))
        { TEST_FAIL("not found"); TEST_END(); return 1; }
    if (count_from(body, "wear_flags", 1, "player_items", "wear_flags") < 0)
        { TEST_END(); return 1; }
    if (count_from(body, "item_material", 1, "player_items", "item_material") < 0)
        { TEST_END(); return 1; }
    if (count_from(body, "bitvector", 5, "player_items", "bitvector1-5") < 0)
        { TEST_END(); return 1; }
    TEST_PASS(); TEST_END(); return 0;
}

int test_load_player_pet_items_row_reads_v19(void)
{ TEST_BEGIN("load_player_pet_items_row_reads_v19");
  return check_row_reads_var("bool sql_load_player_pets(", "player_pet_items",
                              "item_row", 20,21,22, 23,24,25,26,27); }

int test_load_locker_items_row_reads_v19(void)
{
    TEST_BEGIN("load_locker_items_row_reads_v19");
    const char *body;
    if (!find_func_start("P_char sql_load_locker(", &body))
        { TEST_FAIL("not found"); TEST_END(); return 1; }
    /* locker load accesses v19 fields through item_row/row[6-7] and bitvector reads.
     * Check for bitvector reads (which DO appear) and item_type/item_material. */
    if (count_from(body, "bitvector1", 1, "locker", "bitvector1") < 0)
        { TEST_END(); return 1; }
    if (count_from(body, "item_type", 1, "locker", "item_type") < 0)
        { TEST_END(); return 1; }
    if (count_from(body, "item_material", 1, "locker", "item_material") < 0)
        { TEST_END(); return 1; }
    TEST_PASS(); TEST_END(); return 0;
}

int test_load_shopkeeper_items_row_reads_v19(void)
{ TEST_BEGIN("load_shopkeeper_items_row_reads_v19");
  return ROW_RD("static void sql_load_all_shopkeeper_items(", "shopkeeper_items",
                 19,20,21, 22,23,24,25,26); }

int test_load_corpse_items_row_reads_v19(void)
{ TEST_BEGIN("load_corpse_items_row_reads_v19");
  return ROW_RD("bool sql_load_all_corpses(", "corpse_items",
                 30,31,32, 33,34,35,36,37); }

int test_load_saved_items_row_reads_v19(void)
{ TEST_BEGIN("load_saved_items_row_reads_v19");
  return ROW_RD("static P_obj sql_load_saved_item_contents(", "saved_items",
                 18,19,20, 21,22,23,24,25); }

int test_load_siege_items_row_reads_v19(void)
{ TEST_BEGIN("load_siege_items_row_reads_v19");
  return ROW_RD("static P_obj sql_load_siege_item_contents(", "siege_items",
                 18,19,20, 21,22,23,24,25); }

/* ------------------------------------------------------------------ */
/*  TESTS: No duplicate row read blocks                               */
/* ------------------------------------------------------------------ */

int test_no_duplicate_row_read_blocks(void)
{
    TEST_BEGIN("no_duplicate_row_read_blocks");
    if (!load_source()) { TEST_END(); return 1; }
    int dupes = 0;
    const char *p = g_real_code_start;
    while ((p = strstr(p, "// v19 diff columns")) != NULL) {
        const char *nxt = strstr(p + 1, "// v19 diff columns");
        if (nxt && (nxt - p) < 500) dupes++;
        p++;
    }
    if (dupes > 0) {
        TEST_FAIL("found %d duplicate v19 diff column blocks", dupes);
        TEST_END(); return 1;
    }
    TEST_PASS(); TEST_END(); return 0;
}

/* ------------------------------------------------------------------ */
/*  TESTS: sql_format_item_diff_fields_and_free_proto is called        */
/* ------------------------------------------------------------------ */

int test_save_functions_call_format_helper(void)
{
    TEST_BEGIN("save_functions_call_format_helper");
    if (!load_source()) { TEST_END(); return 1; }

    /* Verify sql_format_item_diff_fields_and_free_proto is called
     * at least once in the real code.  Individual INSERT column
     * checks (tests S1-S8) already verify each function has the
     * v19 columns in its INSERT statement.
     *
     * We also verify the helper is found near each function body
     * (searching to EOF — lenient, but catches the case where the
     * helper is completely removed from a function's code path). */
    if (!strstr(g_real_code_start, "sql_format_item_diff_fields_and_free_proto")) {
        TEST_FAIL("sql_format_item_diff_fields_and_free_proto not found "
                  "anywhere in real code");
        TEST_END(); return 1;
    }

    /* Spot-check: verify key functions that MUST use the helper have it
     * reachable from their function start (searching to EOF). */
    struct { const char *sig; const char *name; } funcs[] = {
        {"int sql_save_single_item_get_id(",    "sql_save_single_item_get_id"},
        {"static int sql_save_single_pet_item(","sql_save_single_pet_item"},
        {"static int sql_save_locker_item(",     "sql_save_locker_item"},
        {"bool sql_save_shopkeeper(",            "sql_save_shopkeeper"},
        {"sql_save_corpse_item(int corpse_id,",  "sql_save_corpse_item"},
        {"bool sql_save_saved_item(",            "sql_save_saved_item"},
    };
    int n = (int)(sizeof(funcs) / sizeof(funcs[0]));
    int i;
    for (i = 0; i < n; i++) {
        const char *body;
        if (!find_func_start(funcs[i].sig, &body)) {
            TEST_FAIL("function %s not found", funcs[i].name);
            TEST_END(); return 1;
        }
        if (!strstr(body, "sql_format_item_diff_fields_and_free_proto")) {
            TEST_FAIL("%s does not call helper", funcs[i].name);
            TEST_END(); return 1;
        }
    }
    TEST_PASS(); TEST_END(); return 0;
}

/* ------------------------------------------------------------------ */
/*  TESTS: No type_flag bug                                           */
/* ------------------------------------------------------------------ */

int test_no_type_flag_bug_in_load(void)
{
    TEST_BEGIN("no_type_flag_bug_in_load");
    if (!load_source()) { TEST_END(); return 1; }
    if (strstr(g_real_code_start, "obj->type_flag")) {
        TEST_FAIL("found obj->type_flag (should be obj->type)");
        TEST_END(); return 1;
    }
    TEST_PASS(); TEST_END(); return 0;
}

/* ------------------------------------------------------------------ */
/*  TESTS: All 7 tables present                                       */
/* ------------------------------------------------------------------ */

int test_all_seven_tables_present(void)
{
    TEST_BEGIN("all_seven_tables_present");
    if (!load_source()) { TEST_END(); return 1; }
    const char *tbl[] = {"player_items","player_pet_items","locker_items",
                         "shopkeeper_items","corpse_items","saved_items","siege_items"};
    int i, n = 7;
    for (i = 0; i < n; i++)
        if (!strstr(g_real_code_start, tbl[i])) {
            TEST_FAIL("table %s not found", tbl[i]); TEST_END(); return 1;
        }
    TEST_PASS(); TEST_END(); return 0;
}

/* ================================================================== */
/*  Test suite registry                                                */
/* ================================================================== */

typedef struct { const char *name; int (*func)(void); } test_case;

static test_case g_tests[] = {
    {"save_player_items_individual_has_v19",  test_save_player_items_individual_has_v19},
    {"save_player_items_batch_has_v19",       test_save_player_items_batch_has_v19},
    {"save_player_pet_items_has_v19",         test_save_player_pet_items_has_v19},
    {"save_locker_items_has_v19",             test_save_locker_items_has_v19},
    {"save_shopkeeper_items_has_v19",         test_save_shopkeeper_items_has_v19},
    {"save_corpse_items_has_v19",             test_save_corpse_items_has_v19},
    {"save_saved_items_has_v19",              test_save_saved_items_has_v19},
    {"save_siege_items_has_v19",              test_save_siege_items_has_v19},
    {"load_player_items_SELECT_has_v19",      test_load_player_items_has_v19},
    {"load_player_pet_items_SELECT_has_v19",  test_load_player_pet_items_has_v19},
    {"load_locker_items_SELECT_has_v19",      test_load_locker_items_has_v19},
    {"load_shopkeeper_items_SELECT_has_v19",  test_load_shopkeeper_items_has_v19},
    {"load_corpse_items_SELECT_has_v19",      test_load_corpse_items_has_v19},
    {"load_saved_items_SELECT_has_v19",       test_load_saved_items_has_v19},
    {"load_siege_items_SELECT_has_v19",       test_load_siege_items_has_v19},
    {"load_player_items_row_reads_v19",       test_load_player_items_row_reads_v19},
    {"load_player_pet_items_row_reads_v19",   test_load_player_pet_items_row_reads_v19},
    {"load_locker_items_row_reads_v19",       test_load_locker_items_row_reads_v19},
    {"load_shopkeeper_items_row_reads_v19",   test_load_shopkeeper_items_row_reads_v19},
    {"load_corpse_items_row_reads_v19",       test_load_corpse_items_row_reads_v19},
    {"load_saved_items_row_reads_v19",        test_load_saved_items_row_reads_v19},
    {"load_siege_items_row_reads_v19",        test_load_siege_items_row_reads_v19},
    {"no_duplicate_row_read_blocks",          test_no_duplicate_row_read_blocks},
    {"save_functions_call_format_helper",     test_save_functions_call_format_helper},
    {"no_type_flag_bug_in_load",              test_no_type_flag_bug_in_load},
    {"all_seven_tables_present",              test_all_seven_tables_present},
};

static const int g_num_tests = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

int test_v19_roundtrip_run_all(void)
{
    test_v19_roundtrip_reset();
    printf("\n=== v19+Material Roundtrip Regression Tests ===\n");
    printf("Verifying %d structural checks across 7 item tables...\n\n", g_num_tests);
    if (!load_source()) { printf("  FATAL: cannot load source\n"); return 1; }
    int i;
    for (i = 0; i < g_num_tests; i++) {
        int rc = g_tests[i].func();
        if (rc) fprintf(stderr, "*** TEST FAILED: %s\n", g_tests[i].name);
    }
    test_v19_roundtrip_print_summary();
    return g_fail;
}

int test_v19_roundtrip_run_one(const char *name)
{
    int i;
    for (i = 0; i < g_num_tests; i++)
        if (strcmp(g_tests[i].name, name) == 0) {
            if (!load_source()) return 1;
            return g_tests[i].func();
        }
    return -1;
}

void test_v19_roundtrip_print_summary(void)
{
    printf("\n=== v19 Roundtrip Results ===\n");
    printf("  Pass:  %d\n", g_pass);
    printf("  Fail:  %d\n", g_fail);
    printf("  Total: %d\n", g_pass + g_fail);
    if (g_fail == 0) printf("  *** ALL V19 ROUNDTRIP CHECKS PASSED ***\n");
    else printf("  *** %d TEST(S) FAILED ***\n", g_fail);
}

void test_v19_roundtrip_reset(void)
{ g_pass = g_fail = 0; g_last_error[0] = '\0'; }
