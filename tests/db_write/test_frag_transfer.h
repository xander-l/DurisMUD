/* ===================================================================
 * test_frag_transfer.h — Declarations for the frag/pkill and item
 * transfer regression test suite.
 *
 * Verifies the frag system (killed_by, fragWorthy, AddFrags,
 * frag_leaderboard) and item ownership transfers during death.
 * Source-grep tests — no MySQL or MUD runtime needed.
 * =================================================================== */
#ifndef __TEST_FRAG_TRANSFER_H__
#define __TEST_FRAG_TRANSFER_H__

#ifdef __cplusplus
extern "C" {
#endif

int test_frag_transfer_run_all(void);
int test_frag_transfer_run_one(const char *name);
void test_frag_transfer_print_summary(void);
void test_frag_transfer_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_FRAG_TRANSFER_H__ */
