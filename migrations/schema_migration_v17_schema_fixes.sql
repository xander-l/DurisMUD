-- schema_migration_v17_schema_fixes.sql
-- Adds missing columns to tables that the wip-async code expects but
-- which were not present in the base schema migration (run_this_one.sql).
-- All statements are idempotent-safe (errors ignored at execution level).

ALTER TABLE corpse_items ADD COLUMN item_type INT NOT NULL DEFAULT 0;
ALTER TABLE player_items ADD COLUMN item_type INT NOT NULL DEFAULT 0;
ALTER TABLE player_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0;
ALTER TABLE auctions ADD COLUMN obj_info_text TEXT NULL;
