/* ===================================================================
 * test_batch_save_stress.h — Declarations for the batch save
 * stress test suite.
 *
 * Verifies the sql_save_player_items_batch_all() function has:
 *   - Sub-batch splitting at 1MB threshold
 *   - Per-row fallback for oversized items
 *   - single_saved flag in flat_item struct
 *   - 64-bit mysql_insert_id (unsigned long long)
 *   - Container UPDATE via CASE WHEN
 *   - Phase 5 skipping single_saved items
 *
 * Source-grep tests — no MySQL or MUD runtime needed.
 * =================================================================== */
#ifndef __TEST_BATCH_SAVE_STRESS_H__
#define __TEST_BATCH_SAVE_STRESS_H__

#ifdef __cplusplus
extern "C" {
#endif

int test_batch_save_stress_run_all(void);
int test_batch_save_stress_run_one(const char *name);
void test_batch_save_stress_print_summary(void);
void test_batch_save_stress_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_BATCH_SAVE_STRESS_H__ */
