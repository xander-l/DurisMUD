/* ===================================================================
 * test_transaction_rollback.h — Declarations for the transaction
 * rollback test suite.
 *
 * Verifies that multi-step save functions (sql_save_towns,
 * sql_save_guild, sql_save_private_chest_items) correctly roll back
 * their DELETE statements when a subsequent INSERT fails, so that the
 * pre-save table contents are preserved (no data loss).
 *
 * These tests are standalone — no MySQL or MUD state needed. They
 * simulate the transaction control flow by building the expected
 * sequence of SQL operations and verifying that a failure at each
 * failure point triggers sql_rollback() instead of sql_commit().
 * =================================================================== */
#ifndef __TEST_TRANSACTION_ROLLBACK_H__
#define __TEST_TRANSACTION_ROLLBACK_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Test lifecycle ------------------------------------------------ */
int test_transaction_rollback_run_all(void);
int test_transaction_rollback_run_one(const char *name);
void test_transaction_rollback_print_summary(void);
void test_transaction_rollback_reset(void);

/* =================================================================
 * ROLLBACK VERIFICATION TESTS
 *
 * Each test simulates a failure at a specific point in a multi-step
 * save function and verifies that the transaction wrapping would
 * call sql_rollback() (not sql_commit()), so the pre-save DELETE
 * is undone and the table contents are preserved.
 * ================================================================= */

/* sql_save_towns: DELETE then INSERT loop.
 * Inject failure on the 2nd INSERT (1st succeeded) — verify rollback. */
int test_rollback_towns_insert_failure_preserves_old_towns(void);

/* sql_save_towns: inject failure on the very first INSERT after DELETE. */
int test_rollback_towns_first_insert_failure_preserves_old_towns(void);

/* sql_save_guild ranks loop: DELETE then INSERT loop.
 * Inject failure on 3rd rank INSERT — verify rollback. */
int test_rollback_guild_ranks_insert_failure_preserves_old_ranks(void);

/* sql_save_guild members loop: DELETE then INSERT loop.
 * Inject failure on 5th member INSERT — verify rollback. */
int test_rollback_guild_members_insert_failure_preserves_old_members(void);

/* sql_save_private_chest_items: DELETE then INSERT loop.
 * Inject failure on 2nd item INSERT — verify chest contents preserved. */
int test_rollback_private_chest_item_failure_preserves_chest(void);

/* sql_commit failure after all sub-saves succeed — verify rollback
 * is called and the whole save is undone. */
int test_rollback_commit_failure_undoes_whole_save(void);

/* The transaction wrapper must use sql_rollback() in a consistent
 * pattern: begin → work → on-fail rollback+return_false → on-success
 * commit. Verify the exact sequence of calls for each function. */
int test_rollback_call_sequence_is_begin_work_commit_or_rollback(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_TRANSACTION_ROLLBACK_H__ */
