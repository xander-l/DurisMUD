/* ===================================================================
 * test_latency_guard.h — Declarations for the latency tracing and
 * pulse timing regression test suite.
 *
 * Verifies latency_trace_record calls for all game loop sections,
 * total_tick recording, latency dump scheduling, persistence queue
 * latency reporting, pulse interval definitions, and game loop
 * sleep budget adjustment.
 * Source-grep tests — no MySQL or MUD runtime needed.
 * =================================================================== */
#ifndef __TEST_LATENCY_GUARD_H__
#define __TEST_LATENCY_GUARD_H__

#ifdef __cplusplus
extern "C" {
#endif

int test_latency_guard_run_all(void);
int test_latency_guard_run_one(const char *name);
void test_latency_guard_print_summary(void);
void test_latency_guard_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_LATENCY_GUARD_H__ */
