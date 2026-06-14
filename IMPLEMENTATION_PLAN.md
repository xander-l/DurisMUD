# Implementation Plan: Multithreading Database Persistence

**Branch:** `feature/multithreading-database-persistence`
**Last Updated:** June 14, 2026 (Phase 6 marked as draft/skipped)

---

## Status Legend
- ✅ Complete
- 🔶 Partially Complete
- ❌ Not Started
- 🚧 In Progress
- 📝 Drafted/Deferred (plan exists, intentionally not implemented)

---

## Phase 1: Core Player Table Migration (Foundation)

| Task | Status | Notes |
|---|---|---|
| MySQL connection management (`sql.c`, `sql.h`) | ✅ | DB connection, child connection for forks |
| `sql_escape_string`, `sql_run_query`, `sql_player_error` | ✅ | Utility functions |
| Player existence check (`sql_player_exists`) | ✅ | |
| PID lookup (`sql_get_player_pid`) | ✅ | |
| Player delete (`sql_delete_player`, `sql_delete_player_by_name`) | ✅ | |
| Player rename (`sql_player_rename`) | ✅ | |
| `player_data` table schema + save/load | ✅ | 60+ column full player status (`sql_save/load_player_status`) |

---

## Phase 2: Transaction & Batch Infrastructure

| Task | Status | Notes |
|---|---|---|
| `sql_begin_transaction`, `sql_commit`, `sql_rollback` | ✅ | Transaction state tracking |
| `warn_outside_txn` helper | ✅ | Safety net for unwrapped save calls |
| `batch_append` helper (safe snprintf with truncation check) | ✅ | Replaces fragile `pos > buf_size - 200` pattern |
| REPLACE INTO migration for sub-tables | ✅ | Languages, intros, timers, undead slots, forged items, granted cmds |
| Heap-buffer overflow fixes | ✅ | `char_to_room`, `move_cost` fixes |
| Retry logic for transient DB failures | ✅ | |

---

## Phase 3.1–3.4: Player Sub-Table & Item Save/Load

| Task | Status | Notes |
|---|---|---|
| `player_data` (core status) save/load | ✅ | `sql_save/load_player_status` — large single-row table |
| `player_skills` save/load | ✅ | Batched REPLACE INTO |
| `player_affects` save/load | ✅ | `bitvector1-5` migrated to `BIGINT UNSIGNED` |
| `player_witnesses` save/load | ✅ | |
| `player_shapechanges` save/load | ✅ | JSON spellbook via extra_descr |
| `player_recipes` save/load | ✅ | Individual `INSERT IGNORE` per recipe |
| `player_languages` | ✅ | Batched REPLACE INTO |
| `player_intros` | ✅ | Batched REPLACE INTO |
| `player_timers` | ✅ | Batched REPLACE INTO |
| `player_undead_slots` | ✅ | Batched REPLACE INTO |
| `player_forged_items` | ✅ | Batched REPLACE INTO |
| `player_granted_cmds` | ✅ | Batched REPLACE INTO |
| `player_pets` save/load (core) | ✅ | Charm duration, mob vnum, room placement |
| `sql_persistence_item_owner_matches` (function) | ✅ | Ownership verification for loaded items |
| `sql_persistence_item_owner_matches` (test anchor) | ✅ | Test anchor in src/sql.c verified — real implementation found and tested |
| `sql_load_item_extra_descr_from_table` | ✅ | Shared extra description loader |
| `sql_load_item_affects_from_table` | ✅ | Shared affect loader |
| Account management (`sql_save/load_account`, `sql_link_player_to_account`) | ✅ | |
| Locker character (`sql_save/load/delete_locker`) | ✅ | Distinct from locker_items |
| Account bank (`sql_load/save_account_bank`, deposit/withdraw) | ✅ | |
| Guilds (`sql_save/load/delete_guild`) | ✅ | |
| Ships (`sql_save/load/delete_ship`) | ✅ | |
| Towns (`sql_save/load_towns`) | ✅ | |
| Account IPs (`sql_save/load/delete_account_ips`) | ✅ | |
| Kingdom land (`sql_save_kingdom_land`) | ✅ | |

---

## Phase 3.5: v19 Schema Migration (wear_flags, item_type, bitvector1-5)

### Save Side
| Task | Status | Notes |
|---|---|---|
| Shared helper `sql_format_item_diff_fields_and_free_proto` | ✅ | Extracted diff-from-prototype logic |
| player_items individual save INSERT | ✅ | Uses shared helper |
| player_pet_items save INSERT | ✅ | Uses shared helper |
| locker_items save INSERT | ✅ | |
| shopkeeper_items save INSERT | ✅ | |
| corpse_items save INSERT | ✅ | |
| saved_items save INSERT | ✅ | |
| siege_items save INSERT | ✅ | |
| player_items batch save INSERT | ✅ | Added wear_flags, type, material (no bv — intentional for simple items) |

### Load Side
| Task | Status | Notes |
|---|---|---|
| player_items load (col++ pattern) | ✅ | Uses `sql_row_int`/`sql_row_ulong` with prototype defaults |
| player_pet_items load | ✅ | `item_row[20-27]` |
| locker_items / account_locker_items load | ✅ | `row[22-27]` for bitvectors+material, `row[6-7]` for wear/type |
| shopkeeper_items load | ✅ | `row[19-26]` for v19, container at `row[27]` |
| corpse_items load | ✅ | `row[30-37]` after complex JOIN |
| saved_items load | ✅ | `row[18-25]` |
| siege_items load | ✅ | `row[18-25]` |

---

## Phase 3.6: item_material Column

### Save Side
| Task | Status | Notes |
|---|---|---|
| Shared helper includes `material_str` | ✅ | Diff-from-prototype: NULL if matches |
| player_items individual save | ✅ | |
| player_items batch save | ✅ | Added material (commit `d0a49942`) |
| player_pet_items save | ✅ | |
| locker_items save | ✅ | |
| shopkeeper_items save | ✅ | |
| corpse_items save | ✅ | |
| saved_items save | ✅ | |
| siege_items save | ✅ | |

### Load Side
| Task | Status | Notes |
|---|---|---|
| player_items load (col++) | ✅ | `sql_row_int(row, col++, obj->material)` |
| player_pet_items load | ✅ | `item_row[22]` |
| account_locker_items load | ✅ | `row[27]` |
| shopkeeper_items load | ✅ | `row[21]` |
| corpse_items load | ✅ | `row[32]` |
| saved_items load | ✅ | `row[20]` |
| siege_items load | ✅ | `row[20]` |

---

## Phase 3.7: Code Quality & Bug Fixes

| Task | Status | Notes |
|---|---|---|
| Remove duplicated `row[18-25]` block in player_items load | ✅ | Dead code read string columns as ints |
| Fix corrupted comment (two merged into one line) | ✅ | |
| Save-side audit — all 7 tables write item_material | ✅ | Found & fixed batch save gap |
| Load-side audit — all 7 tables read item_material | ✅ | Found & fixed saved/siege/shopkeeper missing reads |
| Add wear_flags + item_type to batch save | ✅ | Completes batch save v19 coverage |
| Final end-to-end audit | ✅ | All 14 checks pass, no remaining gaps |

---

## Phase 4: Build & Testing

| Task | Status | Notes |
|---|---|---|
| Docker test harness (`tests/db_write/`) | ✅ | `Dockerfile.test` installs `build-essential`, compiles via Makefile |
| Docker-based build flow | ✅ | Workspace has no local compiler; all builds use Docker |
| `test_db_write` (main test binary) | ✅ | 13/13 crash-safety, 11/11 rollback |
| `test_main` (test entry point) | ✅ | |
| `test_data_validation` | ✅ | |
| `test_game_scenarios` | ✅ | |
| `test_container_rescue` | ✅ | |
| `test_crash_stress` | ✅ | |
| `test_disconnect_flush` | ✅ | |
| `test_persistence_owner` | ✅ | |
| `test_transaction_rollback` | ✅ | |
| `test_character_lifecycle` | ✅ | |
| Full Docker clean build (`--no-cache`) verified | ✅ | `sql_player.c` compiles clean, all tests pass |
| Integration roundtrip tests | ✅ | 26 structural regression tests (commit `7b77ea6b`) |
| Multi-table consistency tests | ✅ | 16 source-grep tests: all 7 tables listed, DELETE-before-INSERT, obj_uid, owner_matches, txn wrapping |

> **Build flow:** This workspace has no local compiler. All compilation and testing is done through Docker using `tests/db_write/Dockerfile.test` (ubuntu:24.04 + build-essential). The Makefile compiles test sources with `-DPRODUCTION_SOURCE_PATH` pointing to `src/sql_player.c`.

---

## Phase 5: Remaining Work

| Task | Status | Notes |
|---|---|---|
| Transaction wrappers for all save sub-functions | ✅ | All 7 sql_save_player_* + 4 locker/corpse helpers use own_txn; warn_outside_txn removed |
| Flush path on disconnect | ✅ | close_socket, actwiz.c, copyover.c, redis.c — all disconnect/crash paths wrap save in txn; sql_save_player uses own_txn |
| Thread safety documentation (`THREAD_SAFETY.md`) | ✅ | Mutex inventory, lock hierarchy, ABBA prevention |
| Persistence lock hierarchy | ✅ | Documented |
| `sql_persistence_item_owner_matches` test anchor fix | ✅ | Anchor verified — test finds real implementation via leading comment |
| Monitoring / scalar tracking enhancements | ✅ | sql_zone_touch_finished implemented with async persistence queue pattern |
| Other table files (`sql_mob`, `sql_room`, etc.) | 📝 | Plan drafted as Phase 6 (refactor scope); deferred per user direction |
| Performance optimization (query batching, connection pooling) | ❌ | |
| Incremental save path (dirty flags, `db_item_id`) | ✅ | 16 regression tests: 11 mock behavioral + 5 source-grep guards |

---

## Phase 6: Refactor — Split `sql_player.c` into Focused Files *(DRAFT — skipped for now)*

> **Status:** DRAFT — deferred. The refactor is well-scoped (see plan below) but the user has decided to skip it for now in favor of higher-impact work. All concrete steps remain documented for future activation.

**Context:** `src/sql_player.c` is **9,209 lines** with ~120 functions covering 6+ unrelated domains. It's the largest file in the project and the dominant contributor to incremental compile times. This phase splits it into ~6 cohesive files grouped by entity/table family, with **zero behavior change**.

### Target File Structure

| # | New File | Lines | Domain | Key Functions |
|---|---|---|---|---|
| 1 | `src/sql_player.c` | ~1,700 | **Player core** (txn, master save, status, record, migration) | `sql_begin_transaction`, `sql_save_player`, `sql_save/load_player_status`, `sql_load_player`, `sql_migrate_player`, etc. |
| 2 | `src/sql_player_subtables.c` | ~800 | **Player sub-tables** (skills, affects, witnesses, shapechanges, recipes) | `sql_save/load_player_{skills,affects,witnesses,shapechanges,recipes}` |
| 3 | `src/sql_player_items.c` | ~2,500 | **All 7 item tables + pets** (player_items, player_pet_items, locker_items, account_locker_items, shopkeeper_items, corpse_items, saved_items, siege_items) | `sql_save/load_player_items`, `sql_save/load_player_pets`, batch helpers, row helpers |
| 4 | `src/sql_account.c` | ~1,500 | **Accounts** (accounts, bank, IPs, towns, kingdom) | `sql_save/load_account`, `sql_account_bank_*`, `sql_save/load_account_ips`, `sql_save/load_towns`, `sql_save_kingdom_land` |
| 5 | `src/sql_locker.c` | ~1,100 | **Lockers + private chests** | `sql_save/load_locker`, `sql_locker_exists`, `sql_create_private_chest`, `sql_log_chest_activity`, `sql_save/load_private_chest_items` |
| 6 | `src/sql_world_entities.c` | ~3,500 | **World entities** (corpses, shopkeepers, saved items, siege, ships, guilds, spellbook) | `sql_save/load_corpse`, `sql_save/restore_shopkeeper`, `sql_save_saved_item`, `sql_save/load_siege_list`, `sql_save/load_ship`, `sql_save/load_guild`, `sql_add_spellbook_mob` |

**Total:** ~11,100 lines (vs 9,209 original). The ~1,900 line increase comes from per-file `__NO_MYSQL__` stub blocks (currently a single 122-line block at the top of `sql_player.c`).

### Architecture Decisions

1. **Headers:** `src/sql_player.h` stays as the single public header — all 50+ declarations remain there. No callers need to change.
2. **Internal header:** New `src/sql_player_internal.h` for ~6 static helpers used across files:
   - `sql_run_query` (currently static, used by many functions)
   - `batch_append` (currently static, used by item save)
   - `sql_format_item_diff_fields_and_free_proto` (used by all 7 item tables)
   - `sql_row_int` / `sql_row_long` / `sql_row_ulong` / `sql_row_str` (used by all item load paths)
   - `in_transaction` global: definition stays in `sql_player.c` (only file that mutates it); `extern bool in_transaction;` declared in `sql_player_internal.h`; **rule: no other file may write to it — read via `sql_in_transaction()` only**
3. **`__NO_MYSQL__` stubs:** Each new file gets its own minimal stub block listing only the functions it defines. This preserves the existing compile-without-MySQL behavior.
4. **Build system:** `src/Makefile` gets 5 new entries in the `OBJS` list (`sql_player_subtables.o`, `sql_player_items.o`, `sql_account.o`, `sql_locker.o`, `sql_world_entities.o`). The Makefile uses `-MMD -MP` for auto-generated dependencies, so no manual dep updates are needed for the new files.
5. **No behavior change:** All function signatures, all SQL queries, all transaction boundaries, all error handling unchanged. This is purely a file-layout refactor.
6. **Cross-file call graph:** `sql_save_player` (master, file 1) calls `sql_save_player_status` (file 1), `sql_save_player_items` (file 3), `sql_save_player_skills` (file 2), etc. This works because all sub-function declarations are in `sql_player.h` (public) or `sql_player_internal.h` (shared helpers). The **only cross-file concern is the master save function** — verify its callee declarations are all in one of these two headers.

### Implementation Order

Each step ends with a Docker build + full test run before moving to the next, so regressions are caught early.

| Step | Task | Risk | Validation |
|---|---|---|---|
| 1 | Create `src/sql_player_internal.h` with shared static helpers | Low | Build + tests pass with no other changes |
| 2 | Extract `sql_player_subtables.c` (smallest, lowest risk) | Low | Build + tests pass; sub-table save/load still works |
| 3 | Extract `sql_account.c` | Low-Med | Build + tests pass; account flows still work |
| 4 | Extract `sql_locker.c` | Medium | Build + tests pass; locker roundtrip still works |
| 5 | Extract `sql_player_items.c` (largest item domain) | Medium-High | Build + tests pass; v19+material+incremental save still works |
| 6 | Extract `sql_world_entities.c` (everything else) | High | Build + tests pass; corpses, shopkeepers, ships, guilds all still work |
| 7 | Update `src/Makefile` to compile all 6 files | — | Build succeeds |
| 8 | Final code review + commit (one commit per extracted file, 5 commits total, so regressions can be bisected) | — | Tests pass, review approved |

**Commit strategy:** One commit per extracted file (steps 2–6 = 5 commits). Each commit is preceded by a passing build+test run, so the history is bisectable. Final step (7) is its own commit.

### Risks & Mitigations

| Risk | Mitigation |
|---|---|
| Static helper used by moved function isn't in the new file's scope | Put in `sql_player_internal.h` and make non-static |
| `__NO_MYSQL__` stub missing in new file causes linker error | Audit each new file's function list and replicate stubs |
| Cross-file function ordering issue (forward decl) | Internal header has the right declarations; sql_player.h is included first |
| `tests/db_write/Dockerfile.test` doesn't copy new .c files | Already uses `COPY src/` (entire src directory) — no change needed |
| Test source-grep tests break (e.g. multi-table_consistency checks `src/sql_player.c` for specific patterns) | After split, some patterns move to other files. Need to update tests to check all sql_*.c files or specific new files |
| `in_transaction` global accessed from multiple files | Keep in `sql_player.c`; add `extern` in internal header |
| Function name collision (e.g., `alloc_temp_char`, `free_temp_char` are generic names) | Before extracting each file, grep the rest of `src/` for the function name; if found, rename or qualify |

### Test Suite Updates — Migration Map

| Test File | Patterns That Move | Update Strategy |
|---|---|---|
| `test_multi_table_consistency.c` | `INSERT INTO locker_items`, `INSERT INTO shopkeeper_items`, `INSERT INTO corpse_items`, `INSERT INTO saved_items`, `INSERT INTO siege_items` — all move to `sql_player_items.c`; locker save/load moves to `sql_locker.c` | Change grep target from `src/sql_player.c` to `src/sql_player.c OR src/sql_player_items.c OR src/sql_locker.c` for table-specific checks; `src/sql_*.c` glob for the "all 7 tables" check |
| `test_transaction_rollback.c` | `sql_save_player_items`, `sql_save_locker`, `sql_save_corpse` (test anchor comments) — items to file 3, locker to file 5, corpse to file 6 | Move the test anchor comment with the function definition; update the source-grep target to the new file |
| `test_incremental_save.c` | `OBJ_RFLAG_DIRTY_CONTAINER`, `all_items_have_db_ids`, `resave_dirty_containers` — all stay in file 3 (`sql_player_items.c`) | Update source-grep target from `src/sql_player.c` to `src/sql_player_items.c` |
| `test_persistence_owner.c` | Already reads `src/sql.c` (which is not being split) | No change |
| `test_v19_roundtrip.c` | Mock-based, no source-grep | No change |
| `test_db_write.c` / `test_data_validation.c` / `test_game_scenarios.c` / `test_container_rescue.c` / `test_crash_stress.c` / `test_disconnect_flush.c` / `test_character_lifecycle.c` | Mock-based | No change |

**Pre-implementation step:** Before extracting file 1, run each source-grep test against current code and record which patterns live in `sql_player.c` and at which line. After extraction, update the test's grep target to point at the new file. This avoids "test passes for the wrong reason" issues.

### Out of Scope

- **No new functionality** — pure file-layout refactor
- **No API changes** — all declarations stay in `sql_player.h`
- **No SQL schema changes**
- **No transaction/batching improvements**
- **`sql_mob` / `sql_room` as new files** — these would add new persistence for mob/room data not currently in SQL; explicitly deferred to a future phase

---

## Test Suite Inventory

| Test File | Status | Covers |
|---|---|---|
| `test_main.c` | ✅ | Test entry point / runner |
| `test_db_write.c` | ✅ | Main harness, crash-safety (13), rollback (11) |
| `test_data_validation.c` | ✅ | Data integrity |
| `test_game_scenarios.c` | ✅ | Game mechanics + persistence |
| `test_container_rescue.c` | ✅ | Container recovery |
| `test_crash_stress.c` | ✅ | Crash stress testing |
| `test_disconnect_flush.c` | ✅ | Disconnect flush |
| `test_persistence_owner.c` | ✅ | Ownership verification |
| `test_transaction_rollback.c` | ✅ | Transaction rollback |
| `test_v19_roundtrip.c` | ✅ | 26 v19+material structural regression tests |
| `test_incremental_save.c` | ✅ | 16 incremental save tests: mock all_items_have_db_ids, resave_dirty_containers, source-grep guards |
| `test_multi_table_consistency.c` | ✅ | 16 multi-table consistency tests: all 7 tables, DELETE cleanup, obj_uid, owner_matches, txn wrapping |

---

## Source Files Inventory

| File | Purpose | Status |
|---|---|---|
| `src/sql.c` / `src/sql.h` | Core SQL utilities, connection management | ✅ All items complete |
| `src/sql_player.c` / `src/sql_player.h` | Player + item save/load (7 item tables) | ✅ v19+material complete |
| `src/sql_persistence_raw.c` | Raw persistence helpers | ✅ |
| `THREAD_SAFETY.md` | Mutex inventory, lock hierarchy | ✅ |

---

## Summary

All Phase 1–4 items complete. Phase 5 has 7 of 9 items complete; only two remain:
- Other table files (, , etc.) — not yet in scope
- Performance optimization (query batching, connection pooling) — not yet in scope

The plan covers 95+ individual tasks across 5 phases and 7 sub-phases, supported by 12 test files and 100+ regression tests.

