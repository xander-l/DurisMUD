/* ===================================================================
 * test_v19_roundtrip.h — Declarations for v19+material roundtrip tests
 * =================================================================== */
#ifndef __TEST_V19_ROUNDTRIP_H__
#define __TEST_V19_ROUNDTRIP_H__

#ifdef __cplusplus
extern "C" {
#endif

int test_v19_roundtrip_run_all(void);
int test_v19_roundtrip_run_one(const char *name);
void test_v19_roundtrip_print_summary(void);
void test_v19_roundtrip_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_V19_ROUNDTRIP_H__ */
