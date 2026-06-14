/*
 * test_multi_table_consistency.h
 *
 * Source-grep regression guards that verify cross-table consistency
 * across all 7 item tables in sql_player.c:
 *   - player_items, player_pet_items, locker_items, shopkeeper_items,
 *     corpse_items, saved_items, siege_items
 *
 * Checks:
 *   - All 7 tables listed
 *   - DELETE-before-INSERT cleanup for each table
 *   - obj_uid present in INSERT statements
 *   - sql_persistence_item_owner_matches called on load
 *   - Transaction wrapping for multi-table save operations
 */

#ifndef __TEST_MULTI_TABLE_CONSISTENCY_H__
#define __TEST_MULTI_TABLE_CONSISTENCY_H__

int  test_multi_table_consistency_run_all(void);
int  test_multi_table_consistency_run_one(const char *name);
void test_multi_table_consistency_print_summary(void);
void test_multi_table_consistency_reset(void);

#endif /* __TEST_MULTI_TABLE_CONSISTENCY_H__ */
