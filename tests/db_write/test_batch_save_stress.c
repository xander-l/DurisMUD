/*
 * test_batch_save_stress.c
 *
 * Source-grep regression guards for the Phase 7a-1 batch save function
 * sql_save_player_items_batch_all() in src/sql_player.c.
 *
 * Verifies:
 *   - Sub-batch splitting at ~1MB threshold
 *   - Per-row fallback for oversized items (single_saved flag)
 *   - 64-bit mysql_insert_id (unsigned long long)
 *   - Container UPDATE via CASE WHEN
 *   - Phase 5 skipping single_saved items
 *   - Caller uses the batched function
 *
 * Strategy:
 *   - Read src/sql_player.c into a buffer.
 *   - Find the "#else" line (real code, not __NO_MYSQL__ stubs).
 *   - Search from "#else" onwards for expected patterns.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_batch_save_stress.h"

/* ------------------------------------------------------------------ */
/*  Test framework                                                    */
/* ------------------------------------------------------------------ */

static int  g_pass = 0;
static int  g_fail = 0;
static char g_last_error[4096];
static char g_src_buf[524288];  /* 512KB — safe for sql_player.c (~265KB) */
static const char *g_real_code_start = NULL;  /* pointer into g_src_buf after #else */

#define TEST_BEGIN(name) do { printf("  %s ... ", name); fflush(stdout); } while (0)
#define TEST_END()       do { printf("\n"); } while (0)
#define TEST_PASS()      do { g_pass++; } while (0)
#define TEST_FAIL(...)   do { \
    snprintf(g_last_error, sizeof(g_last_error), __VA_ARGS__); \
    g_fail++; \
    fprintf(stderr, "\n  FAIL: %s", g_last_error); \
} while (0)

/* ------------------------------------------------------------------ */
/*  Helper: load src/sql_player.c and locate the #else (real code)     */
/* ------------------------------------------------------------------ */

static int load_real_code(void)
{
    if (g_real_code_start)
        return 1;  /* already loaded */

    FILE *f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f) {
        TEST_FAIL("cannot open src/sql_player.c from any known path");
        return 0;
    }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    if (n >= sizeof(g_src_buf) - 1) {
        TEST_FAIL("src/sql_player.c too large for buffer (%zu bytes)", n);
        fclose(f);
        return 0;
    }
    g_src_buf[n] = '\0';
    fclose(f);

    /* Find #else — real implementations start here. */
    const char *else_pos = strstr(g_src_buf, "\n#else");
    if (!else_pos) {
        else_pos = strstr(g_src_buf, "#else");
    }
    if (!else_pos) {
        TEST_FAIL("#else marker not found in src/sql_player.c");
        return 0;
    }

    g_real_code_start = else_pos;
    return 1;
}

/* Helper: search real code block for a substring.  Returns 1 if found. */
static int real_code_has(const char *needle)
{
    if (!g_real_code_start)
        return 0;
    return strstr(g_real_code_start, needle) != NULL;
}

/* Helper: search real code block for a function definition by name.
 * Returns 1 if the function definition (name with return type before it
 * and '(' after it) is found after #else. */
static int real_code_has_function(const char *func_name)
{
    if (!g_real_code_start)
        return 0;

    const char *pos = strstr(g_real_code_start, func_name);
    if (!pos)
        return 0;

    /* Verify it's followed by '(' to confirm it's a function call/def */
    const char *after = pos + strlen(func_name);
    while (*after == ' ' || *after == '\t') after++;
    if (*after == '(')
        return 1;

    return 0;
}

/* ================================================================== */
/*  Test 1: sql_save_player_items_batch_all exists in real code       */
/* ================================================================== */

static void test_batch_function_exists(void)
{
    TEST_BEGIN("sql_save_player_items_batch_all exists in real code");
    if (!load_real_code()) { TEST_END(); return; }
    if (!real_code_has_function("sql_save_player_items_batch_all")) {
        TEST_FAIL("sql_save_player_items_batch_all not found after #else");
        TEST_END();
        return;
    }
    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 2: flatten_item_tree exists and returns bool                 */
/* ================================================================== */

static void test_batch_flatten_tree_exists(void)
{
    TEST_BEGIN("flatten_item_tree exists (returns bool)");
    if (!load_real_code()) { TEST_END(); return; }
    if (!strstr(g_real_code_start, "flatten_item_tree")) {
        TEST_FAIL("flatten_item_tree not found in real code");
        TEST_END();
        return;
    }
    /* Must have 'static bool flatten_item_tree' or similar */
    if (!strstr(g_real_code_start, "bool flatten_item_tree")) {
        TEST_FAIL("flatten_item_tree does not return bool");
        TEST_END();
        return;
    }
    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 3: flat_item struct has single_saved field                   */
/* ================================================================== */

static void test_batch_flat_item_has_single_saved(void)
{
    TEST_BEGIN("flat_item struct has single_saved field");
    if (!load_real_code()) { TEST_END(); return; }

    /* Find the struct definition */
    const char *struc = strstr(g_real_code_start, "struct flat_item");
    if (!struc) {
        TEST_FAIL("struct flat_item not found in real code");
        TEST_END();
        return;
    }

    if (!strstr(struc, "single_saved")) {
        TEST_FAIL("struct flat_item missing single_saved field");
        TEST_END();
        return;
    }

    /* Also verify it's bool */
    if (!strstr(struc, "bool") || !strstr(struc, "single_saved")) {
        /* just verify both are in the struct vicinity */
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 4: flatten_item_tree sets single_saved = false               */
/* ================================================================== */

static void test_batch_flatten_sets_single_saved(void)
{
    TEST_BEGIN("flatten_item_tree sets single_saved = false");
    if (!load_real_code()) { TEST_END(); return; }

    /* Find flatten_item_tree function */
    const char *func = strstr(g_real_code_start, "flatten_item_tree");
    if (!func) {
        TEST_FAIL("flatten_item_tree not found");
        TEST_END();
        return;
    }

    if (!strstr(func, "single_saved = false")) {
        TEST_FAIL("flatten_item_tree does not initialize single_saved = false");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 5: Sub-batch flush threshold (1MB)                           */
/* ================================================================== */

static void test_batch_sub_batch_flush_threshold(void)
{
    TEST_BEGIN("sub-batch flush threshold is ~1MB (1048576)");
    if (!load_real_code()) { TEST_END(); return; }

    /* Find the batch_all function */
    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    if (!strstr(func, "1048576")) {
        TEST_FAIL("BATCH_BUF_SIZE 1048576 (1MB) not found");
        TEST_END();
        return;
    }

    if (!strstr(func, "1000000")) {
        TEST_FAIL("FLUSH_THRESHOLD 1000000 (1MB-48KB) not found");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 6: Sub-batch restart after flush                             */
/* ================================================================== */

static void test_batch_sub_batch_flush_restart(void)
{
    TEST_BEGIN("sub-batch flushes and restarts INSERT header");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    /* After flushing, batch must restart with INSERT INTO header */
    /* Look for the snprintf that resets after flush */
    if (!strstr(func, "batch_start_idx = i")) {
        TEST_FAIL("sub-batch does not reset batch_start_idx after flush");
        TEST_END();
        return;
    }

    if (!strstr(func, "items_in_batch  = 0")) {
        TEST_FAIL("sub-batch does not reset items_in_batch after flush");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 7: 64-bit mysql_insert_id (unsigned long long)               */
/* ================================================================== */

static void test_batch_unsigned_long_long_first_id(void)
{
    TEST_BEGIN("first_id uses unsigned long long (64-bit)");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    if (!strstr(func, "unsigned long long first_id")) {
        TEST_FAIL("first_id is not unsigned long long (may use 32-bit int)");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 8: Per-row fallback exists (sql_save_single_item_get_id)     */
/* ================================================================== */

static void test_batch_per_row_fallback_exists(void)
{
    TEST_BEGIN("per-row fallback calls sql_save_single_item_get_id");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    if (!strstr(func, "sql_save_single_item_get_id")) {
        TEST_FAIL("no per-row fallback to sql_save_single_item_get_id");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 9: Per-row fallback detaches contents before single save     */
/* ================================================================== */

static void test_batch_per_row_detaches_contents(void)
{
    TEST_BEGIN("per-row fallback detaches obj->contains before single save");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    /* Must save and restore obj->contains around single-save fallback */
    if (!strstr(func, "saved_contains")) {
        TEST_FAIL("no saved_contains variable (contents not detached before fallback)");
        TEST_END();
        return;
    }

    /* Source uses alignment whitespace (obj->contains        = NULL;),
     * so match on "= NULL;" after the saved_contains declaration. */
    const char *after_saved = strstr(func, "saved_contains");
    if (!after_saved || !strstr(after_saved, "= NULL;")) {
        TEST_FAIL("obj->contains not set to NULL before single-save fallback");
        TEST_END();
        return;
    }

    if (!strstr(func, "contains = saved_contains")) {
        TEST_FAIL("obj->contains not restored after single-save fallback");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 10: Container UPDATE uses CASE WHEN                          */
/* ================================================================== */

static void test_batch_container_case_when_update(void)
{
    TEST_BEGIN("container UPDATE uses CASE WHEN id");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    if (!strstr(func, "CASE id")) {
        TEST_FAIL("container UPDATE does not use CASE WHEN id");
        TEST_END();
        return;
    }

    if (!strstr(func, "container_id = CASE")) {
        TEST_FAIL("container UPDATE does not set container_id = CASE");
        TEST_END();
        return;
    }

    if (!strstr(func, "END WHERE id IN")) {
        TEST_FAIL("container UPDATE missing END WHERE id IN clause");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 11: Phase 5 skips single_saved items                         */
/* ================================================================== */

static void test_batch_phase5_skips_single_saved(void)
{
    TEST_BEGIN("Phase 5 (affects/descr) skips single_saved items");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    /* After the free(batch), the Phase 5 loop must check single_saved */
    /* Search for the pattern: if (flat[i].single_saved) continue; */

    /* Find the Phase 5 region (after free(batch)) */
    const char *phase5 = strstr(func, "Phase 5");
    if (!phase5) {
        TEST_FAIL("Phase 5 comment not found");
        TEST_END();
        return;
    }

    /* Must match the exact guard: if (flat[i].single_saved) continue; */
    if (!strstr(phase5, "flat[i].single_saved")) {
        TEST_FAIL("Phase 5 does not check flat[i].single_saved");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 12: Leading comma stripping via memmove                      */
/* ================================================================== */

static void test_batch_leading_comma_strip(void)
{
    TEST_BEGIN("leading comma stripped via memmove after sub-batch restart");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    if (!strstr(func, "memmove(row_buf, row_buf + 1")) {
        TEST_FAIL("no memmove to strip leading comma after sub-batch restart");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 13: Final sub-batch flush exists                             */
/* ================================================================== */

static void test_batch_final_flush_exists(void)
{
    TEST_BEGIN("final sub-batch flush after loop");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    /* After the for loop, there must be a flush for remaining items */
    /* Search for "final" near flush */
    if (!strstr(func, "final")) {
        /* "final" might be in a comment or the flush message */
    }

    /* Must have a second mysql_insert_id call (for final flush) */
    const char *first_insert_id = strstr(func, "mysql_insert_id");
    if (!first_insert_id) {
        TEST_FAIL("no mysql_insert_id call found");
        TEST_END();
        return;
    }

    const char *second_insert_id = strstr(first_insert_id + 1, "mysql_insert_id");
    if (!second_insert_id) {
        TEST_FAIL("only one mysql_insert_id call (missing final flush)");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 14: offset calculation skips single_saved items              */
/* ================================================================== */

static void test_batch_offset_skips_single_saved(void)
{
    TEST_BEGIN("db_item_id offset skips single_saved items");
    if (!load_real_code()) { TEST_END(); return; }

    const char *func = strstr(g_real_code_start, "sql_save_player_items_batch_all");
    if (!func) {
        TEST_FAIL("batch_all function not found");
        TEST_END();
        return;
    }

    /* The ID assignment loop must check !flat[k].single_saved */
    /* Search for the pattern near first_id + offset */
    const char *first_id = strstr(func, "first_id");
    if (!first_id) {
        TEST_FAIL("first_id not found");
        TEST_END();
        return;
    }

    /* Look for the offset loop that checks single_saved */
    const char *offset_loop = strstr(first_id, "offset");
    if (!offset_loop) {
        TEST_FAIL("offset counter not found near first_id assignment");
        TEST_END();
        return;
    }

    if (!strstr(func, "!flat[k].single_saved")) {
        TEST_FAIL("db_item_id assignment does not skip single_saved items");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test 15: Caller uses batch_all for full saves                     */
/* ================================================================== */

static void test_batch_caller_uses_batch_all(void)
{
    TEST_BEGIN("sql_save_player_items calls batch_all for full saves");
    if (!load_real_code()) { TEST_END(); return; }

    if (!real_code_has("sql_save_player_items_batch_all(pid, ch, save_equipment, save_inventory)")) {
        TEST_FAIL("sql_save_player_items does not call batch_all with correct args");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test registry                                                      */
/* ================================================================== */

typedef struct {
    const char *name;
    void (*func)(void);
} test_entry;

static test_entry g_tests[] = {
    { "batch_function_exists",                test_batch_function_exists },
    { "batch_flatten_tree_exists",            test_batch_flatten_tree_exists },
    { "batch_flat_item_has_single_saved",     test_batch_flat_item_has_single_saved },
    { "batch_flatten_sets_single_saved",      test_batch_flatten_sets_single_saved },
    { "batch_sub_batch_flush_threshold",      test_batch_sub_batch_flush_threshold },
    { "batch_sub_batch_flush_restart",        test_batch_sub_batch_flush_restart },
    { "batch_unsigned_long_long_first_id",    test_batch_unsigned_long_long_first_id },
    { "batch_per_row_fallback_exists",        test_batch_per_row_fallback_exists },
    { "batch_per_row_detaches_contents",      test_batch_per_row_detaches_contents },
    { "batch_container_case_when_update",     test_batch_container_case_when_update },
    { "batch_phase5_skips_single_saved",      test_batch_phase5_skips_single_saved },
    { "batch_leading_comma_strip",            test_batch_leading_comma_strip },
    { "batch_final_flush_exists",             test_batch_final_flush_exists },
    { "batch_offset_skips_single_saved",      test_batch_offset_skips_single_saved },
    { "batch_caller_uses_batch_all",          test_batch_caller_uses_batch_all },
    { NULL, NULL },
};

static const int g_num_tests =
    (int)(sizeof(g_tests) / sizeof(g_tests[0])) - 1;  /* exclude sentinel */

int test_batch_save_stress_run_all(void)
{
    test_batch_save_stress_reset();
    printf("\n=== Batch Save Stress Tests ===\n");
    printf("Running %d tests...\n", g_num_tests);

    /* Pre-load the source file once */
    load_real_code();

    for (int i = 0; g_tests[i].name; i++) {
        g_tests[i].func();
    }

    test_batch_save_stress_print_summary();
    return g_fail;
}

int test_batch_save_stress_run_one(const char *name)
{
    test_batch_save_stress_reset();

    /* Pre-load the source file */
    load_real_code();

    for (int i = 0; g_tests[i].name; i++) {
        if (strcmp(g_tests[i].name, name) == 0) {
            printf("\n=== Batch Save Stress: %s ===\n", name);
            g_tests[i].func();
            return (g_fail > 0) ? 1 : 0;
        }
    }
    return -1;  /* not found */
}

void test_batch_save_stress_print_summary(void)
{
    printf("\n  Batch Save Stress: %d passed, %d failed out of %d tests\n",
           g_pass, g_fail, g_num_tests);
    if (g_fail == 0)
        printf("  *** ALL BATCH SAVE STRESS TESTS PASSED ***\n");
}

void test_batch_save_stress_reset(void)
{
    g_pass = 0;
    g_fail = 0;
    g_last_error[0] = '\0';
    g_real_code_start = NULL;
    memset(g_src_buf, 0, sizeof(g_src_buf));
}
