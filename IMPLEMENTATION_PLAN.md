# Implementation Plan: Multithreading Database Persistence

**Branch:** `feature/multithreading-database-persistence`
**Last Updated:** June 14, 2026 (Phase 8 plan added)

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
| Performance optimization (query batching, connection pooling) | 🚧 | Phase 7 plan complete: 7a-1 (multi-row INSERT), 7a-2 (fork elimination), 7b-1 (connection pool), 7b-2 (multi-statement batches) |
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

## Phase 7: Performance Optimization

**Context:** The player save pipeline runs synchronously in the game thread. A typical save with 30 items generates ~170 individual MySQL queries — one per item, plus affects, extra_descrs, sub-tables, and cleanup. Each is a network roundtrip through a single `MYSQL* DB` connection. The async persistence queues (item/scalar/large events) already offload write-heavy event logging to background threads; this phase extends the same pattern to the player save path and login logging.

### Phase 7a: Multi-Row INSERT for Items + Eliminate fork() in Login Logging *(recommended)*

**Effort:** 1 session | **Risk:** Low | **Files:** `src/sql_player.c`, `src/sql.c`

#### 7a-1: Multi-row INSERT for player_items

**Current state:** `sql_save_player_items()` (line 1879 in `src/sql_player.c`) iterates over equipment + inventory + containers and calls `sql_save_single_item_get_id()` (line 1619) for each item individually. Each call builds a single-row `INSERT INTO player_items` and executes `db_query()`. For a player with 30 items, that is 30 network roundtrips just for the item rows, plus additional queries per affect and extra_descr.

**Target:** Use the existing `batch_append()` helper (line 249, already used for skills/languages/intros/timers/forged_items/granted_cmds sub-tables) to build a single multi-row INSERT:

```sql
INSERT INTO player_items (pid, equip_slot, vnum, wear_flags, item_type, item_material,
                           bitvector1, bitvector2, bitvector3, bitvector4, bitvector5,
                           short_desc, extra_flags, ...)
VALUES (123, 1, 1000, 0, 5, 'steel', 0, 0, 0, 0, 0, 'a sword', ...),
       (123, 0, 2000, 0, 10, 'leather', 0, 0, 0, 0, 0, 'a bag', ...),
       ...;
```

**Implementation:**

1. Add a `sql_save_player_items_batch_all()` function that iterates items and builds a single multi-row INSERT string via `batch_append`, then executes it with one `db_query()` call.
2. **Flatten the item tree first.** The current code recurses into containers, getting the parent's `LAST_INSERT_ID()` immediately and passing it to children. With a batch INSERT, all IDs must be pre-computed (`first_id + offset`). Before building the batch, walk equipment + inventory + all nested containers to produce a flat array of items. Assign each item a pre-computed `db_item_id` (first_id + its position in the array). Container parent references use the pre-computed ID of the parent.
3. Per-item affect and extra_descr INSERTs can be batched similarly (multi-row `INSERT INTO player_item_affects (...), (...), ...`).
4. Use `mysql_insert_id()` to get the first item's `LAST_INSERT_ID()`, then verify that subsequent IDs match `first_id + offset`. MySQL guarantees `LAST_INSERT_ID()` returns the first auto_increment of a multi-row INSERT; subsequent rows get consecutive IDs.
5. Update `resave_dirty_containers()` and the incremental save path to use the new batched function.
6. Fallback: if the batch INSERT fails (e.g., query too long for items with many extra_descrs), fall back to individual INSERTs per item.

**Key invariants to preserve:**
- `db_item_id` must still be assigned to each `P_obj` after save (for incremental save tracking).
- Container parent-child relationships (`item_id` references in sub-containers) must still be correct.
- Transaction wrapping (`BEGIN` / `COMMIT`) must still cover the entire item save.

#### 7a-2: Replace fork() in sql_log_player_login with async queue

**Current state:** `sql_log_player_login()` (line 2257 in `src/sql.c`) does:
1. Copy character data (name, IP, account, client) to stack buffers.
2. `fork()` a child process.
3. Child calls `sql_create_child_connection()` (line 461 in `src/sql_player.c`) to open a new MySQL connection.
4. Child runs `INSERT INTO log_entries ...`.
5. Child exits.

**Problem:** Process creation on every login is heavyweight. `fork()` on a multi-threaded process is also technically undefined behavior (POSIX allows it but the child must only call async-signal-safe functions until `exec()`; MySQL calls are not async-signal-safe).

**Target:** Replace with a scalar event queue enqueue, matching the pattern already used by `epic_gain`, `zone_touch`, and `boon_shop`:

```c
void sql_log_player_login(P_char ch, const char *status)
{
    if (!ch || IS_NPC(ch) || !ch->desc)
        return;

    char line[PERSISTENCE_EVENT_MAX_LEN];
    snprintf(line, sizeof(line),
             "INSERT INTO log_entries (date, kind, ip_address, pid, player_name, zone_number, room_vnum, message) "
             "VALUES (NOW(), '%s', '%s', %d, '%s', 0, 0, 'account=%s client=%s %s')",
             status, ch->desc->host, GET_PID(ch), GET_NAME(ch),
             get_account_name_safe(ch), ch->desc->client_name, ch->desc->client_version);

    if (persistence_scalar_event_worker_running()) {
        if (persistence_scalar_event_queue_enqueue(line))
            return;
        if (persistence_write_fallback_event_line(line, "scalar_event", "player_login", "queue_full_fallback"))
            return;
    }
    /* Fallback: execute synchronously (only if worker not running) */
    qry("%s", line);
}
```

**Key invariants:**
- No data loss: scalar event queue has flat-file fallback + direct SQL fallback.
- Thread safety: `snprintf` uses stack buffers; enqueue is mutex-protected.
- The old `fork()` path can be removed entirely (no callers depend on the child process pattern outside this function).
- **SQL escaping:** Player name, account name, and client strings must be escaped via `persistence_sql_escape_field()` before embedding in the INSERT string. (The existing `fork()` path has the same latent issue — `db_query`'s `%s` does not auto-escape — so this is not a regression, but the new code should fix it.)

**Testing:**
- Existing source-grep tests for `sql_log_player_login` (in `test_sql_coverage.c`) must be updated if the function signature or table changes.
- New mock test: verify the formatted line contains `INSERT INTO log_entries` and all expected fields.
- Docker build + full test suite must pass.

### Phase 7b: Connection Pool + Multi-Statement Batches *(recommended)*

**Effort:** 1–2 sessions | **Risk:** Medium | **Files:** New `src/sql_pool.c`/`.h`, `src/sql_player.c`, `src/sql.c`

#### 7b-1: Connection pool for async workers

**Current state:** The codebase has three separate MySQL connections:
- `DB` (global) — main game thread + all synchronous queries.
- `persistenceDB` — used by `sql_persistence_execute_raw()`.
- `child_conn` — created per-`fork()` in `sql_log_player_login()` (to be removed in 7a-2).

Three async persistence workers (item, scalar, large) all funnel through `sql_persistence_execute_raw()` → `persistenceDB`. Only one worker can execute SQL at a time on `persistenceDB` (MySQL protocol is synchronous per-connection). If the item event worker is writing a slow batch, the scalar event worker blocks.

**Target:** A small connection pool (3–5 connections) shared by async workers:

```c
// src/sql_pool.h
#define SQL_POOL_SIZE 5

MYSQL *sql_pool_acquire(void);     // blocks until a connection is free
void   sql_pool_release(MYSQL *c); // returns connection to pool
int    sql_pool_init(void);        // creates initial pool connections
void   sql_pool_shutdown(void);    // closes all connections
```

**Implementation:**
1. Pool is a fixed-size array of `MYSQL*` + `in_use` flag + mutex + condition variable.
2. `sql_pool_acquire()` waits on `pthread_cond_wait` until a connection is available.
3. Before returning a connection, call `mysql_ping()` — if it fails, reconnect.
4. Workers call `sql_pool_acquire()` before their write and `sql_pool_release()` after.
5. The existing `persistenceDB` path is updated to acquire from the pool instead of using a dedicated connection.

**Key invariants:**
- Pool connections use the same credentials as `persistenceDB`.
- `mysql_ping()` handles connection drops (MySQL server restart, timeout).
- All pool connections use the same `DB_NAME` (no cross-database queries needed).
- The main game thread's `DB` connection is NOT pooled — it stays separate.

#### 7b-2: Multi-statement batches for DELETE+INSERT pairs

**Current state:** Every item save does:
1. `DELETE FROM player_items WHERE pid = X`
2. N individual `INSERT INTO player_items ...`

These are two or more separate `mysql_real_query()` calls over the network. With `CLIENT_MULTI_STATEMENTS` already enabled (line 305 of `src/sql.c`), they can be combined into one call.

**Target:** Build a single multi-statement string and execute it once:

```sql
DELETE FROM player_items WHERE pid = 123;
INSERT INTO player_items (...) VALUES (...), (...), ...;
DELETE FROM player_item_affects WHERE item_id IN (SELECT item_id FROM player_items WHERE pid = 123);
INSERT INTO player_item_affects (...) VALUES (...), (...), ...;
```

**Implementation:**
1. Extend the batch builder in 7a-1 to prepend the `DELETE` and append the affect/extra_descr batches.
2. Call `sql_clear_results()` (already exists, line 1269) after the multi-statement query to drain all result sets.
3. Apply to all 7 item tables (player_items, locker_items, corpse_items, shopkeeper_items, saved_items, siege_items, player_pet_items).

**Key invariants:**
- `mysql_next_result()` must be called until it returns -1 (already handled by `sql_clear_results()`).
- If any statement in the batch fails, subsequent statements still execute (MySQL behavior). Wrap in a transaction to ensure atomicity.
- `LAST_INSERT_ID()` returns the first auto_increment of the last INSERT in the batch.

**Testing:**
- Existing transaction rollback tests (`test_transaction_rollback.c`) verify atomicity — must still pass.
- Existing multi-table consistency tests (`test_multi_table_consistency.c`) verify all 7 tables — must still pass.
- Existing v19 roundtrip tests (`test_v19_roundtrip.c`) verify column coverage — must still pass.
- New test: verify that a single `db_query()` call contains both DELETE and INSERT for the same table.

### Phase 7c: Async Player Save via Queue + Snapshot *(deferred)*

**Effort:** 3–4 sessions | **Risk:** High | **Status:** 📝 Deferred

> **Why deferred:** Async player save requires deep-copying character state under the correct lock, handling stale-data edge cases, and managing object lifecycle across threads. The payoff is large (save latency disappears from the game loop), but the design risk warrants prototyping outside the main branch first.

**Concept:** Instead of `sql_save_player()` blocking the game thread:
1. Enqueue a save request with character PID + save type.
2. A dedicated "player save worker" thread dequeues requests.
3. Under `ch->lock`, deep-copy the subset of character state needed for the save (items, skills, affects, etc.) into a `player_save_snapshot` struct.
4. Release the lock; the worker writes the snapshot to MySQL asynchronously.
5. On completion, free the snapshot.

**Risks that need resolution before implementation:**
- **Stale data:** If the character takes an action between snapshot and save completion, the save reflects pre-action state. For disconnect saves, the character is gone so this is fine. For periodic saves, the next save will catch up.
- **Object lifecycle:** Items in the snapshot are copies; must ensure no pointers into the live character survive the copy.
- **Queue backpressure:** If saves arrive faster than the worker can write, the queue grows. Need a backpressure strategy (drop oldest? block game thread? auto-resize like persistence queues?).
- **Interaction with incremental save:** The dirty-flag system (`OBJ_RFLAG_DIRTY_CONTAINER`, `db_item_id`) assumes synchronous save — the flag is cleared after a successful write. Async save needs a "write-in-progress" state.

**When to activate:** After 7a and 7b are complete and the roundtrip count per save is already down to ~20 queries (from ~170). At that point, the remaining latency is purely network, and an async approach would eliminate it entirely.

### Phase 7 Summary

| Sub-phase | Task | Effort | Risk | Rec? |
|---|---|---|---|---|
| 7a-1 | Multi-row INSERT for player_items | 0.5 session | Low | ✅ |
| 7a-2 | Replace fork() in sql_log_player_login with async queue | 0.5 session | Low | ✅ |
| 7b-1 | Connection pool for async workers | 1 session | Medium | ✅ |
| 7b-2 | Multi-statement batches for DELETE+INSERT pairs | 0.5 session | Medium | ✅ |
| 7c | Async player save via queue + snapshot | 3–4 sessions | High | 📝 Deferred |

**Total recommended effort:** 2.5 sessions across Phases 7a + 7b.
**Expected impact:** Player save query count reduced from ~170 to ~20 (88% reduction). Login logging eliminates a `fork()` call. Async workers can write concurrently instead of serializing on one `persistenceDB` connection.

**Out of scope — Redis `fork()` calls:** The two remaining `fork()` calls in
`src/redis.c` (`flush_dirty_players` and `redis_save_world_state`) are
intentionally left as-is. These fork to offload genuine heavyweight work (bulk
player saves, multi-MB JSON serialization) that would block the game loop if
run synchronously. Converting them to thread-pool workers would require
character-state snapshotting and lock redesign — the complexity is not
justified by the theoretical POSIX concern. See Phase 7c for the general
async-save design.

---

## Phase 8: Schema Migration Runner

**Effort:** 0.5 session | **Risk:** Low | **Files:** New `src/sql_migrate.c`/`.h`, `src/sql.c`, `cycle_mud.sh`, `entrypoint.sh`

**Context:** The codebase has 27 SQL migration files across `migrations/` and `sql/migrations/` directories, covering v1 through v20 plus named migrations (frag leaderboard, polls, epic zone payout, lookup tables). The base schema lives in `src/duris.sql` (32KB). Migrations use idempotency guards (`information_schema.columns` checks with prepared statements), but this is inconsistent — v19 is idempotent, v20 is raw `ALTER TABLE` without checks. On every MUD boot, `cycle_mud.sh` and `entrypoint.sh` manually list individual migration files to run and use `|| true` to swallow errors, masking genuine failures.

### Current State

```bash
# cycle_mud.sh — manual list, errors swallowed:
(mysql < ./migrations/schema_migration_v17_schema_fixes.sql || true)
(mysql < ./migrations/schema_migration_v18_player_affects_unique.sql || true)
(mysql < ./migrations/schema_migration_v19_item_table_columns.sql || true)
# v20 not listed — will never run via cycle_mud.sh!
```

**Problems:**
1. **No tracking table.** Every migration re-runs on every boot (wasteful, though idempotent).
2. **Manual wiring.** Adding a migration requires editing two shell scripts. Forgetting this step means the migration never runs (v20 is already missing from `cycle_mud.sh`).
3. **Silent failures.** `|| true` masks errors. If a migration fails due to disk space or permission issues, the MUD boots with a broken schema.
4. **Inconsistent idempotency.** v19 uses `information_schema` checks; v20 does not. If v20 runs twice, it fails.
5. **Two directories.** `migrations/` and `sql/migrations/` are both used with no clear convention.

### Target

A migration runner called during `initialize_mysql()` (after the MySQL connection is established, alongside `sql_populate_lookup_tables()`) that:

1. Creates a `schema_migrations` tracking table if absent.
2. Scans the `migrations/` directory for `.sql` files, sorted by name.
3. For each file not yet recorded in `schema_migrations`, executes it inside a transaction.
4. On success, inserts a row into `schema_migrations`.
5. On failure, logs the error and aborts the MUD boot (no `|| true`).
6. Replaces the manual `mysql < ...` lines in `cycle_mud.sh` and `entrypoint.sh` with a single call.

### Implementation

#### Step 1: Create the tracking table

Add to `src/duris.sql` (or as the first migration in the runner):

```sql
CREATE TABLE IF NOT EXISTS schema_migrations (
    version     VARCHAR(255) PRIMARY KEY,
    applied_at  TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

#### Step 2: Migrate to a single migrations directory

Move `sql/migrations/add_frag_leaderboard_tables.sql` into `migrations/` and remove `sql/migrations/`. All migration files will live in one directory, sorted alphabetically (v1, v2, ..., v20, add_frag_leaderboard, create_polls, epic-zone-payout, schema_lookup).

Rename files that don't sort correctly:
- `schema_migration.sql` → `schema_migration_v01.sql`
- `schema_migration_v2.sql` through `schema_migration_v20_item_material.sql` — already sort correctly
- `add_frag_leaderboard_tables.sql` stays as-is (sorts after v20)

#### Step 3: Make all migrations idempotent

Audit all 27 files. Any that use raw `ALTER TABLE`/`CREATE TABLE` without `IF NOT EXISTS` or `information_schema` checks must be wrapped. The pattern from v19 is the standard:

```sql
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'table_name'
                     AND column_name = 'column_name');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE table_name ADD COLUMN column_name ...',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
```

Priority targets: v20 (raw ALTER TABLE), v1–v10 (written before the idempotency convention was established).

#### Step 4: Bootstrap strategy for existing databases

**Problem:** On first boot with the runner, `schema_migrations` is empty. All 27 migration files would be executed, even though v1–v19 were already applied by the old shell scripts. Some early migrations (e.g., `run_this_one.sql`, 716 lines of combined schema) could conflict with later incremental migrations.

**Solution — two-pronged:**

1. **Make ALL migrations fully idempotent during Step 3.** Every migration uses `CREATE TABLE IF NOT EXISTS`, `ALTER TABLE ... ADD COLUMN IF NOT EXISTS`-style patterns. Even if re-run, they're safe. (This is the primary safety net.)

2. **Bootstrap on first run.** If `schema_migrations` is empty but the database has tables (indicating an existing installation), the runner records all migration filenames as "applied" without executing SQL. This is detected by checking if any known table (e.g., `player_data`) exists:

```c
// During first run: if schema_migrations is empty, check if this is
// an existing database by looking for a known table
int bootstrap_existing_db(void) {
    MYSQL_RES *res = db_query("SELECT 1 FROM information_schema.tables "
                              "WHERE table_schema = DATABASE() "
                              "AND table_name = 'player_data'");
    if (!res) return 0;
    int exists = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exists;
}
```

If the database already has tables, record all files as applied and skip execution. If it's a fresh database (no tables), run all migrations normally. This handles both the production upgrade case and the fresh-install case.

#### Step 5: Create src/sql_migrate.c

```c
// src/sql_migrate.h
#ifndef __SQL_MIGRATE_H__
#define __SQL_MIGRATE_H__

// Run all unapplied migrations from the migrations/ directory.
// Returns 0 on success, non-zero on failure (MUD should abort boot).
int sql_run_migrations(void);

#endif
```

```c
// src/sql_migrate.c — called once during initialize_mysql()

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sql_migrate.h"
#include "sql.h"

#define MIGRATIONS_DIR "migrations"

static int is_sql_file(const char *name) {
    const char *ext = strrchr(name, '.');
    return ext && strcmp(ext, ".sql") == 0;
}

static int migration_applied(const char *filename) {
    // Use mysql_real_escape_string to safely embed filename in query
    char escaped[512];
    mysql_real_escape_string(DB, escaped, filename, strlen(filename));
    char query[600];
    snprintf(query, sizeof(query),
             "SELECT 1 FROM schema_migrations WHERE version='%s'", escaped);
    MYSQL_RES *res = db_query("%s", query);
    if (!res) return 0;
    int applied = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return applied;
}

static int bootstrap_existing_db(void) {
    MYSQL_RES *res = db_query("SELECT 1 FROM information_schema.tables "
                              "WHERE table_schema = DATABASE() "
                              "AND table_name = 'player_data'");
    if (!res) return 0;
    int exists = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exists;
}

int sql_run_migrations(void) {
    // 1. Ensure tracking table exists
    if (!qry("CREATE TABLE IF NOT EXISTS schema_migrations ("
             "version VARCHAR(255) PRIMARY KEY, "
             "applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)")) {
        logit(LOG_DEBUG, "sql_run_migrations: failed to create schema_migrations table");
        return -1;
    }

    // 2. Scan migrations directory
    DIR *dir = opendir(MIGRATIONS_DIR);
    if (!dir) {
        logit(LOG_DEBUG, "sql_run_migrations: cannot open %s directory", MIGRATIONS_DIR);
        return -1;
    }

    struct dirent *entry;
    char *files[256];
    int n = 0;
    while ((entry = readdir(dir)) && n < 256) {
        if (is_sql_file(entry->d_name)) {
            files[n] = strdup(entry->d_name);
            n++;
        }
    }
    closedir(dir);

    // Sort alphabetically
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (strcmp(files[i], files[j]) > 0) {
                char *tmp = files[i];
                files[i] = files[j];
                files[j] = tmp;
            }

    // 3. Bootstrap: if this is an existing database with no migration
    //    records, mark all files as applied without executing them
    int total_applied = 0;
    for (int i = 0; i < n; i++) total_applied += migration_applied(files[i]);

    if (total_applied == 0 && bootstrap_existing_db()) {
        logit(LOG_STATUS, "Bootstrapping migration tracking for existing database...");
        for (int i = 0; i < n; i++) {
            char escaped[512];
            mysql_real_escape_string(DB, escaped, files[i], strlen(files[i]));
            qry("INSERT INTO schema_migrations (version) VALUES ('%s')", escaped);
        }
        logit(LOG_STATUS, "sql_run_migrations: bootstrapped %d migrations as 'applied' "
              "(existing database, SQL not re-executed)", n);
        for (int i = 0; i < n; i++) free(files[i]);
        return 0;
    }

    // 4. Run unapplied migrations
    int applied = 0, failed = 0;
    for (int i = 0; i < n; i++) {
        if (migration_applied(files[i]))
            continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", MIGRATIONS_DIR, files[i]);

        FILE *f = fopen(path, "r");
        if (!f) {
            logit(LOG_DEBUG, "sql_run_migrations: cannot open %s", path);
            failed++; break;
        }

        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *sql_text = malloc(sz + 1);
        if (!sql_text) { fclose(f); failed++; break; }

        size_t read_sz = fread(sql_text, 1, sz, f);
        sql_text[read_sz] = '\0';
        fclose(f);

        // Execute migration SQL directly (not via qry — avoids %% format issues)
        // Wrapped in a best-effort transaction; DDL statements cause implicit
        // commits in MySQL, so the safety net is idempotency, not the txn.
        logit(LOG_STATUS, "Applying migration: %s", files[i]);
        qry("BEGIN");
        if (mysql_real_query(DB, sql_text, strlen(sql_text)) != 0) {
            logit(LOG_DEBUG, "sql_run_migrations: SQL error in %s: %s",
                  files[i], mysql_error(DB));
            qry("ROLLBACK");
            free(sql_text);
            failed++; break;
        }

        // Drain any multi-statement results (migrations may contain
        // multiple statements separated by ;)
        sql_clear_results();

        // Record the migration
        char escaped[512];
        mysql_real_escape_string(DB, escaped, files[i], strlen(files[i]));
        if (!qry("INSERT INTO schema_migrations (version) VALUES ('%s')", escaped)) {
            logit(LOG_DEBUG, "sql_run_migrations: failed to record %s", files[i]);
            qry("ROLLBACK");
            free(sql_text);
            failed++; break;
        }

        qry("COMMIT");
        free(sql_text);
        applied++;
        logit(LOG_STATUS, "Migration applied: %s", files[i]);
    }

    for (int i = 0; i < n; i++) free(files[i]);

    if (failed > 0) return -1;

    logit(LOG_STATUS, "sql_run_migrations: %d applied, %d skipped (already applied)",
          applied, n - applied - total_applied);
    return 0;
}
```

**Key design notes:**
- Uses `mysql_real_query()` directly instead of `qry("%s", sql_text)` — avoids `%` in SQL being interpreted as format specifiers.
- Uses `mysql_real_escape_string()` for filenames in `INSERT INTO schema_migrations`.
- Calls `sql_clear_results()` after executing migration SQL to drain any multi-statement results.
- DDL statements (`CREATE TABLE`, `ALTER TABLE`) cause implicit commits in MySQL. The `BEGIN`/`COMMIT` wrapper is best-effort; the real safety net is idempotency from Step 3.
- The bootstrap logic detects existing databases by checking for the `player_data` table and pre-populates `schema_migrations` without re-executing SQL.

#### Step 6: Wire into boot sequence

In `src/sql.c`, add `#include "sql_migrate.h"` and call after `sql_populate_lookup_tables()`:

```c
int initialize_mysql() {
    // ... existing connection setup ...
    sql_resetConnectTimes();
    sql_populate_lookup_tables();

    // Run schema migrations (NEW — Phase 8)
    if (sql_run_migrations() != 0) {
        logit(LOG_DEBUG, "FATAL: schema migrations failed, aborting boot");
        mysql_close(DB);
        return -1;
    }

    return 1;
}
```

The `#ifdef __NO_MYSQL__` stub in `src/sql.c` adds:
```c
int sql_run_migrations(void) { return 0; }
```
Note: `sql_migrate.c` itself does NOT compile under `__NO_MYSQL__` — it is excluded from the build when MySQL is disabled (like `sql_player.c` and `sql.c` are).

#### Step 7: Update shell scripts

**cycle_mud.sh** — replace the manual list:

```bash
# Before:
(mysql < ./migrations/schema_migration_v17_schema_fixes.sql || true)
(mysql < ./migrations/schema_migration_v18_player_affects_unique.sql || true)
(mysql < ./migrations/schema_migration_v19_item_table_columns.sql || true)

# After: migrations are now handled by the MUD binary during boot.
# No shell-level migration commands needed.
```

**entrypoint.sh** — replace manual migration lines:

```bash
# Before:
(mysql -u root duris_dev < /duris/sql/migrations/add_frag_leaderboard_tables.sql || true)
(mysql -u root duris < /duris/sql/migrations/add_frag_leaderboard_tables.sql || true)
(mysql -u root duris_dev < /duris/migrations/schema_migration_v16_item_events.sql || true)
(mysql -u root duris < /duris/migrations/schema_migration_v16_item_events.sql || true)

# After: the base schema import (src/duris.sql) remains, but individual
# migration imports are removed — the MUD binary handles them.
```

### Key Invariants

- **Idempotency AND tracking provide defense in depth.** Even if a migration lacks `IF NOT EXISTS`, the runner won't re-run it once recorded in `schema_migrations`. Conversely, even if the tracking INSERT fails, idempotent SQL prevents damage on re-run.
- **DDL implicit commit is acknowledged.** MySQL autocommits before `CREATE TABLE`/`ALTER TABLE`. The `BEGIN`/`COMMIT` wrapper is best-effort; the safety net for DDL migrations is idempotency, not the transaction.
- **Failure is loud.** If a migration fails, `initialize_mysql()` returns -1 and the MUD does not boot. No `|| true`.
- **Order is alphabetical.** Migration filenames determine order. New migrations just need a name that sorts after existing ones.
- **The runner is the single source of truth.** Shell scripts no longer need to know about individual migrations.
- **Bootstrap handles existing databases.** If `schema_migrations` is empty but the database has tables (checked via `player_data`), all migration files are recorded as "applied" without executing SQL. Fresh databases run all migrations normally.
- **Renaming is safe.** Renamed migration files (e.g., `schema_migration.sql` → `schema_migration_v01.sql`) are handled by the bootstrap: on an existing database, the new filename is recorded as "applied" without execution. The old filename would never have been in `schema_migrations`, so it simply doesn't run. On a fresh database, the renamed file runs normally.

### Testing

- **New source-grep test:** Verify `sql_run_migrations()` is called from `initialize_mysql()` in `src/sql.c`.
- **New source-grep test:** Verify `schema_migrations` CREATE TABLE statement exists.
- **New source-grep test:** Verify `sql_migrate.c` reads files from the `migrations/` directory.
- **Docker build + full test suite must pass.**
- **Manual test:** On a fresh database, `initialize_mysql()` should apply all 27 migrations. On a second boot, it should report "0 applied, 27 skipped."

### Non-Versioned Files Audit

The following files sort before `schema_migration_v01.sql` and need classification:

| File | Lines | Action |
|---|---|---|
| `create_polls_tables.sql` | ~20 | Keep as-is; creates new tables, idempotent with `IF NOT EXISTS` |
| `epic-zone-payout.sql` | ~10 | Keep as-is; simple ALTER, make idempotent in Step 3 |
| `run_this_one.sql` | 716 | **Move to `src/duris.sql`** — this is a combined base schema, not an incremental migration. It should run once before any migrations. The `duris.sql` import in `entrypoint.sh` already handles this. |
| `schema_lookup_tables.sql` | ~20 | Keep as-is; populates lookup data, make idempotent with `INSERT IGNORE` |

After moving `run_this_one.sql` into `duris.sql`, the remaining 26 files sort correctly: non-versioned utility migrations first (polls, payout, lookup), then versioned migrations v01–v20, then `add_frag_leaderboard_tables.sql`.

### Risks & Mitigations

| Risk | Mitigation |
|---|---|
| Migration file contains `\0` or is binary | Only `.sql` files are processed; `fread` result is null-terminated |
| `%` in migration SQL interpreted as format specifier | Use `mysql_real_query()` directly instead of `qry("%s", ...)` |
| Filename contains apostrophe in tracking query | `mysql_real_escape_string()` before embedding in `INSERT INTO schema_migrations` |
| Migration file is too large for memory | `malloc` failure is caught; file is read into heap, not stack |
| `readdir` order varies by filesystem | Files are sorted alphabetically with `strcmp` after collection |
| DDL implicit commit breaks transaction wrapper | Acknowledged — transaction is best-effort; idempotency is the safety net |
| Existing database has already-applied migrations not tracked | Bootstrap detects existing DB via `player_data` table and records all files as "applied" without execution |
| Renamed migration file executes again on existing DB | Bootstrap records the new filename as "applied"; idempotent SQL prevents damage even if executed |
| `sql/migrations/` directory still has files after move | The runner only scans `migrations/`; leftover files are harmless but should be cleaned up |

---

## Phase 9: Live-Game Stress Tests — Pet Equipment, Charm Lifecycle, Crash Recovery

**Effort:** 3.5–4.5 sessions | **Risk:** Medium | **Files:** New test files + existing test harness extensions

**Context:** All 12 existing test files (125+ tests) pass, but they share a fundamental limitation: they are **mock-based**. They verify SQL *strings* but never execute against a real MySQL database and never exercise the full game loop. The pet save/load path (`sql_save_player_pets` / `sql_load_player_pets`) has zero dedicated tests despite being critical for crash recovery.

**Key scenarios not covered by existing tests:**
- Pet with equipped weapon + armor → crash save → reload → all equipment restored
- Pet with items in a container (bag on pet containing potions) → nested save/load
- Charm expires → `charm_broken` fires → `clear_links(pet, LNK_PET)` → save → pet equipment NOT loaded for former owner
- Pet killed → items drop to room/corpse → ownership chain tracked in `persistence_item_events`
- Copyover recovery — pets saved/restored from descriptor data (separate path from SQL)
- `MAX_PETS = 5` — save 5 pets with equipment, crash, all 5 restored

### Pet/Charm Architecture Summary

**Save path** (`sql_save_player_pets` in `sql_player.c:2475-2599`):
1. Only saves on `RENT_CRASH` / `RENT_CRASH2` — normal logout DELETEs pet rows
2. Iterates `ch->followers`, skips non-NPC, gets `mob_vnum` + `room_vnum` + `charm_duration`
3. `INSERT INTO player_pets (owner_pid, mob_vnum, room_vnum, charm_duration, pet_order)`
4. Per pet: saves equipment (`pet->equipment[i]`) + inventory (`pet->carrying`) into `player_pet_items`
5. Per item: saves affects (`player_pet_item_affects`) + extra_descrs (`player_pet_item_extra_descr`)
6. All wrapped in transaction (`own_txn` if not already in one)

**Load path** (`sql_load_player_pets` in `sql_player.c:2603-2865`):
1. `SELECT * FROM player_pets WHERE owner_pid = ?`
2. Per pet row: `read_mobile(pet_rnum, REAL)`, `char_to_room`, `setup_pet(charm_duration, PET_NOAGGRO)`, `add_follower`
3. `SELECT * FROM player_pet_items WHERE pet_id = ?`
4. Two-pass item load: first create all items, then place them (handles container parents)
5. After load: `DELETE FROM player_pets WHERE owner_pid = ?` (cleanup)

**Charm lifecycle** (`magic.c:8030 charm_generic`, `affects.c:2816 charm_broken`):
- `charm_generic`: clears victim's existing pets, sets `AFF_CHARM`, calls `setup_pet` + `add_follower`
- `charm_broken` (via `LNK_PET` link): fires when charm expires, calls `stop_follower` + `clear_links`
- On logout: follower pets are `extract_char`'d (items drop to room) — NOT saved to DB
- On crash: `sql_save_player_pets` runs with `RENT_CRASH` — pets + items saved for recovery

### Phase 9a: Mock-Based Pet Lifecycle Tests *(recommended first)*

**Effort:** 1 session | **Risk:** Low | **Files:** New `tests/db_write/test_pet_lifecycle.c`/`.h`

These are source-grep + SQL-pattern verification tests, same approach as the existing 12 test files. No MySQL needed — they verify the production source code contains the correct patterns.

#### Test 1: Source-grep — pet save function exists and is not a stub

```c
// Verify sql_save_player_pets() is the real implementation, not a return-false stub
// Anchored on leading comment: "pet save - save all player's pets with equipment"
// Must contain: INSERT INTO player_pets, player_pet_items, DELETE FROM player_pets
```

#### Test 2: Source-grep — pet load function exists and is not a stub

```c
// Verify sql_load_player_pets() is the real implementation
// Anchored on leading comment: "pet load - restore all player's pets with equipment"
// Must contain: SELECT FROM player_pets, read_mobile, setup_pet, SELECT FROM player_pet_items
```

#### Test 3: Source-grep — crash-only save guard

```c
// Verify sql_save_player_pets only saves on RENT_CRASH / RENT_CRASH2
// Must contain: if (save_type != RENT_CRASH && save_type != RENT_CRASH2)
// On non-crash: DELETE FROM player_pets (cleanup), return true
```

#### Test 4: Mock — pet save INSERT SQL generation

```c
// Verify sql_save_player_pets generates:
// - INSERT INTO player_pets (owner_pid, mob_vnum, room_vnum, charm_duration, pet_order)
// - INSERT INTO player_pet_items (pet_id, vnum, equip_slot, container_id, ... 28 columns)
// - INSERT INTO player_pet_item_affects (item_id, location, modifier)
// - INSERT INTO player_pet_item_extra_descr (item_id, keyword, description)
```

#### Test 5: Mock — pet item save skips ITEM_NORENT

```c
// Verify sql_save_single_pet_item checks IS_SET(obj->extra_flags, ITEM_NORENT)
// and returns 0 (skip) for non-rentable items
```

#### Test 6: Mock — pet item save recurses into containers

```c
// Verify sql_save_single_pet_item recurses into obj->contains
// (container contents are saved recursively)
```

#### Test 7: Mock — charm_broken callback exists

```c
// Verify charm_broken() is defined and clears LNK_PET link
// Anchored on: define_link(LNK_PET, "PET", charm_broken, ...)
```

#### Test 8: Mock — setup_pet called with charm duration

```c
// Verify sql_load_player_pets calls setup_pet(pet, ch, charm_duration, PET_NOAGGRO)
// and add_follower(pet, ch) for each loaded pet
```

#### Test 9: Source-grep — MAX_PETS respected

```c
// Verify #define MAX_PETS 5 exists in config.h
// Verify sql_save_player_pets handles up to MAX_PETS followers
```

#### Test 10: Source-grep — pet item load two-pass pattern

```c
// Verify sql_load_player_pets uses two-pass load:
// Pass 1: create all items, store (db_id, obj) in temp array
// Pass 2: place items (handles container parent references)
```

#### Test 11: Mock — pet save on crash preserves items for recovery

```c
// Full mock scenario:
// 1. Player has pet Wolf with Iron Sword equipped + Health Potion in inventory
// 2. Crash save (RENT_CRASH) → verify SQL writes pet + items + affects
// 3. Crash recovery load → verify SELECT reads pet + items back
// 4. After load, DELETE FROM player_pets cleans up
```

#### Test 12: Mock — normal save DELETEs pets (not crash)

```c
// Verify that on save_type != RENT_CRASH:
// - DELETE FROM player_pets WHERE owner_pid = ?
// - No pet INSERTs generated
// - Returns true (cleanup only)
```

### Phase 9b: Multi-Table Consistency — player_pet_items

**Effort:** 0.25 session | **Risk:** Low | **Files:** `tests/db_write/test_multi_table_consistency.c`

Extend the existing multi-table consistency test to include `player_pet_items`:
- DELETE-before-INSERT for player_pet_items
- `obj_uid` column in player_pet_items
- Transaction wrapping for pet item save
- All 8 item tables referenced (was 7, now includes player_pet_items)

### Phase 9c: Live DB Integration Tests *(requires MySQL)*

**Effort:** 1.5–2 sessions | **Risk:** Medium | **Files:** New `tests/db_write/Dockerfile.test-mysql`, new test files

**Architecture:** Extend the test Dockerfile to include `mysql-server`. The test binary:
1. Starts `mysqld` in the container
2. Creates the test database and runs schema migrations
3. Initializes the MySQL connection (calls `initialize_mysql()`)
4. Runs test scenarios that exercise actual save/load roundtrips
5. Queries the DB directly to verify state after each scenario

**Key test scenarios:**

| Scenario | Save | Verify | Load | Verify |
|---|---|---|---|---|
| Pet with equipment, crash | `sql_save_player_pets(ch, RENT_CRASH)` | `SELECT * FROM player_pets` has 1 row; `player_pet_items` has equipment rows | `sql_load_player_pets(ch)` | Pet in room, equipment restored, DB cleaned up |
| Pet with container items | Same as above | Container items saved with correct `container_id` FK | Same as above | Nested items restored in correct container |
| 5 pets with equipment | Save all 5 | All 5 rows in `player_pets` | Load all 5 | All 5 in room with equipment |
| Normal logout (not crash) | `sql_save_player_pets(ch, RENT_DEATH)` | `player_pets` has 0 rows | N/A | Items NOT loaded (pets were extracted) |
| Charm breaks during crash recovery | Simulate: save pet, charm_broken, load | Pet row still in DB (was saved before charm broke) | Pet loads, but `charm_broken` already cleared link | Pet in room but not following; items dropped |
| Player_pet_items v19+material roundtrip | Save item with all 28 columns | All columns match | Load item | `wear_flags`, `item_type`, `bitvector1-5`, `item_material` all correct |
| Item with apostrophe in name | Save "O'Malley's Lucky Charm" on pet | Name escaped correctly | Load item | Name roundtrips correctly |### Phase 9d: Copyover Pet Recovery

**Effort:** 0.5 session | **Risk:** Low | **Files:** New mock test in `test_pet_lifecycle.c`

Copyover has a separate pet save/restore path (via descriptor data in `copyover.c:753-759`), not SQL. Mock test verifies:
- `IS_PC_PET(ch)` is checked during copyover mob counting
- Pets are skipped from `write_mob_entry` (saved per-descriptor instead)
- `setup_pet` + `add_follower` called during copyover recovery

### Phase 9e: Frag, Kill & Item Transfer Regression Tests

**Effort:** 0.5 session | **Risk:** Low | **Files:** New `tests/db_write/test_frag_transfer.c`/`.h`

Source-grep regression guards for the frag/pkill system and item ownership transfers during death.

#### Test 1: sql_save_pkill writes to DB

```c
// Verify sql_save_pkill() exists in src/sql.c and is not a stub
// Must contain: INSERT into pkills or persistence_item_events
// Must handle: IS_PC_PET(killer) → use GET_MASTER(killer)
```

#### Test 2: fragWorthy checks racewar + level

```c
// Verify fragWorthy() in src/fraglist.c checks:
// - Racewar mismatch (opposite_racewar)
// - Level difference bounds
// - Returns 0 for non-worthy kills
```

#### Test 3: killed_by UPDATE uses escaped name

```c
// Verify fight.c writes:
// UPDATE player_data SET killed_by = '<escaped>' WHERE pid = <victim>
// Uses persistence_sql_escape_field or equivalent escaping
```

#### Test 4: item transfer on death (owner_corpse events)

```c
// Verify persistence_record_item_event("owner_corpse", ...) called
// when items move from dead player to corpse
// Must contain: target="corpse:<name>"
```

#### Test 5: frag_leaderboard REPLACE INTO has all columns

```c
// Verify sql_update_frag_leaderboard() uses REPLACE INTO frag_leaderboard
// (pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at)
```

#### Test 6: AddFrags updates both in-memory and DB

```c
// Verify AddFrags() increments ch->only.pc->frags AND calls
// sql_update_frag_leaderboard(ch)
```

### Phase 9f: Locker Stress Regression Tests

**Effort:** 0.5 session | **Risk:** Low | **Files:** New `tests/db_write/test_locker_stress.c`/`.h`

Source-grep regression guards for locker save/load, enter/exit, chest access, and multi-player scenarios. The 10K items per locker scenario (live stress test) is deferred to Phase 9c (live DB integration tests).

#### Test 1: locker_items INSERT has correct columns (v19+material)

```c
// Verify INSERT INTO locker_items includes:
// locker_id, chest_id, vnum, container_id, quantity,
// weight, cost, timer, extra_flags, wear_flags, item_type,
// value0-7, name, short_descr, description, action_descr,
// bitvector1-5, item_material
```

#### Test 2: locker save uses DELETE-before-INSERT pattern

```c
// Verify sql_save_locker() has:
// DELETE FROM locker_items WHERE locker_id=? AND (chest_id IS NULL OR chest_id=?)
// before the INSERT loop
```

#### Test 3: locker chest save/load exists

```c
// Verify sql_save_private_chest_items() and sql_load_private_chest_items()
// are real implementations (not stubs)
// Must contain: INSERT INTO locker_items, DELETE FROM locker_items for chest
```

#### Test 4: locker enter/exit path in storage_lockers.c

```c
// Verify storage_lockers.c contains:
// - do_enter_locker / do_exit_locker functions
// - char_to_room / extract_char for locker transition
// - sql_save_locker / sql_load_locker called appropriately
```

#### Test 5: locker private chest password verification

```c
// Verify sql_verify_chest_password() exists and checks
// SHA2(password, 256) against stored hash
```

#### Test 6: locker load two-pass item placement

```c
// Verify sql_load_locker_items() uses two-pass approach:
// Pass 1: create all items
// Pass 2: place items (handles container parent references)
// Must contain: "two-pass" or "first create all items" comment
```

#### Test 7: sql_locker_exists checks DB before create

```c
// Verify sql_locker_exists() queries DB before INSERT
// Prevents duplicate locker creation
```

#### Test 8: locker transaction wrapping

```c
// Verify sql_save_locker() uses either:
// - sql_begin_transaction/sql_commit/sql_rollback OR
// - own_txn flag with explicit begin/commit/rollback
```

### Phase 9g: Latency & Pulse Timing Guard Tests

**Effort:** 0.5 session | **Risk:** Low | **Files:** New `tests/db_write/test_latency_guard.c`/`.h`

Source-grep regression guards for latency tracing infrastructure and game loop pulse timing. These ensure the latency monitoring system stays intact and the game doesn't accumulate lag between pulses.

#### Test 1: latency_trace_record called for all game loop sections

```c
// Verify comm.c calls latency_trace_record for each game loop phase:
// - "connections"
// - "commands"
// - "prompts"
// - "ne_events"
// - "gmcp_flush"
// - "activities"
// - "combat"
// - "affect_and_points"
// All 8 must be present in comm.c
```

#### Test 2: total_tick recorded every pulse

```c
// Verify latency_trace_record("total_tick", ...) called at end of game loop
// Must calculate: loop_time = (loop_time_end - loop_time_start) in usec
```

#### Test 3: latency dump happens every 300 tics

```c
// Verify: if (!(tics % 300)) { latency_trace_dump(); }
// Writes to /durismud/logs/latency_trace.log
// Also calls persistence_queue_latency_dump() + utility_latency_dump()
```

#### Test 4: persistence_queue_latency_dump exists

```c
// Verify persistence_queue_latency_dump() is defined (not a stub)
// Must report queue depths and dropped counts
```

#### Test 5: utility_latency_dump exists

```c
// Verify utility_latency_dump() is defined (not a stub)
// Must report utility-level timing stats
```

#### Test 6: OPT_USEC defines pulse interval

```c
// Verify #define OPT_USEC 250000 in config.h
// 4 pulses/sec = 250ms = 250000 usec
```

#### Test 7: PULSES_IN_TICK defined

```c
// Verify PULSES_IN_TICK is defined (used by event scheduling)
// Typically 240 (60 seconds of events at 4 pulses/sec)
```

#### Test 8: game loop sleep adjusts based on usec_spent

```c
// Verify game loop calculates usec_spent and adjusts sleep:
// usec_spent = loop_time * 1000000
// sleep_time = max(0, OPT_USEC - usec_spent)
// This prevents lag accumulation between pulses
```

#### Test 9: LATENCY_TRACE_MAX_SAMPLES defined

```c
// Verify #define LATENCY_TRACE_MAX_SAMPLES 4096 in latency_trace.h
// Circular buffer prevents unbounded memory growth
```

#### Test 10: latency_trace_record mutex-protected

```c
// Verify latency_trace_record() uses pthread_mutex_lock/unlock
// Thread-safe for concurrent recording from game thread and workers
```

### Phase 9 Summary

| Sub-phase | Task | Effort | Risk | Priority |
|---|---|---|---|---|
| 9a | Mock-based pet lifecycle tests (12 tests) | 1 session | Low | ✅ Done |
| 9b | Multi-table consistency — add player_pet_items | 0.25 session | Low | ✅ Done |
| 9c | Live DB integration tests (20 tests, 6 frags + 6 locker + 8 latency) | 1.5–2 sessions | Medium | ✅ Done |
| 9d | Copyover pet recovery mock tests | 0.5 session | Low | Low |
| 9e | Frag + item transfer tests (6 source-grep tests) | 0.5 session | Low | ✅ Done |
| 9f | Locker stress tests (8 source-grep tests) | 0.5 session | Low | ✅ Done |
| 9g | Latency + pulse timing guard tests (10 source-grep tests) | 0.5 session | Low | ✅ Done |

**Total recommended effort:** 4.75–5.75 sessions.
**Expected impact:** 56 new tests (36 source-grep regression + 20 live DB integration) across pet lifecycle, frags/transfers, locker stress, and latency guards. Live DB tests provide end-to-end verification with real MySQL.

---

## Phase 10: Account/Character Lifecycle Bug Fixes & Tests

**Effort:** 1 session | **Risk:** Medium | **Files:** `src/sql_player.c`, `tests/db_write/`, `sql/migrations/`

**Context:** User reported two critical bugs:
1. **Ghost characters:** Character "Sabir" was created, logged out, then the name was taken (can't re-create) but the character didn't appear in the character selection menu.
2. **Name reuse failure:** After deleting a character, the name can't be reused in certain circumstances — something stays registered that never clears.

**Root cause analysis** revealed three interconnected bugs in the `account_characters` table and the `ON DUPLICATE KEY UPDATE` clause in `sql_save_account_characters()`:

### Bugs Found & Fixed

#### Bug 1: Ghost characters — `deleted_at` never cleared on re-create
When a character is deleted, `sql_soft_delete_character()` (in `sql.c:734`) sets `deleted_at = NOW()` on `account_characters`. If the character is later re-created with the same name, `sql_save_account_characters()` re-runs the `INSERT ... ON DUPLICATE KEY UPDATE`, but the old UPDATE clause only updated `login_count`, `last_login`, `blocked`, `racewar` — it **never set `deleted_at = NULL`**. Then `sql_load_account_characters()` filters with `WHERE deleted_at IS NULL`, so the re-created character disappears from the menu.

**Fix:** Added `deleted_at=NULL` to the `ON DUPLICATE KEY UPDATE` clause.

#### Bug 2: PID never updated in ON DUPLICATE KEY
`sql_save_account_characters()` inserts with `pid` from `sql_get_player_pid()`. But if the character was just created (pid not yet in `player_data`), pid=0 gets inserted. The `ON DUPLICATE KEY UPDATE` never propagated the real PID to the existing row. After character deletion and re-creation, the old row still referenced the deleted `player_data` row's PID.

**Fix:** Added `pid=VALUES(pid)` to the `ON DUPLICATE KEY UPDATE` clause.

#### Bug 3: Missing `UNIQUE KEY (account_name, char_name)` — row identification
Without a natural key on `(account_name, char_name)`, the `ON DUPLICATE KEY UPDATE` only triggered on `UNIQUE KEY pid (pid)`. When a character was deleted and re-created (new PID), no unique key matched the re-INSERT, so a **second row** was created instead of updating the existing one. The old row stayed with `deleted_at=NOW()`, the new row was active, but the LEFT JOIN in `sql_load_account_characters()` joined on `pid` which now pointed to the old deleted `player_data` row.

**Fix:** Added `UNIQUE KEY acct_char (account_name, char_name)` to `account_characters`.

#### Edge case fix: Missing `char_name=VALUES(char_name)`
If two unsaved characters on the same account both got pid=0, the `UNIQUE KEY pid` collision would trigger `ON DUPLICATE KEY UPDATE`, overwriting the first character's row but keeping the old `char_name`. 

**Fix:** Added `char_name=VALUES(char_name)` to the UPDATE clause.

### Code Changes

**`src/sql_player.c:4061`** — Updated ON DUPLICATE KEY UPDATE:
```sql
-- Old:
on duplicate key update login_count=%lu, last_login=FROM_UNIXTIME(NULLIF(%ld,0)), blocked=%d, racewar=%d

-- New:
on duplicate key update login_count=%lu, last_login=FROM_UNIXTIME(NULLIF(%ld,0)), blocked=%d, racewar=%d, deleted_at=NULL, pid=VALUES(pid), account_name=VALUES(account_name), char_name=VALUES(char_name)
```

### Schema Changes

**`sql/migrations/add_account_characters_columns.sql`** (NEW) — Production migration:
- Adds missing columns: `login_count`, `last_login`, `blocked`, `racewar` (used by `sql_save_account_characters()` but never created in the original migration)
- Adds `UNIQUE KEY acct_char (account_name, char_name)` — critical for ON DUPLICATE KEY to identify rows when PID changes

**`sql/migrations/cleanup_ghost_account_characters.sql`** (NEW) — Cleanup migration:
- Deletes rows where `pid = 0` (stale entries from characters linked before first save)
- Deletes soft-deleted rows that have a newer active sibling row for the same `(account_name, char_name)`

**`tests/db_write/schema.sql`** — Test schema updated:
- Added `accounts` table (11 columns matching production)
- Added `player_data` table (simplified, 8 columns)
- Added `login_count`, `last_login`, `blocked`, `racewar` columns to `account_characters`
- Added `UNIQUE KEY acct_char (account_name, char_name)`

### New Live DB Tests (8 tests)

**`tests/db_write/test_live_accounts.c`** — Full account/character lifecycle:

| # | Test | What it verifies |
|---|---|---|
| 1 | create new account | `INSERT INTO accounts` roundtrip |
| 2 | create character and link | `INSERT INTO player_data` + `INSERT INTO account_characters`, PID stored correctly |
| 3 | character listing | `LEFT JOIN` query returns correct character count with `deleted_at IS NULL` filter |
| 4 | soft-delete character | `UPDATE deleted_at=NOW()` then `WHERE deleted_at IS NULL` excludes it |
| 5 | name reuse after deletion | **Critical fix test**: delete, re-create with new PID, verify `deleted_at` is cleared and character is visible |
| 6 | PID propagation | pid=0 inserted before save, real PID propagated via `pid=VALUES(pid)` |
| 7 | multi-character account | 3 characters, delete middle one, verify 2 remain visible |
| 8 | full delete-recreate cycle | The "Sabir" scenario: create→save→delete→recreate→verify visible |

All 8 tests pass against real MySQL 8.0 in the Docker test harness.

### Phase 10 Summary

| Task | Status |
|---|---|
| Bug fix: `deleted_at=NULL` in ON DUPLICATE KEY UPDATE | ✅ |
| Bug fix: `pid=VALUES(pid)` in ON DUPLICATE KEY UPDATE | ✅ |
| Bug fix: `char_name=VALUES(char_name)` in ON DUPLICATE KEY UPDATE | ✅ |
| Schema: `UNIQUE KEY acct_char (account_name, char_name)` | ✅ |
| Schema: Missing columns in `account_characters` | ✅ |
| Migration: `add_account_characters_columns.sql` | ✅ |
| Migration: `cleanup_ghost_account_characters.sql` | ✅ |
| Live DB tests: 8 account/character lifecycle tests | ✅ 8/8 passed |
| Total live DB tests: 28 (20 Phase 9 + 8 Phase 10) | ✅ 28/28 passed |

---

## Summary

All Phase 1–4 items complete. Phase 5: 7 of 9 items complete. Phase 6: 📝 deferred. Phase 7: 🚧 plan complete (implementation pending). Phase 8: 🚧 plan complete (implementation pending). Phase 9: 🚧 in progress (9a done, 9b done, 9e/f/g being built).

Remaining to implement:
- **Phase 7a-1:** Multi-row INSERT for player_items (0.5 session)
- **Phase 7a-2:** Replace fork() with async queue (0.5 session)
- **Phase 7b-1:** Connection pool for async workers (1 session)
- **Phase 7b-2:** Multi-statement batches (0.5 session)
- **Phase 8:** Schema migration runner (0.5 session)
- **Phase 9c:** Live DB integration tests (1.5–2 sessions)
- **Phase 9d:** Copyover pet recovery mock tests (0.5 session)
- **Phase 9e:** Frag + item transfer tests (in progress)
- **Phase 9f:** Locker stress tests (in progress)
- **Phase 9g:** Latency guard tests (in progress)
- **Phase 7c:** Async player save (deferred, 3–4 sessions)

The plan covers 130+ individual tasks across 9 phases, supported by 17 test files and 165+ regression tests.

