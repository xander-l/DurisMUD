/* ===================================================================
 * test_main.c — Standalone entry point for the DB write unit test
 * suite.  Compiles standalone — no MySQL or MUD dependencies needed.
 *
 * Usage:
 *   ./test_db_write              — run all tests
 *   ./test_db_write <test_name>  — run a single test
 *   ./test_db_write -l           — list all tests
 * =================================================================== */

#include <stdio.h>
#include <string.h>
#include "test_db_write.h"
#include "test_game_scenarios.h"
#include "test_data_validation.h"
#include "test_character_lifecycle.h"
#include "test_crash_stress.h"
#include "test_container_rescue.h"
#include "test_persistence_owner.h"
#include "test_transaction_rollback.h"
#include "test_disconnect_flush.h"
#include "test_v19_roundtrip.h"

static void list_tests(void)
{
    printf("Available tests:\n\n");
    printf("  SQL Escape:\n");
    printf("    escape_null_input\n");
    printf("    escape_empty_string\n");
    printf("    escape_normal_string\n");
    printf("    escape_apostrophe\n");
    printf("    escape_backslash\n");
    printf("    escape_pipe\n");
    printf("    escape_carriage_return\n");
    printf("    escape_newline\n");
    printf("    escape_mixed_special_chars\n");
    printf("    escape_null_buffer\n");
    printf("    escape_zero_buffer_size\n");
    printf("    escape_buffer_overflow\n");
    printf("    escape_exact_buffer_fit\n");
    printf("    escape_no_special_chars_long\n");
    printf("    escape_sql_injection\n");
    printf("\n  Replay Parser:\n");
    printf("    replay_basic_item_event\n");
    printf("    replay_all_fields_present\n");
    printf("    replay_minimal_fields\n");
    printf("    replay_item_uid_none\n");
    printf("    replay_event_with_apostrophes\n");
    printf("    replay_clean_note_no_pipes\n");
    printf("    replay_empty_event_type\n");
    printf("    replay_missing_optional_fields\n");
    printf("    replay_prefix_only\n");
    printf("    replay_null_line\n");
    printf("    replay_empty_line\n");
    printf("    replay_uid_is_numeric\n");
    printf("\n  Game Scenarios:\n");
    printf("    scenario_player_killed_by\n");
    printf("    scenario_player_killed_by_apostrophe\n");
    printf("    scenario_boon_shop_update_stats\n");
    printf("    scenario_boon_shop_update_points\n");
    printf("    scenario_boon_shop_insert\n");
    printf("    scenario_boon_deactivate\n");
    printf("    scenario_boon_progress_update\n");
    printf("    scenario_boon_progress_reset\n");
    printf("    scenario_epic_gain\n");
    printf("    scenario_auction_bid\n");
    printf("    scenario_auction_bid_apostrophe\n");
    printf("    scenario_zone_alignment\n");
    printf("    scenario_player_saved\n");
    printf("    scenario_locker_saved\n");
    printf("    scenario_locker_deposit\n");
    printf("    scenario_locker_withdraw\n");
    printf("    scenario_item_pickup\n");
    printf("    scenario_item_drop\n");
    printf("    scenario_item_destroyed\n");
    printf("    scenario_corpse_item_save\n");
    printf("    scenario_auction_listing\n");
    printf("    scenario_item_apostrophe_name\n");
    printf("    scenario_full_frag_chain\n");
    printf("\n  Data Validation:\n");
    printf("    roundtrip_player_saved\n");
    printf("    roundtrip_locker_deposit\n");
    printf("    roundtrip_apostrophe_names\n");
    printf("    roundtrip_item_destroyed\n");
    printf("    precision_player_saved_values\n");
    printf("    precision_locker_withdraw\n");
    printf("    precision_item_uid_unquoted\n");
    printf("    precision_vnum_negative_one\n");
    printf("    precision_none_item_short\n");
    printf("    gmcp_group_status_top_level\n");
    printf("    gmcp_group_member_fields\n");
    printf("    gmcp_group_leader_rank\n");
    printf("    gmcp_group_size_matches\n");
    printf("    gmcp_group_health_is_numeric\n");
    printf("    gmcp_npc_target_fields\n");
    printf("    gmcp_empty_group\n");
    printf("    gmcp_position_values\n");
    printf("    full_chain_roundtrip\n");
    printf("\n  Crash Stress Tests (document bugs):\n");
    printf("    stress_death_flow_atomicity_gap\n");
    printf("    stress_corpse_save_transaction_leak\n");
    printf("    stress_delete_corpse_orphans_items\n");
    printf("    stress_double_writecorpse_duplicate\n");
    printf("    stress_crash_item_duplication\n");
    printf("    stress_no_unique_constraint_corpses\n");
    printf("    stress_writecorpse_inconsistent_delete\n");
    printf("    stress_transaction_leak_cascade\n");
    printf("    stress_persistence_event_queue_divergence\n");
    printf("    stress_writecorpse_silent_failure\n");
    printf("\n  Container Rescue:\n");
    printf("    resolve_room_in_room\n");
    printf("    resolve_room_carried\n");
    printf("    resolve_room_worn\n");
    printf("    resolve_room_inside_direct\n");
    printf("    resolve_room_nested\n");
    printf("    resolve_room_nowhere\n");
    printf("    resolve_room_carried_null\n");
    printf("    resolve_room_worn_null\n");
    printf("    event_type_corpse\n");
    printf("    event_type_container\n");
    printf("    event_type_quiver\n");
    printf("    reason_corpse\n");
    printf("    reason_container\n");
    printf("\n  Persistence Owner Validation (Phase 3.2):\n");
    printf("    owner_uid_zero_keeps_item\n");
    printf("    owner_null_args_keep_item\n");
    printf("    owner_no_db_keeps_item\n");
    printf("    owner_no_events_keeps_item\n");
    printf("    owner_query_fails_keeps_item\n");
    printf("    owner_match_keeps_item\n");
    printf("    owner_type_mismatch_discards_item\n");
    printf("    owner_ref_mismatch_discards_item\n");
    printf("    owner_locker_match_keeps_item\n");
    printf("    owner_corpse_match_keeps_item\n");
    printf("    owner_production_source_is_not_stub\n");
    printf("\n  Transaction Rollback (Phase 3.4/3.5):\n");
    printf("    rollback_towns_insert_failure_preserves_old_towns\n");
    printf("    rollback_towns_first_insert_failure_preserves_old_towns\n");
    printf("    rollback_guild_ranks_insert_failure_preserves_old_ranks\n");
    printf("    rollback_guild_members_insert_failure_preserves_old_members\n");
    printf("    rollback_private_chest_item_failure_preserves_chest\n");
    printf("    rollback_commit_failure_undoes_whole_save\n");
    printf("    rollback_call_sequence_is_begin_work_commit_or_rollback\n");
    printf("    rollback_locker_item_failure_preserves_chest\n");
    printf("    rollback_corpse_item_failure_preserves_old_corpse\n");
    printf("    rollback_begin_transaction_failure_prevents_writes\n");
    printf("    rollback_production_source_has_rollback_calls\n");
    printf("\n  Disconnect Flush (Phase 3.7):\n");
    printf("    scheduled_save_is_applied_on_disconnect\n");
    printf("    queued_event_is_noop_after_flush\n");
    printf("    endtoend_no_double_save\n");
    printf("    flush_no_pending_is_noop\n");
    printf("    flush_npc_is_noop\n");
    printf("    flush_dead_char_is_noop\n");
    printf("    double_flush_is_idempotent\n");
    printf("    level_dirty_propagates\n");
    printf("    flush_all_flushes_every_pending\n");
    printf("    schedule_merges_existing_slot\n");
    printf("    full_queue_falls_back_to_sync_save\n");
    printf("\n  v19+Material Roundtrip (Phase 3.5/3.6):\n");
    printf("    save_player_items_individual_has_v19\n");
    printf("    save_player_items_batch_has_v19\n");
    printf("    save_player_pet_items_has_v19\n");
    printf("    save_locker_items_has_v19\n");
    printf("    save_shopkeeper_items_has_v19\n");
    printf("    save_corpse_items_has_v19\n");
    printf("    save_saved_items_has_v19\n");
    printf("    save_siege_items_has_v19\n");
    printf("    load_player_items_SELECT_has_v19\n");
    printf("    load_player_pet_items_SELECT_has_v19\n");
    printf("    load_locker_items_SELECT_has_v19\n");
    printf("    load_shopkeeper_items_SELECT_has_v19\n");
    printf("    load_corpse_items_SELECT_has_v19\n");
    printf("    load_saved_items_SELECT_has_v19\n");
    printf("    load_siege_items_SELECT_has_v19\n");
    printf("    load_player_items_row_reads_v19\n");
    printf("    load_player_pet_items_row_reads_v19\n");
    printf("    load_locker_items_row_reads_v19\n");
    printf("    load_shopkeeper_items_row_reads_v19\n");
    printf("    load_corpse_items_row_reads_v19\n");
    printf("    load_saved_items_row_reads_v19\n");
    printf("    load_siege_items_row_reads_v19\n");
    printf("    no_duplicate_row_read_blocks\n");
    printf("    save_functions_call_format_helper\n");
    printf("    no_type_flag_bug_in_load\n");
    printf("    all_seven_tables_present\n");
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "--list") == 0) {
            list_tests();
            return 0;
        }
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [-l] [test_name]\n", argv[0]);
            printf("  -l, --list    List all tests\n");
            printf("  -h, --help    Show this help\n");
            return 0;
        }
        /* Run single test — try each suite.  Each suite's run_one returns
         * -1 when the name isn't found, so the chain falls through.
         *
         * Note: we deliberately do NOT call any print_summary() at the end
         * here.  test_crash_stress_print_summary() (the only one called by
         * the old code path) prints crash_stress's static g_pass/g_fail,
         * which are 0 for every other suite's individual test, producing
         * the misleading "Pass: 0 / Fail: 0 / ALL CRASH-SAFETY FIXES
         * VERIFIED" output.  The run_one call already printed the test
         * result for the user.  The summary is only meaningful in the
         * full-suite path below, where each suite's run_all prints its own
         * summary. */
        int rc = test_db_write_run_one(argv[1]);
        if (rc == -1) rc = test_game_scenarios_run_one(argv[1]);
        if (rc == -1) rc = test_data_validation_run_one(argv[1]);
        if (rc == -1) rc = test_character_lifecycle_run_one(argv[1]);
        if (rc == -1) rc = test_crash_stress_run_one(argv[1]);
        if (rc == -1) rc = test_container_rescue_run_one(argv[1]);
        if (rc == -1) rc = test_persistence_owner_run_one(argv[1]);
        if (rc == -1) rc = test_transaction_rollback_run_one(argv[1]);
        if (rc == -1) rc = test_disconnect_flush_run_one(argv[1]);
        if (rc == -1) rc = test_v19_roundtrip_run_one(argv[1]);
        if (rc == -1) {
            fprintf(stderr, "Unknown test: '%s'\n", argv[1]);
            fprintf(stderr, "Run with -l to list all available tests.\n");
            return 1;
        }
        return rc > 0 ? 1 : 0;
    }

    /* Run full suite — both unit tests and game scenarios */
    int failures = test_db_write_run_all();
    printf("\n");
    failures += test_game_scenarios_run_all();
    printf("\n");
    failures += test_data_validation_run_all();
    printf("\n");
    failures += test_character_lifecycle_run_all();
    printf("\n");
    failures += test_crash_stress_run_all();
    printf("\n");
    failures += test_container_rescue_run_all();
    printf("\n");
    failures += test_persistence_owner_run_all();
    printf("\n");
    failures += test_transaction_rollback_run_all();
    printf("\n");
    failures += test_disconnect_flush_run_all();
    printf("\n");
    failures += test_v19_roundtrip_run_all();
    return failures > 0 ? 1 : 0;
}
