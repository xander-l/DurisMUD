# Thread Safety & Locking Hierarchy

**Status:** Living document for the `feature/multithreading-database-persistence` branch.
**Audience:** Contributors who add code that touches the persistence queue, fallback log, MySQL persistence connection, or latency trace.
**Last reviewed against:** `src/persistence_queue.c` (item/scalar/large workers), `src/persistence_queue.h`, `src/utility.c` (fallback log writer + worker callbacks), `src/sql_persistence_raw.c` + `src/sql.c` (`persistence_sql_mutex`), `src/latency_trace.h`.

> **Locating mutexes in source:** the inventory in §2 uses symbol names
> (which don't drift) rather than line numbers (which do). To find any
> mutex in the current source, grep for the symbol name.

## Table of Contents

1. [Why This Document Exists](#1-why-this-document-exists)
2. [Mutex Inventory](#2-mutex-inventory)
3. [Lock Ordering Rules](#3-lock-ordering-rules-the-only-rules-that-matter)
   - 3.1 [Strict Hierarchy](#31-strict-hierarchy)
   - 3.2 [Forbidden Patterns](#32-forbidden-patterns)
   - 3.3 [Correct Patterns](#33-correct-patterns)
   - 3.4 [Why This Prevents ABBA](#34-why-this-prevents-abba)
4. [Threads in the Persistence Subsystem](#4-threads-in-the-persistence-subsystem)
5. [Condition Variables](#5-condition-variables)
6. [Heartbeat / Deadlock Detection](#6-heartbeat--deadlock-detection)
7. [Failure Modes & Recovery](#7-failure-modes--recovery)
8. [Adding New Mutexes (Checklist)](#8-adding-new-mutexes-checklist)
9. [Testing the Locking Invariants](#9-testing-the-locking-invariants)
10. [References](#10-references)

---

## 1. Why This Document Exists

The persistence subsystem introduced a queue + worker thread model in the
`feature/multithreading-database-persistence` branch. It has **four threads**
that can touch the same backing stores (MySQL `persistenceDB` connection,
`logs/LOG_EVENT` flat-file log, in-memory queues, latency ring buffer). Without
an explicit lock ordering, ABBA deadlocks between the queue mutexes and the
SQL mutex are a real risk (see `IMPLEMENTATION_PLAN.md §5.1 RC-1`).

This document is the **single source of truth** for which mutexes exist, who
acquires them, and in what order. Any new code that touches any of these
mutexes **must** conform to the rules in §3 — and any new mutex that nests
with these must be added to §2 + §3.

---

## 2. Mutex Inventory

| # | Mutex | File | Scope | Protects | Acquired by |
|---|-------|------|-------|----------|-------------|
| 1 | `persistence_item_event_queue_mutex` | `src/persistence_queue.c` | `static` (file scope) | `persistence_item_event_queue` (events, head, tail, count, capacity, dropped, resize_count) and worker control flags (`is_running`, `stop_requested`, `drain_requested`, `last_heartbeat`) | main thread (enqueue/dequeue/pending/etc.) and item-event worker thread |
| 2 | `persistence_scalar_event_queue_mutex` | `src/persistence_queue.c` | `static` (file scope) | `persistence_scalar_event_queue` + worker control flags | main thread and scalar-event worker thread |
| 3 | `persistence_large_event_queue_mutex` | `src/persistence_queue.c` | `static` (file scope) | `persistence_large_event_queue` + worker control flags | main thread and large-event worker thread |
| 4 | `persistence_sql_mutex` | `src/sql.c` | `extern` (process-wide) | The MySQL `persistenceDB` connection | any thread that calls `sql_persistence_execute_raw()` (item/scalar/large worker writer callbacks, `persistence_replay_fallback_events()` on boot) |
| 5 | `persistence_fallback_log_mutex` | `src/utility.c` | `static` (file scope) | Appends to `logs/LOG_EVENT` flat-file log | any thread that calls `persistence_write_fallback_event_line()` or the per-worker `persistence_*_event_log_writer()` fallback path |
| 6 | `_latency_mutex` | `src/latency_trace.h` | `static` (translation-unit scope) | The latency ring buffer + per-section accumulators in `latency_trace.h` | any thread that calls `latency_trace_record()` / `latency_trace_reset()` / `latency_trace_dump()` |

**Two non-persistence mutexes also exist in the codebase but are out of scope
for this document** (they live in their own sub-systems and do not nest with
the persistence mutexes):

- `connect_mutex`, `error_mutex`, `wq->mutex` in `src-migrate/migrate_common.c` — used by the offline migration tool, not loaded by the live MUD.
- Game-level character/descriptor/wait queues are not documented here — they
  predate the persistence refactor and are protected by convention only.

---

## 3. Lock Ordering Rules (the only rules that matter)

> **If you remember nothing else from this document, remember this:**
> **A queue mutex is NEVER held while acquiring any other mutex.**
> The writer callback is called **outside** every queue mutex.

### 3.1 Strict Hierarchy

Lock acquisitions must follow this order from outermost (acquired first) to
innermost (acquired last). **Never acquire a higher-numbered lock while
holding a lower-numbered one.**

```
┌─────────────────────────────────────────────────────────────┐
│  Main thread only:                                         │
│    lock(queue_mutex) → enqueue/drop → unlock(queue_mutex)   │
│    on full:  lock(fallback_mutex) → write → unlock         │
│  Queue mutex is RELEASED before fallback mutex is acquired. │
│  No nesting.                                                │
├─────────────────────────────────────────────────────────────┤
│  Worker thread:                                            │
│   1. lock(queue_mutex)                                     │
│   2. wait for event, copy line out                         │
│   3. unlock(queue_mutex)         ←─ MUST release before #4  │
│   4. call writer callback                                  │
│        ├→ sql_persistence_execute_raw: lock(sql_mutex)     │
│        │    → mysql_real_query → unlock(sql_mutex)         │
│        └→ on SQL failure: lock(fallback_mutex)             │
│             → fopen/fputs/fclose → unlock(fallback_mutex)  │
│   5. lock(queue_mutex)           ←─ re-acquire for pop     │
│   6. pop, update counters → unlock(queue_mutex)            │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  Watchdog (main thread):  lock(queue_mutex) only            │
│  to read last_heartbeat. No nested locks.                   │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  Boot replay (main thread, single-threaded):                │
│  fgets(LOG_EVENT) → sql_persistence_execute_raw             │
│  → lock(sql_mutex) → mysql_real_query → unlock             │
│  No queue_mutex involved. No fallback_mutex either.        │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 Forbidden Patterns

These patterns **must not appear** anywhere in the codebase. CI / code review
should flag them.

```c
/* ❌ FORBIDDEN — queue mutex held while acquiring sql_mutex */
pthread_mutex_lock(&persistence_item_event_queue_mutex);
sql_persistence_execute_raw(line);   /* locks persistence_sql_mutex */
pthread_mutex_unlock(&persistence_item_event_queue_mutex);

/* ❌ FORBIDDEN — queue mutex held while acquiring fallback_mutex */
pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
persistence_write_fallback_event_line(line, ...);
pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

/* ❌ FORBIDDEN — sql_mutex held while acquiring anything else */
pthread_mutex_lock(&persistence_sql_mutex);
persistence_write_fallback_event_line(line, ...);
pthread_mutex_unlock(&persistence_sql_mutex);

/* ❌ FORBIDDEN — fallback_mutex held while acquiring anything else */
pthread_mutex_lock(&persistence_fallback_log_mutex);
sql_persistence_execute_raw(line);
pthread_mutex_unlock(&persistence_fallback_log_mutex);
```

### 3.3 Correct Patterns

```c
/* ✅ CORRECT — writer called outside queue mutex */
pthread_mutex_lock(&persistence_item_event_queue_mutex);
/* ... copy line out into a local buffer ... */
pthread_mutex_unlock(&persistence_item_event_queue_mutex);   /* release first */

if (sql_persistence_write_item_event_line(line))            /* locks sql_mutex internally */
    return 1;
/* ... on failure, fall through to flat log ... */
pthread_mutex_lock(&persistence_fallback_log_mutex);         /* acquired fresh */
fputs(line, log_f);
pthread_mutex_unlock(&persistence_fallback_log_mutex);

/* ✅ CORRECT — main thread enqueue with full-queue fallback */
if (!persistence_item_event_queue_enqueue(line))            /* locks queue_mutex internally */
{
    /* queue_mutex is already released by the time we get here */
    persistence_write_fallback_event_line(line, ...);        /* locks fallback_mutex */
}
```

### 3.4 Why This Prevents ABBA

ABBA deadlock requires two threads to acquire two locks in **opposite order**:

```
Thread 1:  A → B
Thread 2:  B → A       ←─ both stuck, classic deadlock
```

The design above **never holds two mutexes at once**. The only mutex
combinations that ever coexist in the same critical section are:

| Combination | Where it happens | Safe? |
|-------------|------------------|-------|
| `queue_mutex` ↔ `sql_mutex` | **Never** (writer called outside queue_mutex) | ✅ by construction |
| `queue_mutex` ↔ `fallback_mutex` | **Never** (caller of fallback doesn't hold queue_mutex) | ✅ by construction |
| `sql_mutex` ↔ `fallback_mutex` | **Never** (writer releases sql_mutex before taking fallback_mutex) | ✅ by construction |
| `queue_mutex` ↔ `_latency_mutex` | Only if a queue op calls `latency_trace_*()` while holding queue_mutex. Currently `persistence_scalar_event_queue_enqueue()` does this on the drop path. This is a **documented exception**, not a deadlock (the latency ring buffer never calls back into queue code), but reviewers should be aware of it. | ✅ safe (leaf lock) |
| `fallback_mutex` ↔ `_latency_mutex` | The worker writer callbacks (`persistence_item_event_log_writer`, `persistence_scalar_event_log_writer`) call `latency_trace_record("fallback_file_write", _fb_us, 0)` **inside** the `persistence_fallback_log_mutex` critical section (`src/utility.c` ~line 1385). Same reasoning as above — latency ring buffer never calls back into fallback code. | ✅ safe (leaf lock) |
| `queue_mutex` ↔ `logit()` / `wizlog()` | `persistence_queue_grow()` calls `logit("logs/log/status", ...)` and `wizlog(57, ...)` while holding the queue mutex (called from inside `*_event_queue_enqueue()` on the grow path). These logging primitives are pre-existing game-log routines that do **not** internally acquire any persistence mutex; treat them as no-ops for the purposes of this hierarchy. A future contributor adding a new logging call inside a queue-locked section is safe **as long as** the logger does not call back into persistence code. | ✅ safe (logging leaf) |

---

## 4. Threads in the Persistence Subsystem

| # | Thread | Started by | Mutexes touched | Notes |
|---|--------|------------|-----------------|-------|
| 1 | **Main game thread** | `run_the_game()` in `comm.c` | All queue mutexes (read/write), fallback mutex (full-queue path), sql_mutex (boot replay only) | Single-threaded for the game loop. All `persistence_*_event_queue_*()` calls from game code are main-thread. |
| 2 | **Item-event worker** | `persistence_start_item_event_worker()` in `comm.c` | `persistence_item_event_queue_mutex` (its own queue only), `persistence_sql_mutex` (via writer), `persistence_fallback_log_mutex` (via writer fallback), `_latency_mutex` (writer records latency) | Drains `persistence_item_event_queue`. Writer = `persistence_item_event_log_writer` in `utility.c`. |
| 3 | **Scalar-event worker** | `persistence_start_scalar_event_worker()` in `comm.c` | `persistence_scalar_event_queue_mutex`, `persistence_sql_mutex`, `persistence_fallback_log_mutex`, `_latency_mutex` | Drains `persistence_scalar_event_queue`. Writer = `persistence_scalar_event_log_writer` in `utility.c`. |
| 4 | **Large-event worker** | `persistence_start_large_event_worker()` in `comm.c` | `persistence_large_event_queue_mutex`, `persistence_sql_mutex`, `persistence_fallback_log_mutex`, `_latency_mutex` | Drains `persistence_large_event_queue` (128KB-payload events, pkill info, etc.). |
| 5 | **Watchdog (main thread, periodic)** | **Triggered by:** main-thread `event_autosave()` path in `actoth.c` (which calls `do_save_silent()` → `persistence_flush_item_events(64)`) **and** the `persistence_*_worker_stop()` teardown path. **Implementation:** `persistence_worker_heartbeat_check()` (forward-declared at `src/utility.c` ~line 1340) called from `persistence_flush_scalar_events()`. | All three queue mutexes (read-only, to read `last_heartbeat` + `is_running`); may call `persistence_start_*_event_worker()` which takes queue_mutex | Runs in main-thread context. Restarts workers whose `heartbeat_age >= 30s` and `is_running == 0`. |

> **Forked children:** `sql_log_player_login()` in `src/sql.c` does
> `fork()` and the child runs `mysql_*` calls with `sql_create_child_connection()`
> (own `MYSQL*` handle, **no shared `persistence_sql_mutex`**). The child
> is single-threaded w.r.t. persistence mutexes by virtue of being a fresh
> process. Not a locking concern.

---

## 5. Condition Variables

Two `pthread_cond_t`s pair with queue mutexes to implement the worker's
blocking wait:

- `persistence_item_event_queue_cond` (in `persistence_queue.c`, paired with mutex #1)
- `persistence_scalar_event_queue_cond` (in `persistence_queue.c`, paired with mutex #2)
- `persistence_large_event_queue_cond` (in `persistence_queue.c`, paired with mutex #3)

Workers wait on the cond while holding the matching queue mutex, and the
producer signals the cond after enqueue. **Signaling is always done under
the matching queue mutex** (per POSIX requirement), and the worker only
acquires the queue mutex — no other mutex — around the `cond_wait()` call.

This means there is no cond-var-induced risk of nesting queue_mutex with
sql_mutex or fallback_mutex.

---

## 6. Heartbeat / Deadlock Detection

Each worker maintains a `time_t last_heartbeat` field, protected by its own
queue mutex. The worker updates this timestamp:

1. **At the top of every loop iteration** (under queue_mutex, after `cond_wait` wakeup).
2. **After waking from `cond_wait`** (under queue_mutex).

The main-thread watchdog `persistence_worker_heartbeat_check()` (default
threshold = `PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS = 30` seconds, in
`persistence_queue.h:18`) reads each worker's `last_heartbeat` under its
queue mutex. If the heartbeat is stale (≥ 30s old) **and** the worker is
not running (pthread killed), the watchdog auto-restarts the worker.

> **What this does and does not detect**
> - ✅ Detects a worker thread that crashed (`pthread_exit` without cleanup, killed by signal, segfaulted) — `pthread_kill(tid, 0)` returns `ESRCH`, watchdog clears `is_running` and restarts.
> - ✅ Detects a worker that hung while **holding** its own queue mutex (heartbeat stops updating because the worker is stuck in `cond_wait` and never re-enters the loop head). This catches the `persistence_queue_grow()` realloc failure path and the `latency_trace_record()` deadlock.
> - ❌ Does **not** detect a worker stuck in a blocking MySQL call **without** holding its queue mutex — see `persistence_item_event_worker_running()` comment in `persistence_queue.c` for the explicit caveat. Mitigation: the 30s threshold is generous enough that a slow MySQL query is not a false positive. (Per-call MySQL timeout is bounded: `MYSQL_OPT_READ_TIMEOUT` / `MYSQL_OPT_WRITE_TIMEOUT` are set to **10 seconds** in `src/sql.c:initialize_mysql()`, so the worst-case single-write block before fallback kicks in is ~10s — well within the 30s heartbeat threshold.)

The watchdog itself **only reads** queue mutex state — it does not modify
shared data, so it cannot participate in a deadlock cycle.

---

## 7. Failure Modes & Recovery

| Symptom | Likely cause | Detection | Auto-recovery |
|---------|--------------|-----------|---------------|
| `is_running == 1` but `pthread_kill(tid, 0) == ESRCH` | Worker thread died without cleanup (segfault, killed signal) | `persistence_*_event_worker_running()` | Watchdog clears flag, next call falls through to sync path; worker restart on next watchdog tick |
| `heartbeat_age >= 30s` | Worker stuck (mutex deadlock, blocking I/O, infinite loop in writer callback) | `persistence_worker_heartbeat_check()` | Watchdog restarts the worker; `auto_restart` alert is logged |
| `persistence_sql_mutex` blocking for >`MYSQL_OPT_*_TIMEOUT` | MySQL connection drop, network partition, server overload | MySQL library timeout | `sql_persistence_execute_raw()` returns FALSE; writer falls through to `persistence_write_fallback_event_line()` (still under fallback_mutex, never blocks indefinitely) |
| Queue full | Producer rate > worker drain rate | `persistence_*_event_queue_enqueue()` returns 0 | Caller writes the event to `logs/LOG_EVENT` via `persistence_write_fallback_event_line()`. On next boot, `persistence_replay_fallback_events()` re-inserts from `LOG_EVENT`. |
| `logs/LOG_EVENT` open failure | Disk full, permission error, filesystem unmounted | `fopen()` returns NULL inside fallback path | `persistence_alert()` fires; event is **dropped**. Loud alert ensures this is noticed. |

---

## 8. Adding New Mutexes (Checklist)

If you need to introduce a new mutex that nests with any of the persistence
mutexes above, do the following:

1. **Update §2** of this document with the new mutex's name, location, scope, and ownership.
2. **Update §3.1** to place the new mutex in the hierarchy. The new mutex must
   sit at a level where no existing mutex is held while acquiring it.
3. **Verify §3.4** still holds — the table of "mutex combinations that ever
   coexist" must remain either empty (best) or safe-by-construction.
4. **Add a code example** to §3.3 showing the correct usage.
5. **Document any exception** in §3.4 with explicit reasoning.
6. **Add a test** to `tests/db_write/` that exercises the new mutex and
   would fail loudly if a future change introduces a nested-lock deadlock.

> **Template for new worker-writer callbacks:** copy the pattern from
> `persistence_item_event_log_writer` in `src/utility.c` — that's the
> canonical example of "writer holds no queue_mutex, only ever acquires
> `persistence_sql_mutex` or `persistence_fallback_log_mutex`, never both".
> The scalar- and large-event writer callbacks follow the same shape.

If the new mutex is **leaf-level** (no callbacks, no nested acquisitions),
you can skip steps 2–5 and just add a row to §2.

---

## 9. Testing the Locking Invariants

These tests already exist and protect the hierarchy:

| Test file / harness | What it proves |
|---------------------|----------------|
| `tests/db_write/test_db_write.c` + siblings (Layer 1+2 in `IMPLEMENTATION_PLAN.md §8.1`) | Queue + worker behave correctly under contention, overflow, drain. Does not test lock order explicitly but fails if mutexes are missing. Compiled standalone — no MySQL or MUD state needed. |
| `tests/db_write/Dockerfile.test` + `make && ./test_db_write` | End-to-end: build = source of truth. `RUN make && ./test_db_write` in the Dockerfile means a test failure fails the build, not just the run. Any compile error or runtime deadlock in a new test would fail the build. Run via `docker run --rm duris-db-write-test ./test_db_write` (or `./test_db_write -l` to list, `./test_db_write <name>` to run one). |
| `persistence_*_event_worker_stuck()` integration | Heartbeat age advances → worker not stuck. Heartbeat age stale → worker restarted. Verifies the §6 invariant. |
| `tests/db_write/test_container_rescue.c`, `test_crash_stress.c` | Exercise the fallback log writer path under stress (concurrent producer + consumer + container kill). Catches any future change that nests queue_mutex with fallback_mutex. |

A **ThreadSanitizer build** (`-fsanitize=thread`) is the next-level check —
it instruments all `pthread_mutex_lock` calls and will loudly report any
unprotected shared state. See `IMPLEMENTATION_PLAN.md §5.2` action item 2
and §8.4 TSan build target.

---

## 10. References

- `IMPLEMENTATION_PLAN.md` §1.1 (architecture), §5.1 (race conditions), §5.2 (locking hierarchy action items), §8.1 Layer 3+4 (tests)
- `src/persistence_queue.c` (queues, workers, heartbeats)
- `src/persistence_queue.h` (tunables: queue caps, heartbeat threshold, line max len)
- `src/sql_persistence_raw.c` (the only place `persistence_sql_mutex` is acquired in the raw-write path)
- `src/sql.c:133` (`persistence_sql_mutex` declaration)
- `src/utility.c:90` (`persistence_fallback_log_mutex` declaration)
- `src/utility.c:persistence_item_event_log_writer` (worker callback — the **template** for all new worker writers)
- `src/latency_trace.h` (leaf-level ring buffer; locked by every writer callback on the `fallback_file_write` trace)
- `tests/db_write/Dockerfile.test` (build = source of truth)

---

*If you change the locking model, update this document in the same commit.*
