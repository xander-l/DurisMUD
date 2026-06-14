-- Migration: add item_material column to the 7 item tables
-- Phase 3.6: persist obj->material as a diff-from-prototype value
--   NULL = item matches its prototype's material (load code uses proto)
--   non-NULL = item has a custom material override
--
-- The shared helper sql_format_item_diff_fields_and_free_proto() in
-- src/sql_player.c writes "NULL" when the item's material matches its
-- prototype, or the numeric value otherwise.
--
-- This migration adds the column to the same 7 tables that received the
-- v19 diff columns (item_type, wear_flags, bitvector1-5):
--   corpse_items, locker_items, shopkeeper_items, siege_items,
--   saved_items, player_pet_items, account_locker_items

ALTER TABLE corpse_items      ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE locker_items      ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE shopkeeper_items  ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE siege_items       ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE saved_items       ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE player_pet_items  ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE account_locker_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
