# Implementation Plan: Multithreading Database Persistence

**Branch:** `feature/multithreading-database-persistence`
**Last Updated:** June 13, 2026

---

## Status Legend
- ✅ Complete
- 🔶 Partially Complete
- ❌ Not Started
- 🚧 In Progress

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
| `sql_persistence_item_owner_matches` (test anchor) | 🔶 | Test expects anchor in `src/sql.c`; function may be elsewhere |
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
| Multi-table consistency tests | ❌ | |

> **Build flow:** This workspace has no local compiler. All compilation and testing is done through Docker using `tests/db_write/Dockerfile.test` (ubuntu:24.04 + build-essential). The Makefile compiles test sources with `-DPRODUCTION_SOURCE_PATH` pointing to `src/sql_player.c`.

---

## Phase 5: Remaining Work

| Task | Status | Notes |
|---|---|---|
| Transaction wrappers for all save sub-functions | ✅ | All 7 sql_save_player_* + 4 locker/corpse helpers use own_txn; warn_outside_txn removed |
| Flush path on disconnect | ✅ | close_socket/actwiz.c force-quit wrap final save in txn; sql_save_player uses own_txn |
| Thread safety documentation (`THREAD_SAFETY.md`) | ✅ | Mutex inventory, lock hierarchy, ABBA prevention |
| Persistence lock hierarchy | ✅ | Documented |
| `sql_persistence_item_owner_matches` test anchor fix | 🔶 | Test expects anchor in `src/sql.c`; function may be elsewhere |
| Monitoring / scalar tracking enhancements | 🔶 | |
| Other table files (`sql_mob`, `sql_room`, etc.) | ❌ | Not yet in scope |
| Performance optimization (query batching, connection pooling) | ❌ | |
| Incremental save path (dirty flags, `db_item_id`) | 🔶 | Implemented but not fully tested |

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

---

## Source Files Inventory

| File | Purpose | Status |
|---|---|---|
| `src/sql.c` / `src/sql.h` | Core SQL utilities, connection management | 🔶 `sql_persistence_item_owner_matches` test anchor |
| `src/sql_player.c` / `src/sql_player.h` | Player + item save/load (7 item tables) | ✅ v19+material complete |
| `src/sql_persistence_raw.c` | Raw persistence helpers | ✅ |
| `THREAD_SAFETY.md` | Mutex inventory, lock hierarchy | ✅ |

---

## Next High-Impact Item

**Integration roundtrip tests** — the db_write test suite verifies crash-safety and rollback behavior but doesn't test that v19 diff columns (wear_flags, item_type, item_material, bitvector1-5) survive a save→load roundtrip. Add tests that save items with non-default v19 values, load them back, and assert the values match across all 7 item tables.
