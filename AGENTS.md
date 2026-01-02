# Agent Notes

- Attempting to compile with `__NO_MYSQL__` (e.g., `make -f Makefile.linux CFLAGS="... -D__NO_MYSQL__"`) fails because many files still reference MySQL types/symbols (e.g., `artifact.c` uses `MYSQL_RES`, `mysql_store_result`, `DB`, etc.). A MySQL-free build is not viable without wider refactors or additional conditional compilation; use `make -f Makefile.linux MOCK_MYSQL=1` to build with the mock MySQL client for harness runs.
- Running `scripts/test_nanny_login.py --bin src/dms_new --mini` without mocks currently fails in this environment with `MySQL initialization failed! Dying!` because no MySQL server is available; a reachable MySQL instance is required for non-mock harness runs.
