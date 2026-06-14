/* ===================================================================
 * test_locker_stress.h — Declarations for the locker stress
 * regression test suite.
 *
 * Verifies locker save/load, enter/exit, chest access, private chest
 * password verification, and transaction wrapping.
 * Source-grep tests — no MySQL or MUD runtime needed.
 * =================================================================== */
#ifndef __TEST_LOCKER_STRESS_H__
#define __TEST_LOCKER_STRESS_H__

#ifdef __cplusplus
extern "C" {
#endif

int test_locker_stress_run_all(void);
int test_locker_stress_run_one(const char *name);
void test_locker_stress_print_summary(void);
void test_locker_stress_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_LOCKER_STRESS_H__ */
