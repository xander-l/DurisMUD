/* ===================================================================
 * test_pet_lifecycle.h — Declarations for the pet lifecycle test suite.
 *
 * Verifies the pet save/load path (sql_save_player_pets,
 * sql_load_player_pets), charm lifecycle (charm_generic, charm_broken),
 * and crash recovery edge cases.  Source-grep + SQL-pattern tests —
 * no MySQL or MUD runtime needed.
 * =================================================================== */
#ifndef __TEST_PET_LIFECYCLE_H__
#define __TEST_PET_LIFECYCLE_H__

#ifdef __cplusplus
extern "C" {
#endif

int test_pet_lifecycle_run_all(void);
int test_pet_lifecycle_run_one(const char *name);
void test_pet_lifecycle_print_summary(void);
void test_pet_lifecycle_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_PET_LIFECYCLE_H__ */
