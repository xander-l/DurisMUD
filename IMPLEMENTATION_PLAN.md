# DurisRebase: Multithreading + Database Persistence — Implementation Plan & Documentation

**Branch:** `feature/multithreading-database-persistence`  
**Base:** `master` (merge base: `5f09cbd2`)  
**Date:** June 13, 2026  
**Status:** In Progress — Phase 1 & 2 complete, Phase 3-5 planned. §7.3 tooling approach is still being drafted (scripts vs commands).

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Feature Documentation (Completed Work)](#2-feature-documentation-completed-work)
3. [Bug Fixes Documented](#3-bug-fixes-documented)
4. [Remaining Work: Item Persistence Edge Cases](#4-remaining-work-item-persistence-edge-cases)
5. [Remaining Work: Async Race Condition Audit](#5-remaining-work-async-race-condition-audit)
6. [Remaining Work: DB Conversion Completion](#6-remaining-work-db-conversion-completion)
7. [Remaining Work: Event Logging System](#7-remaining-work-event-logging-system)
8. [Remaining Work: Testing & Validation](#8-remaining-work-testing--validation)
9. [Task Prioritization & Sequence](#9-task-prioritization--sequence)

> **Note:** The developer tooling approach in §7.3 is still being drafted (whether to use scripts vs existing wiz infrastructure vs something else). The rest of this plan is ready for execution.

> **New:** Appendix C catalogs every database query in the codebase — classified as synchronous write, synchronous read, mixed read-write, converted to async, or purposefully omitted from async conversion.

---

## 1. Architecture Overview

The `feature/multithreading-database-persistence` branch introduces three major systems:

### 1.1 Async Persistence Pipeline

```
Main Game Loop (single-threaded)
    │
    ├──► persistence_item_event_queue_enqueue()    ──► Item Worker Thread
    │         ↓ (fallback on overflow)                   ↓
    │    persistence_write_fallback_event_line()    sql_persistence_execute_raw()
    │         ↓                                          ↓
    │    /durismud/logs/LOG_EVENT                  MySQL persistence_event_items
    │
    ├──► persistence_scalar_event_queue_enqueue()  ──► Scalar Worker Thread
    │                                                   ↓
    │                                              sql_persistence_execute_raw()
    │                                                   ↓
    │                                              MySQL persistence_event_scalars
    │
    └──► persistence_large_event_queue_enqueue()   ──► Large Worker Thread
                                                         ↓
                                                    sql_persistence_execute_raw()
                                                         ↓
                                                    MySQL (large payloads)
```

**Queue Types:**
| Queue | Initial Cap | Max Cap | Event Max Len | Slot Size |
|-------|------------|---------|---------------|-----------|
| Item | 4,096 | 131,072 | 1,024 | PERSISTENCE_EVENT_MAX_LEN |
| Scalar | 4,096 | 131,072 | 1,024 | PERSISTENCE_EVENT_MAX_LEN |
| Large | 64 | 2,048 | 131,072 | PERSISTENCE_LARGE_EVENT_MAX_LEN |

**Worker Lifecycle:**
- Started during `run_the_game()` in `comm.c` after `persistence_replay_fallback_events()`
- Stopped (with drain) after `game_loop()` returns
- Heartbeat watchdog checks every game pulse — auto-restarts stuck workers after 30s
- Deadlock detection: `pthread_kill(tid, 0)` checks if thread still exists

### 1.2 Database Persistence

- Second MySQL connection (`persistenceDB`) for non-blocking writes
- Mutex-guarded (`persistence_sql_mutex`)
- Schema migration system (v17) adds missing columns on every boot
- `sql_persistence_execute_raw()` in `sql_persistence_raw.c`

### 1.3 Crash Recovery System

```
Boot time:
  persistence_replay_fallback_events()
    └── Reads /durismud/logs/LOG_EVENT
        └── Replays each event into MySQL
        └── Rotates log file

Save time (writeCharacter):
  sql_save_locker() / sql_save_player()
    └── On failure: persistence_write_character_flat_fallback()
        └── Writes raw backup file
        └── Calls persistence_alert()
```

---

## 2. Feature Documentation (Completed Work)

### 2.1 New Files

| File | Purpose | Lines |
|------|---------|-------|
| `src/persistence_queue.c` | Thread-safe ring-buffer event queues (item/scalar/large) with auto-grow, worker threads, heartbeat system | 1,159 |
| `src/persistence_queue.h` | Public API: enqueue/dequeue, worker start/stop, heartbeat, SQL escaping | 102 |
| `src/latency_trace.h` | Minimal latency tracing ring buffer, scope-based macro, dump/reset | 216 |
| `src/sql_persistence_raw.c` | `sql_persistence_execute_raw()` — raw SQL on persistenceDB connection | 55 |
| `src/test_async.h` | Thin wrapper for async persistence tests | 21 |
| `migrations/schema_migration_v17_schema_fixes.sql` | Adds `item_type`, `wear_flags`, `obj_info_text` columns | 9 |
| `tests/async/test_persistence.c` | 57+ unit tests for queue, worker, fallback, game scenarios | ~2,500 |
| `tests/async/test_persistence.h` | Test declarations | ~140 |

### 2.2 Modified Files — Detailed Change Log

#### `src/files.c` (+255/- lines)
- **`writeObjectlist()`, `writeObject()`, `write_one_object()`** — Added `obj_uid` persistence via `O_F_UID` flag; calls `persistence_record_item_event()` for item saves
- **`restoreObjects()`, `read_one_object()`** — Restores `obj_uid`; spellbook corruption fix (resets `value[3]` if spell data missing)
- **`writeCharacter()`** — Added `persistence_write_character_flat_fallback()` on SQL save failure + `persistence_alert()`
- **`writeCorpse()`** — Added `persistence_alert()` on corpse save failure
- **`persistence_write_character_flat_fallback()`** — Raw file backup when SQL save fails
- **`persistence_refresh_restored_corpse()`** — Refresh decay timer on restored corpses after crash
- **`getUnsignedLongLong()`** — New parser for 64-bit unsigned integers
- New macros: `O_F_UID 128`, `ADD_ULL()`, `GET_ULL()`
- `SAV_ITEMVERS` bumped 35→36, `SAV_MAXSIZE` 240K→2MB

#### `src/sql_player.c` (+126/- lines)
- **`sql_save_single_item_get_id()`** — Changed `wear_str` from `"NULL"` to `"0"`; always generates `type_str` from `obj->type` (removed prototype conditional)
- **`sql_load_player_items()`** — Added `sql_persistence_item_owner_matches()` ownership validation; extracts items on mismatch
- **`sql_load_locker_items_filtered()`** — Ownership validation via `sql_persistence_item_owner_matches()`
- **`sql_load_private_chest_items()`** — Ownership validation
- **`sql_load_all_corpses()`** — Ownership validation; now queries `obj_uid`, `item_condition`; initializes `next_obj_uid`
- **`sql_save_corpse()`** — Added `sql_rollback()` on failure
- **`sql_delete_corpse()`** — Cascade delete: removes item affects + items before corpse
- **`sql_save_corpse_item()`** — Now accepts additional `save_id` parameter; records item events via `persistence_record_item_event()`
- **`sql_save_locker_item()`** — `wear_str` "NULL" → "0"

#### `src/utility.c` (+860/- lines)
- **Event Worker Wrappers:**
  - `persistence_start_item_event_worker()`, `persistence_stop_item_event_worker()`
  - `persistence_start_scalar_event_worker()`, `persistence_stop_scalar_event_worker()`
  - `persistence_start_large_event_worker()`, `persistence_stop_large_event_worker()`
- **Fallback System:**
  - `persistence_write_fallback_event_line()` — Thread-safe (mutex-guarded) flat-file writer
  - `persistence_replay_fallback_events()` — Reads LOG_EVENT on boot, replays into MySQL
  - `persistence_flush_item_events()`, `persistence_flush_scalar_events()`
  - Item/scalar/large log writer callbacks
- **Item UID System:**
  - `persistence_next_item_uid()` — Retrieves and increments the global UID counter
  - `persistence_assign_item_uid()` — Assigns UID to an object
  - `persistence_item_uid_text()` — Formats UID-xxx string
- **Event Recording:**
  - `persistence_record_item_event()` — Records item ownership change events
  - `persistence_clean_field()` — Sanitizes fields by replacing `|`, `\r`, `\n` with spaces
- **Monitoring:**
  - `persistence_alert()` — Centralized alerting/logging
  - `persistence_worker_heartbeat_check()` — Watchdog that detects stuck workers
- **Cross-TU Latency:**
  - `utility_latency_dump()` — Dumps this TU's latency trace to shared log
  - `utility_latency_reset()` — Resets this TU's latency buffer
- **Bug Fixes:**
  - `move_cost()` — Index bounds check on `movement_loss` lookup
  - `get_player_name_from_pid()` — NULL check on `mysql_store_result()`

#### `src/comm.c` (+43/- lines)
- **Worker Startup** (in `run_the_game()`):
  - `persistence_replay_fallback_events()`
  - `persistence_start_item_event_worker()`
  - `persistence_start_scalar_event_worker()`
  - `persistence_start_large_event_worker()`
  - Boot-time queue flood test
- **Worker Shutdown:**
  - `persistence_stop_scalar_event_worker()`
  - `persistence_stop_large_event_worker()`
  - `persistence_stop_item_event_worker()`
- **Latency Tracing:** Extensive `latency_trace_record()` calls throughout `game_loop()`

#### `src/sql.c` (+92 lines)
- **`log_epic_gain_event()`** — Epic gain logging via persistence queue (fallback to direct `qry`)
- **`sql_persistence_connection()`** — Manages `persistenceDB` connection
- **Persistence Write Wrappers:**
  - `sql_persistence_write_item_event_line()`
  - `sql_persistence_write_scalar_event_line()`
  - `sql_persistence_write_large_event_line()`
- **Stubs:**
  - `sql_persistence_item_owner_matches()` — Always returns `true` (TODO: implement actual lookup)
  - `sql_zone_touch_finished()` — No-op stub
- Added `persistenceDB` global, `persistence_sql_mutex`, `pthread.h` include

#### `src/sql.h` (+26 lines)
- DB credential helpers: `get_db_host()`, `get_db_user()`, `get_db_passwd()`, `get_db_name()`, `get_db_port()`
- Persistence layer declarations (duplicate block from another merge)
- `log_epic_gain_event()` declaration
- `sql_persistence_item_owner_matches()` declaration (×2 — pre-existing duplication)

#### `src/prototypes.h` (+36/- lines)
- Worker start/stop declarations for item, scalar, large event workers
- `persistence_flush_item_events()`, `persistence_flush_scalar_events()`
- `persistence_worker_heartbeat_check()`

#### `src/utility.h` (+3 lines)
- `utility_latency_dump()` and `utility_latency_reset()` declarations

#### `src/files.h` (+14/- lines)
- `O_F_UID 128` flag (object has persistent item identity)
- `ADD_ULL()` and `GET_ULL()` macros
- `SAV_ITEMVERS 35→36`, `SAV_MAXSIZE 240000→2000000`

#### `src/actoth.c` (+140/- lines)
- **Deferred Save System:**
  - `PERSISTENCE_DEFERRED_SAVE_SLOTS` (512 slots)
  - `find_deferred_save_slot()`, `find_empty_deferred_save_slot()`
  - `event_deferred_character_save()` — Executes saved operation on event trigger
  - `persistence_schedule_checkpoint()` — Schedules deferred save
  - `persistence_schedule_character_save()` — Schedules character save with context
  - `persistence_schedule_level_checkpoint()` — Level-up checkpoint
- **`do_save_silent()`** — Now also saves player ship if present
- **`event_autosave()`** — Added `persistence_flush_item_events(64)` before silent save

#### `src/boon.c` (+321/- lines)
- Redis caching for boon data with serialize/deserialize helpers
- `boon_was_recently_processed()` — 2-second deduplication ring buffer
- Read-through cache in `get_boon_data()`
- New includes: `persistence_queue.h`, `redis.h`

#### `src/epic.c` (+78/- lines)
- `gain_epic()` — Triggers `log_epic_gain_event()` for persistence
- `epic_stone()` — Replaced direct `db_query()` with `sql_zone_touch_finished()`; deferred alignment updates
- `apply_pending_epic_zone_completions()` — Processes deferred alignment deltas
- `epic_zone_done()` — Grace period increased 15min→30min
- New include: `persistence_queue.h`

#### `src/epic.h` (+3/- lines)
- New declarations for epic zone completion logic

#### `src/tradeskill.c` (+24/- lines)
- Multiple `do_save_silent()` → `persistence_schedule_character_save()` replacements in epic store and learn_tradeskill

#### `src/limits.c` (+4 lines)
- Added `#include "actobj.h"` for deferred save integration

#### `src/magic.c` (+29/- lines)
- Spell/item interactions integration

#### `src/necromancy.c` (+10/- lines)
- Integration with persistence event recording

#### `src/specs.*.c` (multiple files)
- Minor persistence integration in mobile specs, Ioun stones, Lohrr artifacts, Undermountain, Winterhaven

#### `src/statistics.c` (+7 lines)
- Integration with persistence event recording

#### `src/test_async.c` (+15 lines)
- Test runner integration

#### `src/actobj.c` (+2/- lines)
- Item-related persistence hook

#### Infrastructure Files
- `Dockerfile` (+105 lines) — Multi-stage build, dependency installation
- `entrypoint.sh` (+91 lines) — Startup logic, migration routing
- `cycle_mud.sh` (+4 lines) — Migration execution routing
- `.gitignore` (+6 lines) — Build artifacts, IDE files
- `src/Makefile` (+7 lines) — Build targets for persistence modules

### 2.3 Database Schema Changes (v17 Migration)

```sql
ALTER TABLE corpse_items ADD COLUMN item_type INT NOT NULL DEFAULT 0;
ALTER TABLE player_items ADD COLUMN item_type INT NOT NULL DEFAULT 0;
ALTER TABLE player_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0;
ALTER TABLE auctions ADD COLUMN obj_info_text TEXT NULL;
```

**Rationale:** The `item_type` and `wear_flags` columns were expected by the async persistence code but were missing from the base schema. Without them, `INSERT INTO player_items` would fail with MySQL error 1048 (column cannot be null).

### 2.4 Event Formats

#### Item Events (PERSISTENCE_ITEM_EVENT)
```
PERSISTENCE_ITEM_EVENT|ts=<unix_time>|event=<action>|item_uid=<UID>|vnum=<vnum>|item=<name>|actor=<name>|actor_id=<pid>|source=<location>|target=<location>|note=<detail>
```

#### Scalar Events (PERSISTENCE_SCALAR_EVENT)
```
PERSISTENCE_SCALAR_EVENT|ts=<unix_time>|event=<action>|<key=value pairs...>
```

Used for: player kills, boon collection, epic gain, auction bids, zone alignment, player level-up

#### SQL Execution
All events ultimately become SQL INSERT statements executed via `sql_persistence_execute_raw()`:
```sql
INSERT INTO persistence_event_items (ts, event, item_uid, vnum, ...) VALUES (...)
INSERT INTO persistence_event_scalars (ts, event, pid, ...) VALUES (...)
```

---

## 3. Bug Fixes Documented

### 3.1 Critical: NULL item_type/wear_flags causing save failure (commit `595f8432`)
**Symptom:** Player rents/re-enters, "couldn't find character", equipment disappears. Intermittent.
**Root Cause:** `sql_save_single_item_get_id()` in `sql_player.c` sent `NULL` strings for `item_type` and `wear_flags` when items matched their prototype. MySQL columns are `NOT NULL DEFAULT 0`, causing error 1048 and save rollback.
**Fix:** 
- `strcpy(wear_str, "NULL")` → `strcpy(wear_str, "0")` 
- `type_str` now always sends `obj->type` value (removed conditional checking proto match)
**Files:** `src/sql_player.c`

### 3.2 Spellbook Corruption on Restore
**Symptom:** Spellbook items loaded with corrupted `value[3]` (used pages count).
**Fix:** Added check in `restoreObjects()` — if `ITEM_SPELLBOOK` has spell data missing, reset `obj->value[3]` to 0.
**Files:** `src/files.c`

### 3.3 move_cost Out-of-Bounds Access
**Symptom:** Potential crash looking up `movement_loss` with bad terrain index.
**Fix:** Added bounds check before array lookup.
**Files:** `src/utility.c`

### 3.4 mysql_store_result NULL Dereference
**Symptom:** Potential crash in `get_player_name_from_pid()`.
**Fix:** Added NULL check on `mysql_store_result()` return.
**Files:** `src/utility.c`

### 3.5 Build Failures After Persistence Refactor (commit `6d0a5125`)
**Symptom:** `make dms_new` failed with 5 undefined symbols.
**Root Cause:** HEAD commit `789d4b07` called functions never implemented.
**Fix:** Added stubs and declarations:
- `persistence_sql_escape_field()` — Full implementation in `persistence_queue.c`
- `utility_latency_dump()` / `utility_latency_reset()` — Declarations in `utility.h` (impl in `utility.c` from HEAD)
- `persistence_start_large_event_worker()` / `persistence_stop_large_event_worker()` — Declarations in `prototypes.h`
- `sql_persistence_item_owner_matches()` — Safe stub returning `true` in `sql.c`
- `sql_zone_touch_finished()` — No-op stub in `sql.c`
**Files:** `src/persistence_queue.h`, `src/persistence_queue.c`, `src/utility.h`, `src/prototypes.h`, `src/sql.c`

---

## 4. Remaining Work: Item Persistence Edge Cases

### 4.1 Audit: All Item Transition Paths

The following item movement scenarios need explicit ownership event recording:

| Scenario | Event Recorded? | Call Site | Risk |
|----------|----------------|-----------|------|
| Player picks up item from ground | ⚠️ CHECK | `actobj.c` | Item could duplicate if save fails mid-transfer |
| Player drops item | ⚠️ CHECK | `actobj.c` | Item ownership not updated until save |
| Player gives item to another player | ⚠️ CHECK | Trade system | Race condition: two players saving simultaneously |
| Item transferred to container/backpack | ⚠️ CHECK | Container code | Nested ownership chain not tracked |
| Item moved to locker | ✅ Yes | `sql_player.c` | Ownership validated on load |
| Item moved from locker to inventory | ⚠️ CHECK | Locker code | Transition event may be missing |
| Item sold to shop | ⚠️ CHECK | Shop code | Ownership transfer to shop NPC |
| Item bought from shop | ⚠️ CHECK | Shop code | New ownership not immediately persisted |
| Item from auction | ⚠️ CHECK | Auction code | Auction handoff between players |
| Corpse looted | ⚠️ CHECK | Corpse looting | Item transfers from corpse to player |
| Item enchanted/modified | ⚠️ CHECK | Magic/enchant code | Item modified without save event |
| Item destroyed (sacrifice, etc.) | ⚠️ CHECK | Various | No event recorded for deletion |

### 4.2 Action Items

1. **Audit all item movement paths** — Tag every call site where an item changes ownership, location, or is destroyed. Ensure `persistence_record_item_event()` is called.
2. **Implement `sql_persistence_item_owner_matches()`** — Replace the `return true` stub with actual DB lookup against `persistence_event_items` table to validate ownership chain.
3. **Container/backpack hierarchy** — Ensure nested items (items inside containers inside lockers) have their ownership chain properly recorded and validated.
4. **Trade atomicity** — Verify both sides of a player-to-player trade are persisted before either player saves.
5. **Item resurrection tool** — Create an external script (not a new MUD command) that can:
   - Look up an item by VNUM in the persistence event log
   - Determine its last known owner and location
   - Re-create the item if it was accidentally lost (via SQL, not via in-game command)

### 4.3 `sql_persistence_item_owner_matches()` Implementation Plan

```c
// Current stub (src/sql.c):
bool sql_persistence_item_owner_matches(unsigned long long item_uid,
    const char *owner_type, const char *owner_ref, const char *context)
{
    (void)item_uid; (void)owner_type; (void)owner_ref; (void)context;
    return true;  // TODO: Implement actual lookup
}
```

**Required implementation:**
1. Query `persistence_event_items` table for the most recent event for `item_uid`
2. Compare the event's `target` field against `owner_ref`
3. If no events exist (new item), return `true`
4. If the last event shows the item was destroyed, return `false`
5. If the last event shows the item belongs to a different owner, log a conflict event and return `false`
6. If the last event matches, return `true`

---

## 5. Remaining Work: Async Race Condition Audit

### 5.1 Critical Race Conditions to Analyze

#### RC-1: Queue Growth During Shutdown
**Scenario:** Main thread calls `persistence_*_worker_stop(1)` (drain mode), but game events are still being enqueued.
**Risk:** Items enqueued after worker exits the drain loop but before `pthread_join` completes are lost.
**Mitigation:** The drain flag + queue count check loop should handle this, but verify with stress tests.

#### RC-2: Writer Callback Thread Safety
**Scenario:** Worker thread calls `sql_persistence_execute_raw()` which locks `persistence_sql_mutex`. If MySQL blocks, worker blocks.
**Risk:** Worker blocked on mutex is not "stuck" (heartbeat advances). No data loss, but latency builds.
**Mitigation:** Acceptable — the queue auto-grows to handle bursts.

#### RC-3: Fallback File Concurrent Writes
**Scenario:** Multiple producers (game loop events + failed worker writes) writing to the same LOG_EVENT file.
**Risk:** Interleaved writes corrupting event lines.
**Mitigation:** `persistence_fallback_log_mutex` in `utility.c` — verify it covers ALL call sites.

#### RC-4: Deferred Save vs Immediate Save
**Scenario:** `persistence_schedule_character_save()` schedules a save for 5 ticks later. Player disconnects before tick fires.
**Risk:** Character changes lost.
**Mitigation:** Verify that `do_quit` / disconnect path flushes pending saves.

#### RC-5: obj_uid Counter Race
**Scenario:** Two game events simultaneously call `persistence_next_item_uid()`.
**Risk:** Duplicate UIDs.
**Mitigation:** Verify this is only called from the main game thread (no concurrent access). If called from worker threads, it needs a mutex.

#### RC-6: Heartbeat False Positives
**Scenario:** Worker thread is busy writing a large batch but hasn't hit the mutex-unlock point where heartbeat updates.
**Risk:** Watchdog incorrectly restarts a healthy worker.
**Mitigation:** 30-second threshold should be generous enough. Verify with batch size tests.

### 5.2 Action Items

1. **Verify all lock acquisition order is consistent** — Ensure no ABBA deadlock potential between `persistence_*_queue_mutex` and `persistence_sql_mutex`
2. **Add thread sanitizer (TSan) build** — Compile with `-fsanitize=thread` and run soak tests
3. **Stress test the overflow path** — Fill all three queues past max capacity simultaneously while workers are failing
4. **Verify disconnect flush** — Ensure all scheduled saves are executed when player disconnects
5. **Document the locking hierarchy** — Create a `THREAD_SAFETY.md` documenting which mutexes exist and their ordering

---

## 6. Remaining Work: DB Conversion Completion

### 6.1 Stub Functions Needing Real Implementation

| Function | Current State | Required | Priority |
|----------|--------------|----------|----------|
| `sql_persistence_item_owner_matches()` | Returns `true` | DB lookup | **HIGH** |
| `sql_zone_touch_finished()` | No-op | INSERT into zone_touches | MEDIUM |
| `sql_persistence_execute_raw()` | Implemented (MySQL) | Consider retry on deadlock | LOW |

### 6.2 Fallback Recovery Audit

**Current flow:**
```
Boot: persistence_replay_fallback_events()
  → Reads /durismud/logs/LOG_EVENT
  → For each line: calls sql_persistence_execute_raw()
  → On success: removes line from file
  → On failure: keeps line for next boot
  → Rotates file
```

**Gaps:**
1. **Crash during replay** — If MUD crashes during replay, already-replayed events could be replayed again. Need idempotency (INSERT IGNORE or ON DUPLICATE KEY).
2. **Character flat-fallback** — `persistence_write_character_flat_fallback()` writes raw files but there's no automatic replay of these on boot. These need manual admin intervention.
3. **Corpse flat-fallback** — Corpse save failures are alerted but not re-saved. Corpses may vanish on crash.

### 6.3 Action Items

1. **Implement `sql_persistence_item_owner_matches()`** — See §4.3
2. **Implement `sql_zone_touch_finished()`** — INSERT into zone_touches table with all parameters
3. **Add idempotency to fallback replay** — Use INSERT IGNORE for event replay
4. **Automatic flat-fallback replay** — On boot, scan for flat-fallback character files and attempt to repersist them
5. **Add `mysql_ping()` health check** — Before each `sql_persistence_execute_raw()`, verify the connection is alive; reconnect if dead
6. **DB connection pool resilience** — Add retry logic (with backoff) for transient MySQL errors (1205 lock wait timeout, 1213 deadlock)

### 6.4 Remaining Schema Work

- Verify `persistence_event_items` and `persistence_event_scalars` tables exist in production schema
- Add indexes for `item_uid` lookups (needed for ownership validation)
- Consider partitioning for large event tables

---

## 7. Remaining Work: Event Logging System

### 7.1 Current State

The infrastructure exists but is not universally instrumented:
- `persistence_record_item_event()` — Called from `files.c` (write_one_object), `sql_player.c` (corpse item save)
- Scalar events — Logged for player kills, boons, epic gain, auction bids, etc.
- Test file has scenario tests but not all are wired in production

### 7.2 Missing Event Instrumentation

| Game Event | Instrumented? | Where to Add |
|------------|--------------|--------------|
| Item created (crafting, quest reward) | ❌ | Tradeskill, quest code |
| Item destroyed (sacrifice, trash, decay) | ❌ | actobj.c, decay system |
| Item transferred between players | ❌ | Trade/give code |
| Item moved container→inventory | ❌ | Container code |
| Item enchanted/reforged | ❌ | Magic/enchant code |
| Player death (full inventory save) | ❌ | Death code |
| Locker item insert/remove | ✅ | sql_player.c |
| Corpse item looted | ❌ | Corpse looting code |
| Auction item listed/sold | ⚠️ Partial | Auction code |
| Ship item transfer | ❌ | Ship code |

### 7.3 Item Ownership Conflict Detection

**Goal:** Ability to answer these questions from logs:
1. "Who currently owns item with VNUM X?"
2. "Was item VNUM X ever duplicated?"
3. "When was item VNUM X last seen, and where did it go?"
4. "What items did player Y lose in the last 24 hours?"

**Action Items:**
1. **Create `item_audit_trail` view/table** — Materialized view joining persistence_event_items with player_items for fast lookup
2. **Instrument ALL item lifecycle events** — See table in §7.2
3. **Add developer tooling (scripts, not new command paths):**
   - A shell/Python script that queries `persistence_event_items` by VNUM to show item lifecycle
   - A shell/Python script that queries current owner of an item by VNUM
   - A shell/Python script that lists items owned by a player from event log
   - These should run externally (docker exec) or via an existing `do_wiz` infrastructure hook — do NOT create new MUD command paths
4. **Add duplication detection** — Periodic sweep comparing `persistence_event_items` against `player_items` for orphaned/duplicate items

### 7.4 Event Log Design

```sql
CREATE TABLE IF NOT EXISTS persistence_event_items (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    ts BIGINT NOT NULL,
    event VARCHAR(64) NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    item_name VARCHAR(256),
    actor_name VARCHAR(64),
    actor_id INT,
    source_location VARCHAR(64),
    target_location VARCHAR(64),
    note VARCHAR(256),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_item_uid (item_uid),
    INDEX idx_ts (ts),
    INDEX idx_vnum (vnum),
    INDEX idx_actor (actor_id)
);

CREATE TABLE IF NOT EXISTS persistence_event_scalars (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    ts BIGINT NOT NULL,
    event VARCHAR(64) NOT NULL,
    payload TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_ts (ts),
    INDEX idx_event (event)
);
```

---

## 8. Remaining Work: Testing & Validation

### 8.0 ⚠️ CRITICAL: Character Save Reliability Stress Test (GATE for all further work)

**Background:** The original symptom (player rents/re-enters, "couldn't find character", equipment disappears) was caused by NULL `item_type`/`wear_flags` triggering MySQL error 1048 and save rollback. Commit `595f8432` fixed the NULL values, but the async nature of the save pipeline means there could be other failure modes:
- Worker thread falling behind (queue backlog)
- `persistence_sql_mutex` contention causing timeouts
- Deferred saves not flushing before disconnect
- Flat-fallback files accumulating (indicating SQL failures in the background)

**This must be validated before any further code changes.**

#### 8.0.1 Immediate Verification (can be done now)

1. **Check current running build for save errors:**
   ```bash
   docker exec durismud bash -c "grep -iE 'error 1048|cannot be null|item_type|wear_flags|sql_save.*fail|writeCharacter.*fail|character not found' /duris/logs/log/debug | tail -50"
   ```
2. **Check fallback log size — if it's growing, saves are still failing silently:**
   ```bash
   docker exec durismud bash -c "wc -l /durismud/logs/LOG_EVENT 2>/dev/null; ls -la /durismud/logs/LOG_EVENT* 2>/dev/null"
   ```
3. **Check worker stats — are workers keeping up?**
   ```bash
   # Check status log for worker heartbeat/dropped stats
   docker exec durismud bash -c "grep -iE 'persistence.*(written|dropped|stuck|restart|resize)' /duris/logs/log/status | tail -20"
   ```
4. **Run the async persistence unit tests:**
   ```bash
   docker exec durismud bash -c "cd /duris/tests/async && make clean && make test 2>&1"
   ```

#### 8.0.2 Save Stress Test Plan

| # | Test | Method | Pass Criteria |
|---|------|--------|---------------|
| S1 | Single-player rapid save/load | Script: login, modify inventory, save, quit, reload — repeat 100x | Zero "couldn't find character", zero lost items |
| S2 | Multi-player concurrent saves | Multiple telnet sessions saving simultaneously every tick for 10 min | All characters load correctly, no item loss |
| S3 | Save during MySQL pause | Pause MySQL container mid-save, resume, verify fallback replay | Characters load intact after MySQL resumes |
| S4 | Crash during save | `kill -9` the MUD process mid-save cycle, reboot, verify | Fallback events replayed, characters intact, no item duplication |
| S5 | Locker stress | Fill locker to capacity (save), empty locker (save), repeat 20x | All items accounted for, no orphaned items |
| S6 | Deferred save disconnect | Trigger save, disconnect immediately before tick fires | Save completes before disconnect, or flushes on next login |

### 8.1 Redesigned Unit Test Architecture

**Problem with current tests:**
- ~36 of 57 tests are mechanically identical copies across 3 queue types (item/scalar/large)
- Zero tests connect to MySQL — the entire DB write path is untested
- Zero tests exercise the production save pipeline (`writeCharacter`, `sql_save_player_items`, `sql_save_locker`, `do_save_silent`)
- Fallback tests use a stubbed `/tmp/` writer, not the real `persistence_write_fallback_event_line()`
- Game scenario tests verify only event string format, not actual game state changes
- No tests for `persistence_sql_escape_field()`, `persistence_schedule_character_save()`, or crash recovery

**Redesigned architecture — 7 layers, each targeting a specific production path:**

---

#### Layer 1: Core Queue Tests (consolidate, keep)

**File:** `tests/async/test_persistence.c` (refactored)  
**Prerequisites:** None (no MySQL, no MUD state)  
**Approach:** One generic parameterized test runner operates on all three queue types via a `queue_ops` struct (same pattern as the existing `worker_ops`). Each logical test is written once and instantiated 3×.

| # | Test | What It Proves | Queue Types |
|---|------|---------------|-------------|
| Q1 | `test_queue_enqueue_dequeue` | Basic put/get cycle, pending count correctness | item, scalar, large |
| Q2 | `test_queue_wraparound` | Ring buffer wraps correctly (fill, drain half, refill) | item, scalar, large |
| Q3 | `test_queue_overflow_autogrow` | Queue auto-grows past initial capacity, drops when max reached | item, scalar, large |
| Q4 | `test_queue_order_preserved` | FIFO order maintained through fill/drain cycles | item, scalar, large |
| Q5 | `test_queue_reset` | Reset clears pending/dropped; queue usable after reset | item, scalar, large |
| Q6 | `test_queue_dropped_counter` | Dropped counter increments on overflow, clears correctly | item only |
| Q7 | `test_queue_large_payload` | 100KB payload survives enqueue/dequeue intact | large only |
| Q8 | `test_queue_dequeue_truncation` | Small output buffer truncates correctly, null-terminated | item only |
| Q9 | `test_queue_dequeue_empty` | Dequeue from empty returns 0 (not error) | item only |

**Removed (redundant):** Per-type wraparound/pending/overflow/order/reset tests — these are identical across types, handled by parameterization.

---

#### Layer 2: Worker Lifecycle Tests (keep, already well-structured)

**File:** `tests/async/test_persistence.c`  
**Prerequisites:** None (no MySQL — writer callback is a test stub)  
**Approach:** Uses existing `worker_ops` struct pattern. Each test runs on all three queue types (item, scalar, large).

| # | Test | What It Proves |
|---|------|---------------|
| W1 | `test_worker_start_stop` | Worker starts, `running()` returns true, stops cleanly |
| W2 | `test_worker_flush_basic` | Enqueue 5 events, start worker, stop-with-drain, verify all written |
| W3 | `test_worker_flush_order` | Last event received matches last enqueued (FIFO through worker) |
| W4 | `test_worker_flush_empty` | Start/stop on empty queue — no crash, no spurious writes |
| W5 | `test_worker_flush_failure` | Writer fails first 3 calls, worker retries, succeeds on 4th |
| W6 | `test_worker_stuck_not_running` | `stuck()` returns -1 when worker never started |
| W7 | `test_worker_stuck_fresh_heartbeat` | Fresh heartbeat → not stuck |
| W8 | `test_worker_stuck_stale_heartbeat` | Stale heartbeat (60s ago, threshold 30s) → stuck detected |
| W9 | `test_worker_overflow_drain` | Fill queue past max while worker failing, then drain all |
| W10 | `test_worker_large_payload_flush` | 50KB payload passes through large-event worker |

**Removed (redundant):** Per-type copies of start_stop, flush_basic, flush_order, flush_empty, flush_failure — already handled by `worker_ops` parameterization; keep only the large-payload variant which has unique logic.

---

#### Layer 3: SQL Escaping Tests (NEW)

**File:** `tests/async/test_persistence.c`  
**Prerequisites:** None (pure string function)  
**Targets:** `persistence_sql_escape_field()` in `persistence_queue.c`

| # | Test | Input | Expected Output |
|---|------|-------|----------------|
| E1 | `test_escape_no_special_chars` | `"Longsword+2"` | `"Longsword+2"` |
| E2 | `test_escape_apostrophe` | `"Dragon's Scale"` | `"Dragon''s Scale"` (doubled) |
| E3 | `test_escape_backslash` | `"path\\to\\item"` | `"path\\\\to\\\\item"` (doubled) |
| E4 | `test_escape_pipe` | `"x|y"` | `"x y"` (pipe→space) |
| E5 | `test_escape_newline` | `"line1\nline2"` | `"line1 line2"` (LF→space) |
| E6 | `test_escape_carriage_return` | `"line1\rline2"` | `"line1 line2"` (CR→space) |
| E7 | `test_escape_mixed` | `"O'Brien's|sword\n+1"` | `"O''Brien''s sword +1"` |
| E8 | `test_escape_null_input` | `NULL` | `"none"` |
| E9 | `test_escape_empty_string` | `""` | `""` |
| E10 | `test_escape_null_buffer` | buf=NULL | `""` (returns empty) |
| E11 | `test_escape_zero_buf_size` | buf_size=0 | `""` (returns empty) |
| E12 | `test_escape_buffer_near_limit` | 950-char string in 1024-byte buf | Truncated at buf_size-1, null-terminated |

---

#### Layer 4: Database Integration Tests (NEW — critical gap)

**File:** `tests/db_write/test_db_write.c` (expand existing)  
**Prerequisites:** MySQL connection (`DB_HOST`, `DB_USER`, `DB_PASSWD`, `DB_NAME` env vars)  
**Targets:** `sql_persistence_execute_raw()`, `sql_persistence_connection()`, `sql_persistence_write_item_event_line()`, `sql_persistence_write_scalar_event_line()`, `sql_persistence_write_large_event_line()`

**IMPORTANT:** These tests use a dedicated test database or table suffix (`_test`) to avoid polluting production data. All tables created in `SETUP` and dropped in `TEARDOWN`.

| # | Test | What It Proves |
|---|------|---------------|
| D1 | `test_db_connection_acquire` | `sql_persistence_connection()` returns non-NULL MYSQL* |
| D2 | `test_db_connection_reconnect` | After `mysql_close()`, next call reconnects |
| D3 | `test_db_execute_raw_insert` | `sql_persistence_execute_raw("INSERT INTO test_table ...")` returns true |
| D4 | `test_db_execute_raw_select` | INSERT then SELECT via `db_query()` — data matches |
| D5 | `test_db_execute_raw_bad_sql` | Malformed SQL returns false, does not crash |
| D6 | `test_db_execute_raw_null` | NULL/empty SQL returns false, does not crash |
| D7 | `test_db_write_item_event_end_to_end` | `sql_persistence_write_item_event_line()` → query via `db_query()` → event matches |
| D8 | `test_db_write_scalar_event_end_to_end` | `sql_persistence_write_scalar_event_line()` → query → event matches |
| D9 | `test_db_write_large_event_end_to_end` | `sql_persistence_write_large_event_line()` with 10KB payload → query → payload intact |
| D10 | `test_db_worker_writes_to_db` | Start item worker with `sql_persistence_write_item_event_line` writer, enqueue 5 events, stop-with-drain, verify all 5 rows in DB |
| D11 | `test_db_worker_writes_to_db_scalar` | Same as D10 but for scalar worker |
| D12 | `test_db_mutex_serialization` | Two rapid-fire `sql_persistence_execute_raw()` calls from main thread — both succeed (mutex prevents collision) |

---

#### Layer 5: Save Pipeline Tests (NEW — critical gap)

**File:** `tests/db_write/test_db_write.c` (expand)  
**Prerequisites:** MySQL + minimal MUD state (a dummy `P_char` with inventory)  
**Targets:** `sql_save_player_items()`, `sql_save_locker()`, `sql_save_locker_item()`, `sql_load_player_items()`, `sql_save_single_item_get_id()`

| # | Test | What It Proves |
|---|------|---------------|
| P1 | `test_save_load_player_items_roundtrip` | Create item, `sql_save_player_items()`, DELETE from memory, `sql_load_player_items()` — item restored with matching fields |
| P2 | `test_save_player_items_wear_flags` | Save item with non-zero wear_flags, load back — wear_flags match (proves NULL→"0" fix works) |
| P3 | `test_save_player_items_item_type` | Save item with type, load back — item_type matches (proves type_str fix works) |
| P4 | `test_save_locker_item` | `sql_save_locker_item()` with known item, query `locker_items` table — row exists with correct fields |
| P5 | `test_save_locker_full_cycle` | `sql_save_locker()` for player with 10 items, verify 10 rows in locker_items, `sql_load_locker_items_filtered()` loads all 10 |
| P6 | `test_save_single_item_get_id_no_null_columns` | Call `sql_save_single_item_get_id()` for an item matching its prototype — verify wear_flags column is "0" not NULL, item_type is the actual type value |
| P7 | `test_save_player_items_empty_inventory` | Save player with zero items — returns success, no stray rows |
| P8 | `test_load_player_items_no_rows` | Load items for player with no saved items — returns success, inventory unchanged |

---

#### Layer 6: Crash Recovery Tests (NEW)

**File:** `tests/db_write/test_db_write.c` (expand)  
**Prerequisites:** MySQL + filesystem access to `/durismud/logs/`  
**Targets:** `persistence_write_fallback_event_line()`, `persistence_replay_fallback_events()`, `persistence_write_character_flat_fallback()`

| # | Test | What It Proves |
|---|------|---------------|
| C1 | `test_fallback_write_to_real_log` | `persistence_write_fallback_event_line("test_event", ...)` → file `/durismud/logs/LOG_EVENT` exists, contains "test_event" |
| C2 | `test_fallback_write_null_line` | NULL line returns 0, does not crash, does not write |
| C3 | `test_fallback_replay_events` | Write 3 events to LOG_EVENT, call `persistence_replay_fallback_events()`, verify all 3 appear in MySQL |
| C4 | `test_fallback_replay_idempotent` | Replay same LOG_EVENT twice — no duplicate rows in MySQL (INSERT IGNORE) |
| C5 | `test_fallback_replay_empty_file` | Replay on empty/missing LOG_EVENT — returns success, no crash |
| C6 | `test_flat_fallback_writes_file` | `persistence_write_character_flat_fallback(ch, "locker")` → flat file created with character data |
| C7 | `test_flat_fallback_replay_on_boot` | Write flat fallback file, simulate boot path → character data recovered |

---

#### Layer 7: Item Lifecycle Integration Tests (NEW)

**File:** `tests/db_write/test_db_write.c` (expand)  
**Prerequisites:** MySQL + minimal MUD state (dummy characters with inventories)  
**Targets:** Full save/load cycle across ownership transitions

| # | Test | What It Proves |
|---|------|---------------|
| L1 | `test_item_create_equip_save_load_verify` | Create item → equip → save → destroy inventory → load → item restored with correct wear position |
| L2 | `test_item_transfer_between_players` | Player A has item → save A → delete from A → load into B → save B → load A and B → item only in B's inventory |
| L3 | `test_item_container_hierarchy` | Item in backpack → backpack in locker → save → destroy all → load → item inside backpack inside locker restored |
| L4 | `test_item_destroy_event_recorded` | Create item → save → destroy item → `persistence_record_item_event("destroyed", ...)` → query events table → destruction event exists |
| L5 | `test_item_duplication_detected` | Save item to player A → manually INSERT same item into player B in DB → `sql_persistence_item_owner_matches()` returns false for B → item extracted from B on load |
| L6 | `test_deferred_save_schedule_and_flush` | `persistence_schedule_character_save(ch, 1, 1, "test")` → wait 2 ticks → character saved in DB |
| L7 | `test_deferred_save_disconnect_flush` | Schedule save → simulate disconnect before tick → save still fires (or flushes on next login) |

---

### 8.2 Test File Layout (Proposed)

```
tests/
├── async/
│   ├── Makefile                          # Builds standalone test binary (no MySQL)
│   ├── test_persistence.h                # Layer 1+2 declarations
│   ├── test_persistence.c                # Layer 1: Queue tests (parameterized)
│   ├── test_worker.c                     # Layer 2: Worker lifecycle tests (parameterized)
│   └── test_escape.c                     # Layer 3: SQL escape tests
│
├── db_write/
│   ├── Makefile                          # Builds MySQL-dependent test binary
│   ├── test_db_write.h                   # Layer 4-7 declarations
│   ├── test_db_integration.c             # Layer 4: DB connection + raw SQL
│   ├── test_save_pipeline.c              # Layer 5: Save/load roundtrips
│   ├── test_crash_recovery.c             # Layer 6: Fallback + replay
│   └── test_item_lifecycle.c             # Layer 7: Item lifecycle scenarios
│
└── stress/
    ├── stress_save_rapid.sh              # S1: Single-player rapid save/load (100x)
    ├── stress_save_concurrent.sh         # S2: Multi-player concurrent saves
    ├── stress_crash_recovery.sh          # S4: Crash-during-save recovery
    └── stress_save_deferred.sh           # S6: Deferred save disconnect
```

### 8.3 Integration Testing Strategy

1. **Soak test:** Run MUD for 4+ hours with many players online, verify:
   - No dropped events exceeding threshold
   - No worker restarts (unless forced)
   - No memory leaks (valgrind)
   - Flat fallback file stays empty (all events go through MySQL)

2. **Crash recovery test:**
   - Kill -9 the MUD process mid-save
   - Boot again, verify fallback events replayed
   - Verify player saves are intact
   - Verify items are not duplicated

3. **Item lifecycle integration test:**
   - Create item → equip → unequip → drop → another player picks up → put in locker → take out → sell to shop
   - Verify complete event chain recorded
   - Verify ownership validation passes at each step

### 8.4 Action Items

1. **Refactor existing tests** — Consolidate Layer 1+2 using parameterized runners; remove per-type duplication (~36→12 tests)
2. **Add Layer 3: SQL escape tests** — 12 cases covering all special characters and edge cases
3. **Add Layer 4: DB integration tests** — 12 cases hitting real MySQL via the persistence connection
4. **Add Layer 5: Save pipeline tests** — 8 cases exercising `sql_save_player_items`, `sql_save_locker`, `sql_load_player_items`
5. **Add Layer 6: Crash recovery tests** — 7 cases for fallback write, replay, idempotency, flat-fallback
6. **Add Layer 7: Item lifecycle tests** — 7 cases for full ownership transitions through DB
7. **Create stress test scripts** — Shell scripts for rapid save/load, concurrent saves, crash recovery
8. **Add valgrind/memcheck to CI** — Catch memory leaks in queue growth/reset paths

---

## 9. Task Prioritization & Sequence

> **GATE CHECK:** Before ANY Phase 3+ work begins, the §8.0 Save Stress Test must pass. If the current build still has intermittent save failures, all effort goes into fixing that before anything else.

### Phase 0: Save Reliability Validation (GATE — Must Pass First)

| # | Task | Section | Priority | Est. Effort |
|---|------|---------|----------|-------------|
| 0.1 | Run immediate verification checks (error logs, fallback file, worker stats) | §8.0.1 | **CRITICAL** | Small |
| 0.2 | Run async persistence unit tests in container | §8.0.1 | **CRITICAL** | Small |
| 0.3 | Single-player rapid save/load stress test (100x) | §8.0.2 S1 | **CRITICAL** | Medium |
| 0.4 | Multi-player concurrent save stress test | §8.0.2 S2 | **CRITICAL** | Medium |
| 0.5 | Crash-during-save recovery test | §8.0.2 S4 | **CRITICAL** | Medium |
| 0.6 | Locker capacity stress test | §8.0.2 S5 | **HIGH** | Small |
| 0.7 | Deferred save disconnect flush test | §8.0.2 S6 | **HIGH** | Small |

**Gate decision:** If tests S1-S5 all pass → proceed to Phase 3. If any fail → fix the save pipeline until they pass, then re-test.

### Phase 3: Production Hardening (Pending Gate)

| # | Task | Section | Priority | Est. Effort |
|---|------|---------|----------|-------------|
| 3.1 | Implement `sql_persistence_item_owner_matches()` | §4.3, §6.1 | **CRITICAL** | Large |
| 3.2 | Audit all item transition paths for event recording | §4.1 | **HIGH** | Medium |
| 3.3 | Add `persistence_sql_escape_field` unit tests | §8.1 | MEDIUM | Small |
| 3.4 | Verify disconnect path flushes deferred saves | §5.1 RC-4 | **HIGH** | Small |
| 3.5 | Add idempotency to fallback event replay | §6.2 | **HIGH** | Small |
| 3.6 | Implement `sql_zone_touch_finished()` | §6.1 | MEDIUM | Small |
| 3.7 | Document locking hierarchy (THREAD_SAFETY.md) | §5.2 | MEDIUM | Small |

### Phase 4: Event Logging System

| # | Task | Section | Priority | Est. Effort |
|---|------|---------|----------|-------------|
| 4.1 | Instrument all missing item lifecycle events | §7.2 | **HIGH** | Large |
| 4.2 | Create `item_audit_trail` for fast ownership lookup | §7.3 | MEDIUM | Medium |
| 4.3 | Add developer tooling scripts (shell/Python, external to MUD) for item trace/owner/player-items queries | §7.3 | MEDIUM | Medium |
| 4.4 | Add duplication detection sweep | §7.3 | LOW | Small |
| 4.5 | Create event log schema (if tables don't exist) | §7.4 | **HIGH** | Small |
| 4.6 | Add automatic flat-fallback replay on boot | §6.3 | MEDIUM | Medium |

### Phase 5: Test Suite Redesign & Execution

| # | Task | Section | Priority | Est. Effort |
|---|------|---------|----------|-------------|
| 5.1 | Refactor Layer 1+2: consolidate queue/worker tests with parameterized runners, remove ~36 redundant copies | §8.1 L1-L2 | **HIGH** | Medium |
| 5.2 | Add Layer 3: SQL escape unit tests (12 cases) | §8.1 L3 | **HIGH** | Small |
| 5.3 | Add Layer 4: DB integration tests (12 cases) — requires MySQL connection | §8.1 L4 | **CRITICAL** | Large |
| 5.4 | Add Layer 5: Save pipeline tests (8 cases) — `sql_save_player_items`, `sql_save_locker`, `sql_load_player_items` roundtrips | §8.1 L5 | **CRITICAL** | Large |
| 5.5 | Add Layer 6: Crash recovery tests (7 cases) — fallback write, replay, idempotency, flat-fallback | §8.1 L6 | **HIGH** | Medium |
| 5.6 | Add Layer 7: Item lifecycle tests (7 cases) — full ownership transitions through DB | §8.1 L7 | **HIGH** | Large |
| 5.7 | Create stress test scripts (rapid save/load 100x, concurrent saves, crash recovery) | §8.0.2 | **CRITICAL** | Medium |
| 5.8 | TSan build and soak test (4+ hours) | §5.2, §8.3 | MEDIUM | Medium |
| 5.9 | Valgrind leak check on queue growth/reset paths | §8.4 | LOW | Small |
| 5.10 | Update this document with test results | — | **HIGH** | Small |

### Phase 6: Production Deployment

| # | Task | Priority |
|---|------|----------|
| 6.1 | Schema migration for event log tables | **HIGH** |
| 6.2 | Deploy to staging, run soak test | **HIGH** |
| 6.3 | Monitor worker stats (written, failed, dropped) for 1 week | **HIGH** |
| 6.4 | Deploy to production | MEDIUM |

---

## Appendix A: File Change Summary (Branch vs Master)

| File | Δ Lines | Type |
|------|---------|------|
| `src/utility.c` | +860/- | Persistence implementation |
| `src/boon.c` | +321/- | Redis caching + dedup |
| `src/files.c` | +255/- | obj_uid, flat fallback, crash recovery |
| `src/latency_trace.h` | +216 | **NEW** — Latency tracing |
| `src/persistence_queue.c` | +1,159 | **NEW** — Queue/worker engine |
| `src/persistence_queue.h` | +102 | **NEW** — Queue API |
| `src/actoth.c` | +140/- | Deferred save system |
| `src/sql_player.c` | +126/- | Item ownership, NULL fix |
| `Dockerfile` | +105 | Build system |
| `src/sql.c` | +92 | Persistence DB connection |
| `entrypoint.sh` | +91 | Startup routing |
| `src/epic.c` | +78/- | Zone touch, epic logging |
| `src/sql_persistence_raw.c` | +55 | **NEW** — Raw SQL execution |
| `src/comm.c` | +43/- | Worker lifecycle |
| `src/prototypes.h` | +36/- | Worker declarations |
| `src/specs.ioun.c` | +32/- | Persistence integration |
| `src/magic.c` | +29/- | Spell persistence |
| `src/sql.h` | +26 | DB helpers, persistence decls |
| `src/tradeskill.c` | +24/- | Deferred saves |
| `src/test_async.h` | +21 | **NEW** — Test wrapper |
| `src/test_async.c` | +15 | Test runner |
| `src/files.h` | +14/- | O_F_UID, ADD_ULL, GET_ULL |
| `src/necromancy.c` | +10/- | Persistence integration |
| `src/specs.mobile.c` | +10/- | Persistence integration |
| `migrations/schema_migration_v17_schema_fixes.sql` | +9 | **NEW** — Schema fix |
| `src/specs.lohrr.c` | +8/- | Persistence integration |
| `src/statistics.c` | +7 | Persistence integration |
| `src/Makefile` | +7/- | Build targets |
| `.gitignore` | +6/- | Build artifacts |
| `src/limits.c` | +4 | Deferred save include |
| `cycle_mud.sh` | +4 | Migration routing |
| `src/utility.h` | +3 | Latency declarations |
| `src/epic.h` | +3/- | Epic declarations |
| `src/actobj.c` | +2/- | Persistence hook |
| `src/specs.winterhaven.c` | +2/- | Persistence integration |
| `src/specs.undermountain.c` | +31/- | Persistence integration |

**Total: 36 files, +3,763 / -183 lines**

---

## Appendix B: Git Commit History (Branch-specific)

```
6d0a5125 fix: add missing function stubs and declarations to fix build after persistence refactor
595f8432 fix: prevent NULL item_type/wear_flags in player_items INSERT causing save failure
cf1c9cfa fix: route Docker startup through cycle_mud.sh for proper migration execution
1c9ed4e2 revert: remove v17 migration from entrypoint.sh - migration runs via cycle_mud.sh
5aa6db49 fix: run schema migration v17 on every boot in entrypoint.sh, add player_items.item_type
67b95319 feat: add schema migration v17 for missing columns, runs on every boot
789d4b07 feat: persistence worker heartbeat auto-restart, LOG_EVENT rotation, container rescue in extract_obj
5f09cbd2 (master) Merge pull request #132 from kilobyte/groups
```

---

## Appendix C: Complete Database Query Catalog

This appendix catalogs every SQL query in the codebase, classified by:
- **SYNC-W** = Synchronous Write (blocking, main thread)
- **SYNC-R** = Synchronous Read (blocking, main thread)
- **SYNC-RW** = Synchronous Mixed Read+Write (blocking, main thread, single function does both)
- **ASYNC** = Converted to Async (via persistence queue → worker thread → `persistenceDB`)
- **OMITTED** = Purposefully omitted from async conversion (with reason)
- **STUB** = Function exists but is a no-op/stub, not yet implemented

### C.1 Player Character Save Pipeline

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `writeCharacter()` | files.c | Calls `sql_save_locker()` + `sql_save_player()` | **OMITTED** | Must complete before player quits/crashes. Flat-fallback is the safety net, not async. |
| `sql_save_player()` | sql_player.c | Calls sub-functions below | **OMITTED** | Authoritative save entry point. Must be synchronous so game knows save completed. |
| `sql_save_player_status()` | sql_player.c | `REPLACE INTO player_data (...)` | **SYNC-W** | Part of synchronous save pipeline. Must complete before `writeCharacter()` returns. |
| `sql_save_player_skills()` | sql_player.c | `DELETE FROM player_skills WHERE pid=X` then `INSERT INTO player_skills (...)` batch | **SYNC-RW** | Delete-then-insert pattern. Must complete atomically as part of save. |
| `sql_save_player_affects()` | sql_player.c | `DELETE FROM player_affects WHERE pid=X` then `INSERT INTO player_affects (...)` batch | **SYNC-RW** | Same delete-then-insert pattern. Part of save pipeline. |
| `sql_save_player_items()` | sql_player.c | `DELETE FROM player_items WHERE pid=X` then `INSERT INTO player_items (...)` batch + item affects + keywords | **SYNC-RW** | Most complex save sub-function. Contains `sql_save_single_item_get_id()` for each item. |
| `sql_save_single_item_get_id()` | sql_player.c | `INSERT INTO player_items (...)` per item | **SYNC-W** | Called per-item within `sql_save_player_items()`. **This was the source of bug §3.1** (NULL item_type/wear_flags). |
| `sql_save_player_witnesses()` | sql_player.c | `DELETE FROM player_witnesses WHERE pid=X` then `INSERT INTO player_witnesses (...)` | **SYNC-RW** | Part of save pipeline. |
| `sql_save_player_shapechanges()` | sql_player.c | Batch INSERT into shapechange tables | **SYNC-W** | Part of save pipeline. |
| `sql_save_player_recipes()` | sql_player.c | Batch INSERT into recipe tables | **SYNC-W** | Part of save pipeline. |
| `sql_save_player_pets()` | sql_player.c | `DELETE FROM player_pets WHERE pid=X` then `INSERT INTO player_pets (...)` | **SYNC-RW** | Part of save pipeline. |
| `sql_save_player_languages()` | sql_player.c | `DELETE FROM player_languages WHERE pid=X` then `INSERT INTO player_languages (...)` | **SYNC-RW** | Part of save pipeline. |
| `sql_save_player_intros()` | sql_player.c | `DELETE FROM player_intros WHERE pid=X` then `INSERT INTO player_intros (...)` | **SYNC-RW** | Part of save pipeline. |
| `sql_save_player_timers()` | sql_player.c | `DELETE FROM player_timers WHERE pid=X` then `INSERT INTO player_timers (...)` | **SYNC-RW** | Part of save pipeline. |
| `sql_save_player_undead_slots()` | sql_player.c | `DELETE FROM player_undead_slots WHERE pid=X` then `INSERT INTO player_undead_slots (...)` | **SYNC-RW** | Part of save pipeline. |
| `sql_save_player_forged_items()` | sql_player.c | `DELETE FROM player_forged_items WHERE pid=X` then `INSERT INTO player_forged_items (...)` | **SYNC-RW** | Part of save pipeline. |
| `sql_save_player_granted_cmds()` | sql_player.c | `DELETE FROM player_granted_cmds WHERE pid=X` then `INSERT INTO player_granted_cmds (...)` | **SYNC-RW** | Part of save pipeline. |
| `sql_save_player_core()` | sql_player.c | Basic player status save | **SYNC-W** | Quick save (called from nanny.c on login). Low latency, no need to defer. |

### C.2 Player Character Load Pipeline

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `sql_load_player_items()` | sql_player.c | `SELECT ... FROM player_items JOIN ...` + `SELECT ... FROM player_item_affects` + extra_desc/keyword selects | **SYNC-RW** | Must complete before player can act. Reads items then marks them as equipped in memory. Calls `sql_persistence_item_owner_matches()` stub. |
| `sql_load_locker_items_filtered()` | sql_player.c | `SELECT ... FROM locker_items WHERE ...` + recursive container loads + affect/extra_desc selects | **SYNC-R** | Called during player load to populate locker inventory. Recursive for nested containers. |
| `sql_load_private_chest_items()` | sql_player.c | `SELECT ... FROM private_chest_items WHERE ...` | **SYNC-R** | Loads private chest contents. |
| `sql_load_all_corpses()` | sql_player.c | `SELECT ... FROM corpses JOIN corpse_items ...` + item loading | **SYNC-R** | Loads all corpses on boot. Now queries `obj_uid` and `item_condition`. |
| `sql_load_player_items()` (top-level) | nanny.c | Called after player login | **SYNC-R** | Must complete before player enters game world. |

### C.3 Locker Save Pipeline

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `sql_save_locker()` | sql_player.c | `DELETE FROM locker_items WHERE ...` then per-item `INSERT INTO locker_items (...)` + recursive containers | **SYNC-RW** | Part of `writeCharacter()`. Must complete before save returns. |
| `sql_save_locker_item()` | sql_player.c | `INSERT INTO locker_items (...)` per item | **SYNC-W** | Called per item by `sql_save_locker()`. Fixed NULL→"0" for wear_flags. |

### C.4 Corpse Save/Load

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `sql_save_corpse()` | sql_player.c | `INSERT INTO corpses (...)` + `sql_save_corpse_item()` per item | **SYNC-W** | Now has `sql_rollback()` on failure. Part of `writeCorpse()`. |
| `sql_save_corpse_item()` | sql_player.c | `INSERT INTO corpse_items (...)` per item | **SYNC-W** | Now records `persistence_record_item_event("owner_corpse", ...)`. |
| `sql_delete_corpse()` | sql_player.c | `DELETE FROM corpse_item_affects` → `DELETE FROM corpse_items` → `DELETE FROM corpses` | **SYNC-W** | Cascade delete. Now removes affects + items before corpse row. |

### C.5 Frags / Leaderboard

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `sql_modify_frags()` | sql.c | `SELECT total_frags FROM frag_leaderboard` → `UPDATE frag_leaderboard SET total_frags=X` or `REPLACE INTO frag_leaderboard (...)` | **SYNC-RW** | Frag gain/loss during combat. Low latency read+write. |
| `sql_update_frag_leaderboard()` | sql.c | `SELECT SUM(total_frags) FROM frag_leaderboard` then `REPLACE INTO frag_leaderboard (...)` | **SYNC-RW** | Periodic update. |
| `db_query("UPDATE player_data SET killed_by=...")` | fight.c | Direct UPDATE | **SYNC-W** | Sets killer name on death. |
| `qry("SELECT event_id FROM pkill_info...")` | fight.c | Direct SELECT | **SYNC-R** | PKill event lookups. |

### C.6 Deferred Save System

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `persistence_schedule_character_save()` | actoth.c | Schedules `do_save_silent()` via MUD event system | **OMITTED** | Not a DB query - delays the save by N ticks but the save itself is still synchronous. The scheduling is main-thread event system. |
| `persistence_schedule_level_checkpoint()` | actoth.c | Same as above, marks as level checkpoint | **OMITTED** | Same pattern - delayed synchronous save. |
| `do_save_silent()` | actoth.c | Calls `writeCharacter()` which is synchronous | **OMITTED** | Silent save entry point. Now saves player ship too. |
| `event_autosave()` | actoth.c | Calls `do_save_silent()` + `persistence_flush_item_events(64)` | **OMITTED** | Autosave every 1200 pulses. Flushes async item events before synchronous save. |

### C.7 Auctions

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Auction list | auction_houses.c | `SELECT ... FROM auctions WHERE status=OPEN ...` | **SYNC-R** | Player browsing auctions. Must be synchronous (player waiting). |
| Auction search | auction_houses.c | `SELECT ... FROM auctions WHERE ... LIKE '%keyword%'` | **SYNC-R** | Player searching. Must be synchronous. |
| Auction bid/check | auction_houses.c | `SELECT ... FROM auctions WHERE id=X` → validation → `UPDATE auctions SET cur_price=X, winning_bidder=Y` | **SYNC-RW** | Must be synchronous - validates bid amount, checks funds, updates atomically. |
| Auction create | auction_houses.c | `INSERT INTO auctions (...)` | **SYNC-W** | Player creating auction. |
| Auction close/cancel | auction_houses.c | `UPDATE auctions SET status=REMOVED/COMPLETED` | **SYNC-W** | Close auction, transfer item. |
| Auction backfill (obj_info_text) | auction_houses.c | `SELECT ... WHERE obj_info_text IS NULL` → `UPDATE auctions SET obj_info_text=X` | **SYNC-RW** | Schema migration backfill. One-time or periodic. |
| Auction keyword update | auction_houses.c | `UPDATE auctions SET id_keywords=X` | **SYNC-W** | Keyword index update. |

### C.8 Boons

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Boon creation | boon.c | `INSERT INTO boons (...)` → `SELECT MAX(id) FROM boons` | **SYNC-RW** | Admin creates boon. Infrequent. |
| Boon load | boon.c | `SELECT ... FROM boons WHERE id=X` | **SYNC-R** | Load boon data. |
| Boon progress check | boon.c | `SELECT ... FROM boons_progress WHERE boonid=X AND pid=Y` | **SYNC-R** | Check if player completed boon. |
| Boon progress update | boon.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Converted to async! Boon progress events go through scalar queue. |
| Boon shop load | boon.c | `SELECT ... FROM boons_shop WHERE pid=X` | **SYNC-R** | Load player's boon shop data. |
| Boon shop stats update | boon.c | `qry("UPDATE boons_shop SET stats=X WHERE pid=Y")` | **SYNC-W** | Direct synchronous write. **NOT converted to async** (uses qry not persistence queue). |
| Boon shop insert | boon.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Converted to async. |
| Boon remove | boon.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Converted to async. |
| Boon deactivate | boon.c | `qry("UPDATE boons SET active=0, duration=0 WHERE id=X")` | **SYNC-W** | Direct synchronous. Must take effect immediately. |
| Boon dedup | boon.c | `boon_was_recently_processed()` - ring buffer | **N/A** | In-memory only, not a DB query. |

### C.9 Epic / Zones

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Zone touch (old path) | epic.c | `db_query("UPDATE zones SET last_touch=NOW() WHERE number=X")` | **SYNC-W** | **Still in code** - called alongside `sql_zone_touch_finished()` which is a no-op stub. |
| Zone alignment read | epic.c | `qry("SELECT alignment FROM zones WHERE number=X")` | **SYNC-R** | Read current alignment. |
| Zone alignment update | epic.c | `qry("UPDATE zones SET alignment = alignment + (X) WHERE number=Y")` | **SYNC-W** | Direct synchronous. Must be atomic. |
| Zone alignment event | epic.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Converted to async. Zone alignment events logged via scalar queue. |
| Epic gain (sync) | epic.c | `log_epic_gain(pid, type, data, amount)` → `qry("INSERT INTO epic_gain (...)")` | **SYNC-W** | Direct synchronous INSERT. |
| Epic gain event (async) | epic.c/sql.c | `log_epic_gain_event(...)` → `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Converted to async. Epic gain events go through scalar queue. |
| Epic zone done check | epic.c | `qry("SELECT type_id FROM epic_gain WHERE pid=X AND type=Y")` | **SYNC-R** | Read epic completion status. |
| Zone frequency mod | epic.c | `qry("UPDATE zones SET frequency_mod = ...")` | **SYNC-W** | Zone frequency adjustments. |
| Epic bonus | epic_bonus.c | `SELECT`, `INSERT`, `UPDATE` on epic_bonus table | **SYNC-RW** | Player epic bonus tracking. |
| `sql_zone_touch_finished()` | sql.c | No-op stub | **STUB** | Not implemented. INSERT into zone_touches pending. |

### C.10 Artifacts

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Artifact load | artifact.c | `qry("SELECT vnum, locType, location, owned, timer FROM artifacts WHERE type=X")` | **SYNC-R** | Load all artifacts of a type. |
| Artifact check owned | artifact.c | `qry("SELECT owned, timer FROM artifacts WHERE vnum=X")` | **SYNC-R** | Check if artifact is owned. |
| Artifact insert | artifact.c | `qry("INSERT INTO artifacts (vnum, owned, locType, location, timer, type) VALUES(...)")` | **SYNC-W** | Create artifact. |
| Artifact update | artifact.c | `qry("UPDATE artifacts SET owned=X, locType=Y, location=Z, timer=... WHERE vnum=W")` | **SYNC-W** | Update artifact state. |
| Artifact delete | artifact.c | `qry("DELETE FROM artifacts WHERE vnum=X")` | **SYNC-W** | Remove artifact. |
| Artifact mortal sync | artifact.c | `mysql_real_query(DB, ...)` for batch sync | **SYNC-W** | Bulk artifact sync operation. |
| Artifact poof | artifact.c | `SELECT` → `UPDATE`/`INSERT` | **SYNC-RW** | Artifact poof logic (timed respawn). |

### C.11 Associations / Guilds / Outposts

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Association load | assocs.c | `qry("SELECT id, name FROM associations WHERE active=1")` | **SYNC-R** | Load active associations. |
| Association create/update | assocs.c | `qry("REPLACE INTO associations (...)")` / `qry("UPDATE associations SET name=...")` | **SYNC-W** | Association management. |
| Guild transactions | assocs.c | `qry("INSERT INTO guild_transactions (...)")` | **SYNC-W** | Log guild transaction. |
| Guildhall load | guildhall_db.c | `qry("SELECT ... FROM guildhalls")` / `qry("SELECT ... FROM guildhall_rooms")` | **SYNC-R** | Load guildhall data. |
| Guildhall save | guildhall_db.c | `qry("REPLACE INTO guildhalls (...)")` / `qry("REPLACE INTO guildhall_rooms (...)")` | **SYNC-W** | Save guildhall state. |
| Guildhall delete | guildhall_db.c | `qry("DELETE FROM guildhalls WHERE id=X")` / `qry("DELETE FROM guildhall_rooms WHERE id=X")` | **SYNC-W** | Remove guildhall. |
| Outpost load | outposts.c | `qry("SELECT ... FROM outposts")` | **SYNC-R** | Load outpost state. |
| Outpost update | outposts.c | `qry("UPDATE outposts SET ...")` / `db_query("UPDATE outposts SET ...")` | **SYNC-W** | Update outpost HP, golems, archers, etc. |
| Association resources | outposts.c | `qry("SELECT wood, stone FROM associations WHERE id=X")` / `db_query("UPDATE associations SET wood=X, stone=Y")` | **SYNC-RW** | Resource management. |
| Building ownership | buildings.c | `db_query("UPDATE outposts SET owner_id=X WHERE id=Y")` | **SYNC-W** | Building ownership transfer. |

### C.12 Locker Access

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Locker access check | storage_lockers.c | `qry("SELECT visitor FROM locker_access WHERE owner=X")` | **SYNC-R** | Check who has access. |
| Locker access add | storage_lockers.c | `qry("INSERT INTO locker_access (owner, visitor) VALUES (...)")` | **SYNC-W** | Grant access. |
| Locker access remove | storage_lockers.c | `qry("DELETE FROM locker_access WHERE owner=X AND visitor=Y")` | **SYNC-W** | Revoke access. |
| Remove all access | storage_lockers.c | `qry("DELETE FROM locker_access WHERE visitor=X")` | **SYNC-W** | Cleanup on character delete. |
| Private chest password | storage_lockers.c | `qry("UPDATE private_chests SET password_hash=X WHERE id=Y")` | **SYNC-W** | Set/change chest password. |
| Private chest item load | storage_lockers.c | `db_query(...)` to load chest items | **SYNC-R** | Load chest contents. |

### C.13 Nexus Stones

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Stone list | nexus_stones.c | `qry("SELECT id, name, room_vnum, align FROM nexus_stones")` | **SYNC-R** | Load all stones. |
| Stone detail | nexus_stones.c | `qry("SELECT ... FROM nexus_stones WHERE id=X")` | **SYNC-R** | Load stone details. |
| Stone alignment update | nexus_stones.c | `qry("UPDATE nexus_stones SET align=X, last_touched_at=NOW() WHERE id=Y")` | **SYNC-W** | Alignment change. Must take effect immediately. |
| Stone reset | nexus_stones.c | `qry("UPDATE nexus_stones SET align=0, last_touched_at=NULL WHERE id=X")` | **SYNC-W** | Reset stone on timeout. |
| Stone bonus check | nexus_stones.c | `qry("SELECT align, stat_affect, affect_amount FROM nexus_stones WHERE ...")` | **SYNC-R** | Check bonuses. |

### C.14 Polls

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Poll list | poll.c | `db_query("SELECT ... FROM polls WHERE ...")` | **SYNC-R** | List polls. |
| Poll options | poll.c | `db_query("SELECT ... FROM poll_options WHERE poll_id=X")` | **SYNC-R** | Load options. |
| Poll vote check | poll.c | `db_query("SELECT id FROM poll_votes WHERE poll_id=X AND account_name=Y")` | **SYNC-R** | Check if already voted. |
| Poll vote cast | poll.c | `qry("INSERT IGNORE INTO poll_votes (...)")` | **SYNC-W** | Cast vote. |
| Poll create | poll.c | `qry("INSERT INTO polls (...)")` + `qry("INSERT INTO poll_options (...)")` | **SYNC-RW** | Create poll with options. |
| Poll close | poll.c | `qry("UPDATE polls SET is_active=0 WHERE id=X")` | **SYNC-W** | Close poll. |
| Poll expire | poll.c | `qry("UPDATE polls SET is_active=0 WHERE expires_at < ...")` | **SYNC-W** | Auto-expire old polls. |

### C.15 Statistics

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Statistics insert | statistics.c | `qry("INSERT INTO statistics (...)")` | **SYNC-W** | Periodic statistics logging. Low frequency. |

### C.16 IP / Connection Logging

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `sql_connectIP()` | sql.c | `INSERT` or `UPDATE ip_info` | **SYNC-W** | Log connection. Low latency, no reason to defer. |
| `sql_disconnectIP()` | sql.c | `UPDATE ip_info` with disconnect time | **SYNC-W** | Log disconnection. |
| `sql_select_IP_info()` | sql.c | `SELECT ... FROM ip_info WHERE pid=X` | **SYNC-R** | Read IP history. Called from actwiz.c. |

### C.17 Shop / Quest Trophies

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `sql_shop_sell()` | sql.c | `INSERT INTO shop_trophy (...)` | **SYNC-W** | Log shop sale. Low stakes, could be async. |
| `sql_shop_trophy()` | sql.c | `SELECT COUNT(*) FROM shop_trophy WHERE ...` | **SYNC-R** | Check shop trophy count. |
| `sql_quest_finish()` | sql.c | Not implemented (returns -1) | **STUB** | Placeholder. |
| `sql_quest_trophy()` | sql.c | `SELECT COUNT(*) FROM quest_trophy WHERE ...` | **SYNC-R** | Check quest trophy count. |
| Quest trophy insert | sql.c | `qry("INSERT INTO quest_trophy (...)")` | **SYNC-W** | Log quest completion. |

### C.18 Offline Messages

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Send offline message | sql.c | `qry("INSERT INTO offline_messages (...)")` | **SYNC-W** | Send message to offline player. Low stakes, could be async. |
| Load offline messages | sql.c | `qry("SELECT ... FROM offline_messages WHERE pid=X")` | **SYNC-R** | Load messages on login. Must be synchronous. |
| Delete offline message | sql.c | `qry("DELETE FROM offline_messages WHERE id=X")` | **SYNC-W** | Delete after delivery. |

### C.19 Zone Trophy / Touches

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Zone trophy load | trophy.c | `qry("SELECT zone_number, exp FROM zone_trophy JOIN zones WHERE ...")` | **SYNC-R** | Load trophy data. |
| Zone trophy update | trophy.c | `qry("UPDATE zone_trophy SET exp = ...")` / `qry("DELETE FROM zone_trophy WHERE exp <= 0")` | **SYNC-RW** | Update trophy exp. |
| Zone trophy insert | trophy.c | `qry("INSERT INTO zone_trophy (pid, zone_number, exp) VALUES (...)")` | **SYNC-W** | Add trophy entry. |
| Zone trophy clear | specs.room.c | `qry("DELETE FROM zone_trophy WHERE pid=X")` | **SYNC-W** | Clear player trophies. |
| Zone touches clear | sql.c | `sql_clear_zone_trophy()` → `qry("DELETE FROM zone_trophy")` + `qry("DELETE FROM zone_touches")` | **SYNC-W** | Admin cleanup. |

### C.20 PWipe / Character Delete

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `sql_pwipe()` | sql.c | `qry("DELETE FROM zone_trophy")` + `qry("DELETE FROM zone_touches")` → `qry("UPDATE player_data SET active=0")` | **SYNC-W** | Complete player wipe. Admin-only. Must be synchronous (destructive). |
| `sql_soft_delete_character()` | sql.c | `UPDATE player_data SET active=0 WHERE pid=X` → `UPDATE account_characters SET deleted_at=NOW()` | **SYNC-W** | Soft-delete character. Must be synchronous. |
| Character hard delete | sql_player.c | `DELETE FROM player_data WHERE pid=X` | **SYNC-W** | Hard delete by name. |

### C.21 Web Info / Wiki

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| `sql_webinfo_toggle()` | sql.c | Toggle web info visibility | **SYNC-W** | Player toggle. |
| Wiki search | wikihelp.c | `qry("SELECT title FROM pages WHERE title LIKE '%...%'")` | **SYNC-R** | Wiki search. |
| Wiki page load | wikihelp.c | `qry("SELECT title, text, ... FROM pages WHERE title=X")` | **SYNC-R** | Load wiki page. |
| `get_mud_info()` | sql.c | Web statistics queries | **SYNC-R** | Web-facing info. |

### C.22 Frag List / Leaderboard Display

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Frag list | fraglist.c | `db_query("SELECT ... FROM frag_leaderboard WHERE ...")` | **SYNC-R** | Display leaderboard. Player-facing. |
| Frag totals | fraglist.c | `db_query("SELECT SUM(total_frags) FROM frag_leaderboard WHERE racewar=X")` | **SYNC-R** | Racewar totals. |
| Redis frag cache | redis.c | `db_query("SELECT char_name, total_frags FROM frag_leaderboard ...")` | **SYNC-R** | Cache frag data for web display. |

### C.23 Hardcore / Racewar / Level Cap

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Hardcore leaders | hardcore.c | `db_query("SELECT ... FROM player_data JOIN ... WHERE ...")` | **SYNC-R** | Hardcore leaderboard display. |
| Level cap check | sql.c | `db_query("SELECT ... FROM level_cap")` | **SYNC-R** | Level cap check. |
| Racewar IP lookup | sql.c | `sql_find_racewar_for_ip()` | **SYNC-R** | IP-based racewar lookup. |

### C.24 Timers / Progress

| Function | File | Query Type | Class | Why |
|----------|------|-----------|-------|-----|
| `set_timer()` | timers.c | `qry("REPLACE INTO timers (name, date) VALUES (...)")` | **SYNC-W** | Set named timer. |
| `get_timer()` | timers.c | `qry("SELECT date FROM timers WHERE name=X")` | **SYNC-R** | Read timer. |
| `sql_save_progress()` | sql.c | `db_query("INSERT INTO progress VALUES(...)")` | **SYNC-W** | Log progress event. |

### C.25 World Quests

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| `sql_world_quest_finished()` | sql.c | Process world quest completion | **SYNC-W** | World quest completion. |
| World quest check | world_quest.c | Quest availability checks | **SYNC-R** | Check if quest available. |

### C.26 Wiz / Admin IP Lookups

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| IP lookup by player | actwiz.c | `db_query("SELECT player_name FROM log_entries WHERE ip_address LIKE X")` | **SYNC-R** | Admin tool - lookup players by IP. |
| IP lookup by pid | actwiz.c | `db_query("SELECT ip_address FROM log_entries WHERE pid=X")` | **SYNC-R** | Admin tool - lookup IPs by pid. |
| Last IP | actwiz.c | `db_query("SELECT last_ip FROM ip_info WHERE pid=X")` | **SYNC-R** | Quick IP check. |

### C.27 CTF (Capture the Flag)

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| CTF log | ctf.c | `qry("INSERT INTO ctf_data (...)")` | **SYNC-W** | Log CTF event. Low stakes, could be async. |
| CTF query | ctf.c | `qry("SELECT ... FROM ctf_data WHERE ...")` | **SYNC-R** | CTF stats display. |
| CTF boon check | ctf.c | `qry("SELECT criteria, criteria2 FROM boons WHERE opt=X")` | **SYNC-R** | CTF boon criteria. |

### C.28 Multiplay Whitelist

| Function / Query | File | Query Type | Class | Why |
|------------------|------|-----------|-------|-----|
| Whitelist load | multiplay_whitelist.c | `qry("SELECT ... FROM mp_whitelist")` | **SYNC-R** | Load whitelist. |
| Whitelist add | multiplay_whitelist.c | `qry("INSERT INTO mp_whitelist (...)")` | **SYNC-W** | Add entry. |
| Whitelist remove | multiplay_whitelist.c | `qry("DELETE FROM mp_whitelist WHERE pattern=X")` | **SYNC-W** | Remove entry. |

### C.29 Async Event Pipeline (Converted)

| Function | File | Path | Class | Why |
|----------|------|------|-------|-----|
| `persistence_record_item_event("owner_corpse_restored", ...)` | files.c | `persistence_item_event_queue_enqueue()` → worker → `sql_persistence_execute_raw()` | **ASYNC** | Records item ownership restoration after crash. |
| `persistence_record_item_event("owner_corpse", ...)` | sql_player.c | Same path | **ASYNC** | Records item ownership on corpse creation. |
| Boon shop events (6 sites) | boon.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Boon shop updates. |
| Boon progress events (4 sites) | boon.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Boon progress tracking. |
| Boon remove events (2 sites) | boon.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Boon deactivation logging. |
| Zone alignment event | epic.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Zone alignment change logging. |
| Epic gain event | sql.c | `persistence_scalar_event_queue_enqueue(line)` with fallback | **ASYNC** | Epic gain logging from `log_epic_gain_event()`. |
| Fallback writer (all subsystems) | utility.c | `persistence_write_fallback_event_line(...)` → flat file LOG_EVENT | **ASYNC (fallback)** | Used when async queue is full. Thread-safe. |

### C.30 Summary: Conversion Status

| Category | Count | Status |
|----------|-------|--------|
| Synchronous Writes | ~55 | Main thread, blocking. Most are fine (low latency, infrequent). |
| Synchronous Reads | ~45 | Main thread, blocking. Must be synchronous (player waiting for data). |
| Synchronous Mixed Read-Write | ~15 | Main thread. Delete-then-insert patterns in save pipeline. |
| Converted to Async | ~14 call sites | Boon shop/progress/remove, zone alignment, epic gain, item events (corpse). |
| Not Yet Converted (could be) | ~20 | Auction bids, artifact updates, outpost updates, shop/quest trophies, statistics, offline messages, CTF, progress — all low-stakes writes that could go async. |
| Purposefully Omitted | ~8 | `writeCharacter()` + sub-functions, `do_save_silent()`, `sql_save_locker()`, `sql_save_corpse()` — must complete synchronously to guarantee character state is saved before quit/crash/disconnect. |
| Stubs (not implemented) | 3 | `sql_persistence_item_owner_matches()`, `sql_zone_touch_finished()`, `sql_quest_finish()`. |

### C.31 Key Observations

1. **Save pipeline is entirely synchronous by design.** `writeCharacter()` → `sql_save_locker()` + `sql_save_player()` must complete before the function returns. The async event pipeline is supplementary logging, not a replacement for the authoritative save.

2. **Boon.c is the most converted subsystem.** It uses `persistence_scalar_event_queue_enqueue()` for shop updates, progress tracking, and boon removal — with `persistence_write_fallback_event_line()` fallback at every site. However, `qry("UPDATE boons_shop SET stats=...")` is still called synchronously in some paths.

3. **Epic.c is partially converted.** Zone alignment events go through the async scalar queue, but zone alignment updates (`qry("UPDATE zones SET alignment=...")`) are still synchronous. Epic gain has both a synchronous `qry()` path and an async `log_epic_gain_event()` path.

4. **Item events are barely instrumented.** Only 2 call sites use `persistence_record_item_event()` (corpse restore, corpse save). All other item ownership transitions (pickup, drop, trade, equip, container, locker, shop, auction, destroy) have NO async event recording — they only get saved during `writeCharacter()`.

5. **Large event queue has zero production call sites.** It exists and is tested, but nothing in the production code enqueues to it.

6. **Many low-stakes writes are still synchronous.** Auction bid logging, artifact state updates, outpost HP updates, shop sales, quest trophies, statistics, offline messages, CTF logging — all of these could be safely converted to async without affecting game correctness.

7. **All reads are (correctly) synchronous.** Player-facing queries (auction browse, leaderboard, wiki, nexus stones, polls) must return results immediately. These cannot be async.

---

*This document will be updated as each phase is completed. Check off items in §9 as they are done.*
