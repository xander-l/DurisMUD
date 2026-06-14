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
| Final end-to-end audit | ✅ | All 14 checks pass, no remaining gaps |

---

## Phase 4: Build & Testing

| Task | Status | Notes |
|---|---|---|
| Docker test harness (db_write tests) | ✅ | 13/13 crash-safety, 11/11 rollback |
| Compilation verification (Docker test build) | ✅ | `sql_player.c` compiles in test harness; all tests pass |
| Full project build (g++/C++20) | 🔶 | Test uses gcc/C11; full g++ build needs MySQL dev libs |
| Integration roundtrip tests | ❌ | **NEXT ITEM** — verify v19 columns survive save→load |
| Multi-table consistency tests | ❌ | |

---

## Phase 5: Remaining Work

| Task | Status | Notes |
|---|---|---|
| Transaction wrappers for all save functions | 🔶 | Some save sub-functions still warn outside txn |
| Flush path on disconnect | 🔶 | |
| Thread safety documentation | ✅ | `THREAD_SAFETY.md` updated |
| Persistence lock hierarchy | ✅ | Documented |
| Other sql_*.c files (sql_mob, sql_room, etc.) | ❌ | |
| Performance optimization (query batching, connection pooling) | ❌ | |

---

## Next High-Impact Item

**Integration roundtrip tests** — the db_write test suite verifies crash-safety and rollback behavior but doesn't test that v19 diff columns (wear_flags, item_type, item_material, bitvector1-5) survive a save→load roundtrip. Add tests that save items with non-default v19 values, load them back, and assert the values match across all 7 item tables.
