/*
 * test_incremental_save.h
 *
 * Regression tests for the incremental save path:
 *   - db_item_id assignment and validation
 *   - all_items_have_db_ids() logic
 *   - resave_dirty_containers() behavior
 *   - OBJ_RFLAG_DIRTY_CONTAINER flag propagation
 *
 * Mock-based behavioral tests + source-grep guards against sql_player.c.
 */

#ifndef __TEST_INCREMENTAL_SAVE_H__
#define __TEST_INCREMENTAL_SAVE_H__

int  test_incremental_save_run_all(void);
int  test_incremental_save_run_one(const char *name);
void test_incremental_save_print_summary(void);
void test_incremental_save_reset(void);

#endif /* __TEST_INCREMENTAL_SAVE_H__ */
