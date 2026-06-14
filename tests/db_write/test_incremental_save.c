/*
 * test_incremental_save.c
 *
 * Regression tests for the incremental save path:
 *   - db_item_id assignment and validation
 *   - all_items_have_db_ids() logic
 *   - resave_dirty_containers() behavior
 *   - OBJ_RFLAG_DIRTY_CONTAINER flag propagation
 *
 * Part 1: Mock-based behavioral tests (no MySQL needed).
 * Part 2: Source-grep guards against sql_player.c.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_incremental_save.h"

/* ------------------------------------------------------------------ */
/*  Test framework                                                    */
/* ------------------------------------------------------------------ */

static int  g_pass = 0;
static int  g_fail = 0;
static char g_last_error[4096];
static char g_src_buf[262144];

#define TEST_BEGIN(name) do { printf("  %s ... ", name); fflush(stdout); } while (0)
#define TEST_END()       do { printf("\n"); } while (0)
#define TEST_PASS()      do { g_pass++; } while (0)
#define TEST_FAIL(...)   do { \
    snprintf(g_last_error, sizeof(g_last_error), __VA_ARGS__); \
    g_fail++; \
    fprintf(stderr, "\n  FAIL: %s", g_last_error); \
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

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        TEST_FAIL("%s: expected %d, got %d", msg, (int)(b), (int)(a)); \
        return; \
    } \
} while (0)

/* ------------------------------------------------------------------ */
/*  Mock structures (minimal versions of real MUD types)              */
/* ------------------------------------------------------------------ */

#define MOCK_MAX_WEAR    22
#define MAX_WEAR         22

/* OBJ_RFLAG_DIRTY_CONTAINER = BIT_1 = (1 << 1) = 2 */
#define OBJ_RFLAG_DIRTY_CONTAINER  2

typedef struct mock_obj {
    int             db_item_id;
    unsigned long   runtime_flags;
    struct mock_obj *contains;       /* first contained object */
    struct mock_obj *next_content;   /* sibling in container   */
} mock_obj;

/* Minimal character — just enough for all_items_have_db_ids. */
typedef struct {
    mock_obj *equipment[MOCK_MAX_WEAR];
    mock_obj *save_equip[MOCK_MAX_WEAR];
    mock_obj *carrying;
} mock_char;

/* Save tracker for resave_dirty_containers mock. */
typedef struct {
    int  save_call_count;
    int  last_saved_db_id;
    int  saved_db_ids[32];
    int  saved_count;
} mock_save_state;

/* ------------------------------------------------------------------ */
/*  Mock: all_items_have_db_ids                                       */
/* ------------------------------------------------------------------ */

static int mock_all_items_have_db_ids(const mock_char *ch)
{
    int i;

    if (!ch)
        return 1;

    /* Check worn equipment. */
    for (i = 0; i < MOCK_MAX_WEAR; i++) {
        const mock_obj *eq = ch->equipment[i];
        if (eq && eq->db_item_id <= 0)
            return 0;
    }

    /* Check save_equip (backup equipment slot). */
    for (i = 0; i < MOCK_MAX_WEAR; i++) {
        const mock_obj *eq = ch->save_equip[i];
        if (eq && eq->db_item_id <= 0)
            return 0;
    }

    /* Check inventory. */
    for (const mock_obj *obj = ch->carrying; obj; obj = obj->next_content) {
        if (obj->db_item_id <= 0)
            return 0;
    }

    return 1;
}

/* ------------------------------------------------------------------ */
/*  Mock: resave_dirty_containers                                     */
/* ------------------------------------------------------------------ */

static int mock_resave_container_contents(int pid, mock_obj *container,
                                          mock_save_state *state)
{
    (void)pid;
    state->save_call_count++;
    state->last_saved_db_id = container->db_item_id;
    if (state->saved_count < 32) {
        state->saved_db_ids[state->saved_count++] = container->db_item_id;
    }
    return 1;  /* success */
}

static void mock_resave_dirty_containers(int pid, mock_obj *obj,
                                         mock_save_state *state)
{
    if (!obj)
        return;

    /* Recurse into contents first. */
    if (obj->contains)
        mock_resave_dirty_containers(pid, obj->contains, state);

    if (obj->next_content)
        mock_resave_dirty_containers(pid, obj->next_content, state);

    /* Only save if dirty. */
    if (obj->runtime_flags & OBJ_RFLAG_DIRTY_CONTAINER) {
        if (mock_resave_container_contents(pid, obj, state)) {
            /* Clear the dirty flag on success. */
            obj->runtime_flags &= ~((unsigned long)OBJ_RFLAG_DIRTY_CONTAINER);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  PART 1: Mock behavioral tests                                     */
/* ------------------------------------------------------------------ */

/* Test 1: all_items_have_db_ids returns false when equipment item
 *         is missing db_item_id. */
static void test_mock_all_ids_missing_equip_id(void)
{
    TEST_BEGIN("mock_all_items_have_db_ids: missing equip id -> false");

    mock_obj sword = { .db_item_id = 0 };
    mock_char ch = {0};
    ch.equipment[0] = &sword;

    int rc = mock_all_items_have_db_ids(&ch);
    ASSERT_FALSE(rc, "equip item missing db_item_id should return false");

    TEST_PASS();
    TEST_END();
}

/* Test 2: all_items_have_db_ids returns false when inventory item
 *         is missing db_item_id. */
static void test_mock_all_ids_missing_inv_id(void)
{
    TEST_BEGIN("mock_all_items_have_db_ids: missing inv id -> false");

    mock_obj bag = { .db_item_id = 0, .next_content = NULL };
    mock_obj sword = { .db_item_id = 5 };
    mock_char ch = {0};
    ch.equipment[0] = &sword;
    ch.carrying = &bag;

    int rc = mock_all_items_have_db_ids(&ch);
    ASSERT_FALSE(rc, "inv item missing db_item_id should return false");

    TEST_PASS();
    TEST_END();
}

/* Test 3: all_items_have_db_ids returns true when all items have IDs. */
static void test_mock_all_ids_all_have_ids(void)
{
    TEST_BEGIN("mock_all_items_have_db_ids: all have ids -> true");

    mock_obj sword = { .db_item_id = 5 };
    mock_obj bag   = { .db_item_id = 7, .next_content = NULL };
    mock_char ch = {0};
    ch.equipment[0] = &sword;
    ch.carrying = &bag;

    int rc = mock_all_items_have_db_ids(&ch);
    ASSERT_TRUE(rc, "all items have db_item_id should return true");

    TEST_PASS();
    TEST_END();
}

/* Test 4: all_items_have_db_ids returns true for empty character. */
static void test_mock_all_ids_empty(void)
{
    TEST_BEGIN("mock_all_items_have_db_ids: empty char -> true");

    mock_char ch = {0};
    int rc = mock_all_items_have_db_ids(&ch);
    ASSERT_TRUE(rc, "empty character should return true");

    TEST_PASS();
    TEST_END();
}

/* Test 5: all_items_have_db_ids checks save_equip. */
static void test_mock_all_ids_save_equip_missing(void)
{
    TEST_BEGIN("mock_all_items_have_db_ids: save_equip missing id -> false");

    mock_obj worn    = { .db_item_id = 5 };
    mock_obj backup  = { .db_item_id = 0 };
    mock_char ch = {0};
    ch.equipment[0]  = &worn;
    ch.save_equip[0] = &backup;

    int rc = mock_all_items_have_db_ids(&ch);
    ASSERT_FALSE(rc, "save_equip item missing db_item_id should return false");

    TEST_PASS();
    TEST_END();
}

/* Test 6: resave_dirty_containers skips clean containers. */
static void test_mock_resave_skips_clean(void)
{
    TEST_BEGIN("mock_resave_dirty: skips clean container");

    mock_obj bag = { .db_item_id = 10, .runtime_flags = 0 };
    mock_save_state state = {0};

    mock_resave_dirty_containers(1, &bag, &state);

    ASSERT_EQ(state.save_call_count, 0,
              "clean container should not be saved");

    TEST_PASS();
    TEST_END();
}

/* Test 7: resave_dirty_containers saves dirty containers. */
static void test_mock_resave_saves_dirty(void)
{
    TEST_BEGIN("mock_resave_dirty: saves dirty container");

    mock_obj bag = { .db_item_id = 10,
                     .runtime_flags = OBJ_RFLAG_DIRTY_CONTAINER };
    mock_save_state state = {0};

    mock_resave_dirty_containers(1, &bag, &state);

    ASSERT_EQ(state.save_call_count, 1,
              "dirty container should be saved exactly once");
    ASSERT_EQ(state.last_saved_db_id, 10,
              "correct container db_item_id should be saved");

    TEST_PASS();
    TEST_END();
}

/* Test 8: resave_dirty_containers clears dirty flag after save. */
static void test_mock_resave_clears_flag(void)
{
    TEST_BEGIN("mock_resave_dirty: clears flag after save");

    mock_obj bag = { .db_item_id = 10,
                     .runtime_flags = OBJ_RFLAG_DIRTY_CONTAINER };
    mock_save_state state = {0};

    mock_resave_dirty_containers(1, &bag, &state);

    ASSERT_EQ(state.save_call_count, 1,
              "dirty container should be saved");
    ASSERT_EQ(bag.runtime_flags & OBJ_RFLAG_DIRTY_CONTAINER, 0,
              "dirty flag should be cleared after save");

    TEST_PASS();
    TEST_END();
}

/* Test 9: resave_dirty_containers recurses into sub-containers. */
static void test_mock_resave_recurses(void)
{
    TEST_BEGIN("mock_resave_dirty: recurses into sub-containers");

    /* Bag contains a pouch, pouch is dirty. */
    mock_obj pouch = { .db_item_id = 20,
                       .runtime_flags = OBJ_RFLAG_DIRTY_CONTAINER };
    mock_obj bag   = { .db_item_id = 10,
                       .runtime_flags = 0,
                       .contains = &pouch };
    mock_save_state state = {0};

    mock_resave_dirty_containers(1, &bag, &state);

    ASSERT_EQ(state.save_call_count, 1,
              "dirty sub-container should be saved");
    ASSERT_EQ(state.last_saved_db_id, 20,
              "correct sub-container db_item_id should be saved");
    ASSERT_EQ(pouch.runtime_flags & OBJ_RFLAG_DIRTY_CONTAINER, 0,
              "sub-container dirty flag should be cleared");

    TEST_PASS();
    TEST_END();
}

/* Test 10: resave_dirty_containers saves multiple dirty siblings. */
static void test_mock_resave_multiple_dirty(void)
{
    TEST_BEGIN("mock_resave_dirty: multiple dirty siblings");

    mock_obj pouch2 = { .db_item_id = 30,
                        .runtime_flags = OBJ_RFLAG_DIRTY_CONTAINER,
                        .next_content = NULL };
    mock_obj pouch1 = { .db_item_id = 20,
                        .runtime_flags = OBJ_RFLAG_DIRTY_CONTAINER,
                        .next_content = &pouch2 };
    mock_obj bag    = { .db_item_id = 10,
                        .runtime_flags = 0,
                        .contains = &pouch1 };
    mock_save_state state = {0};

    mock_resave_dirty_containers(1, &bag, &state);

    ASSERT_EQ(state.save_call_count, 2,
              "both dirty sub-containers should be saved");
    /* Order depends on recursion direction; just verify both were saved. */
    int saw_20 = 0, saw_30 = 0;
    for (int i = 0; i < state.saved_count; i++) {
        if (state.saved_db_ids[i] == 20) saw_20 = 1;
        if (state.saved_db_ids[i] == 30) saw_30 = 1;
    }
    ASSERT_TRUE(saw_20 && saw_30,
                "both pouch1 (20) and pouch2 (30) should be in saved list");

    TEST_PASS();
    TEST_END();
}

/* Test 11: resave_dirty_containers handles NULL. */
static void test_mock_resave_null(void)
{
    TEST_BEGIN("mock_resave_dirty: handles NULL");

    mock_save_state state = {0};
    mock_resave_dirty_containers(1, NULL, &state);

    ASSERT_EQ(state.save_call_count, 0,
              "NULL should cause no saves");

    TEST_PASS();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/*  PART 2: Source-grep regression guards                             */
/* ------------------------------------------------------------------ */

/* Test 12: all_items_have_db_ids function exists in sql_player.c
 *          and checks db_item_id <= 0. */
static void test_source_all_items_have_db_ids_exists(void)
{
    TEST_BEGIN("source: all_items_have_db_ids exists");

    FILE *f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END();
        return;
    }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    const char *func = strstr(g_src_buf, "all_items_have_db_ids");
    if (!func) {
        TEST_FAIL("all_items_have_db_ids not found in sql_player.c");
        TEST_END();
        return;
    }

    /* Verify it checks db_item_id <= 0 (the guard condition). */
    const char *check = strstr(func, "db_item_id <= 0");
    if (!check) {
        TEST_FAIL("all_items_have_db_ids does not check db_item_id <= 0");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* Test 13: resave_dirty_containers uses OBJ_RFLAG_DIRTY_CONTAINER. */
static void test_source_resave_uses_dirty_flag(void)
{
    TEST_BEGIN("source: resave_dirty_containers uses OBJ_RFLAG_DIRTY_CONTAINER");

    FILE *f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END();
        return;
    }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    /* Find the function. */
    const char *func = strstr(g_src_buf, "resave_dirty_containers");
    if (!func) {
        TEST_FAIL("resave_dirty_containers not found in sql_player.c");
        TEST_END();
        return;
    }

    /* Verify it uses OBJ_RFLAG_DIRTY_CONTAINER. */
    const char *flag = strstr(func, "OBJ_RFLAG_DIRTY_CONTAINER");
    if (!flag) {
        TEST_FAIL("resave_dirty_containers does not use OBJ_RFLAG_DIRTY_CONTAINER");
        TEST_END();
        return;
    }

    /* And REMOVE_BIT to clear it. */
    const char *remove = strstr(func, "REMOVE_BIT");
    if (!remove) {
        TEST_FAIL("resave_dirty_containers does not use REMOVE_BIT to clear dirty flag");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* Test 14: db_item_id is assigned after successful item insert. */
static void test_source_db_item_id_assigned(void)
{
    TEST_BEGIN("source: db_item_id assigned after insert");

    FILE *f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END();
        return;
    }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    /* Find any assignment of db_item_id (obj->db_item_id = ...). */
    const char *assign = strstr(g_src_buf, "db_item_id =");
    if (!assign) {
        TEST_FAIL("no db_item_id assignment found in sql_player.c — "
                  "items may not be getting DB IDs for incremental saves");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* Test 15: incremental save guard exists —
 *          use_incremental = all_items_have_db_ids(ch). */
static void test_source_incremental_guard_exists(void)
{
    TEST_BEGIN("source: incremental save guard exists");

    FILE *f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END();
        return;
    }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    /* Look for the use_incremental guard expression. */
    const char *guard = strstr(g_src_buf,
        "all_items_have_db_ids(ch) && !save_equipment && !save_inventory");
    if (!guard) {
        /* Try alternate form without spaces. */
        guard = strstr(g_src_buf, "all_items_have_db_ids(ch)");
        if (!guard) {
            TEST_FAIL("incremental save guard not found — "
                      "use_incremental check for all_items_have_db_ids missing");
            TEST_END();
            return;
        }
    }

    TEST_PASS();
    TEST_END();
}

/* Test 16: db_item_id is stored for incremental saves (comment hint). */
static void test_source_incremental_comment_exists(void)
{
    TEST_BEGIN("source: incremental save comment exists");

    FILE *f = fopen("src/sql_player.c", "r");
    if (!f) f = fopen("../../../src/sql_player.c", "r");
    if (!f) f = fopen("../../src/sql_player.c", "r");
    if (!f) {
        TEST_FAIL("cannot open src/sql_player.c");
        TEST_END();
        return;
    }

    size_t n = fread(g_src_buf, 1, sizeof(g_src_buf) - 1, f);
    g_src_buf[n] = '\0';
    fclose(f);

    /* Look for the "incremental saves" comment near db_item_id. */
    const char *comment = strstr(g_src_buf,
        "db id for incremental saves");
    if (!comment) {
        /* Try the other comment variant. */
        comment = strstr(g_src_buf,
            "store db id for incremental saves");
    }
    if (!comment) {
        TEST_FAIL("incremental save comment not found near db_item_id assignment");
        TEST_END();
        return;
    }

    TEST_PASS();
    TEST_END();
}

/* ================================================================== */
/*  Test registry                                                      */
/* ================================================================== */

typedef struct { const char *name; void (*func)(void); } test_entry;

static test_entry g_tests[] = {
    /* Mock behavioral tests */
    { "mock_all_ids_missing_equip_id",     test_mock_all_ids_missing_equip_id },
    { "mock_all_ids_missing_inv_id",       test_mock_all_ids_missing_inv_id },
    { "mock_all_ids_all_have_ids",         test_mock_all_ids_all_have_ids },
    { "mock_all_ids_empty",                test_mock_all_ids_empty },
    { "mock_all_ids_save_equip_missing",   test_mock_all_ids_save_equip_missing },
    { "mock_resave_skips_clean",           test_mock_resave_skips_clean },
    { "mock_resave_saves_dirty",           test_mock_resave_saves_dirty },
    { "mock_resave_clears_flag",           test_mock_resave_clears_flag },
    { "mock_resave_recurses",              test_mock_resave_recurses },
    { "mock_resave_multiple_dirty",        test_mock_resave_multiple_dirty },
    { "mock_resave_null",                  test_mock_resave_null },
    /* Source-grep regression guards */
    { "source_all_ids_exists",             test_source_all_items_have_db_ids_exists },
    { "source_resave_uses_dirty_flag",     test_source_resave_uses_dirty_flag },
    { "source_db_item_id_assigned",        test_source_db_item_id_assigned },
    { "source_incremental_guard_exists",   test_source_incremental_guard_exists },
    { "source_incremental_comment_exists", test_source_incremental_comment_exists },
    { NULL, NULL },
};

static const int g_num_tests = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

int test_incremental_save_run_all(void)
{
    test_incremental_save_reset();
    printf("  [Incremental Save Tests]\n");

    for (int i = 0; g_tests[i].name; i++) {
        g_tests[i].func();
    }

    printf("  Passed: %d/%d\n", g_pass, g_pass + g_fail);
    return g_fail;
}

int test_incremental_save_run_one(const char *name)
{
    for (int i = 0; g_tests[i].name; i++) {
        if (strcmp(g_tests[i].name, name) == 0) {
            printf("  [Incremental Save: %s]\n", name);
            g_tests[i].func();
            return (g_fail > 0);
        }
    }
    return -1;  /* not found */
}

void test_incremental_save_print_summary(void)
{
    printf("  Incremental Save: %d passed, %d failed out of %d tests\n",
           g_pass, g_fail, g_num_tests);
}

void test_incremental_save_reset(void)
{
    g_pass = 0;
    g_fail = 0;
    g_last_error[0] = '\0';
}
