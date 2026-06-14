-- ============================================================================
-- tests/db_write/schema.sql
-- Minimal test schema for live DB roundtrip integration tests.
-- Tables match production but are simplified for test isolation.
-- ============================================================================

CREATE DATABASE IF NOT EXISTS duris_test
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_general_ci;

USE duris_test;

-- ============================================================================
-- Table: accounts (matches production columns)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `accounts` (
  `id` int(11) NOT NULL auto_increment,
  `account_name` varchar(255) NOT NULL,
  `email` varchar(255) DEFAULT NULL,
  `password` varchar(255) DEFAULT NULL,
  `confirmation_code` varchar(255) DEFAULT NULL,
  `confirmed` int(11) DEFAULT 0,
  `confirmation_sent` int(11) DEFAULT 0,
  `blocked` int(11) DEFAULT 0,
  `last_login` datetime NULL DEFAULT NULL,
  `last_good_char` datetime NULL DEFAULT NULL,
  `last_evil_char` datetime NULL DEFAULT NULL,
  `flags1` bigint(20) unsigned DEFAULT 0,
  `flags2` bigint(20) unsigned DEFAULT 0,
  `flags3` bigint(20) unsigned DEFAULT 0,
  `flags4` bigint(20) unsigned DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `account_name` (`account_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Table: player_data (simplified - only columns used by tests)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `player_data` (
  `pid` int(11) NOT NULL auto_increment,
  `name` varchar(255) NOT NULL,
  `level` int(11) DEFAULT 1,
  `race` int(11) DEFAULT 0,
  `racewar` int(11) DEFAULT 1,
  `m_class` int(10) unsigned DEFAULT 1,
  `sex` int(11) DEFAULT 0,
  `last_room` int(11) DEFAULT 0,
  `last_save` datetime NULL DEFAULT NULL,
  PRIMARY KEY (`pid`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Table: account_characters (matches production exactly)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `account_characters` (
  `id` int(11) NOT NULL auto_increment,
  `account_name` varchar(255) NOT NULL,
  `pid` bigint(20) NOT NULL,
  `char_name` varchar(255) NOT NULL,
  `login_count` bigint(20) unsigned DEFAULT 0,
  `last_login` datetime NULL DEFAULT NULL,
  `blocked` int(11) DEFAULT 0,
  `racewar` int(11) DEFAULT 1,
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP,
  `deleted_at` datetime NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`),
  UNIQUE KEY `acct_char` (`account_name`, `char_name`),
  KEY `account_name` (`account_name`),
  KEY `char_name` (`char_name`),
  KEY `deleted_at` (`deleted_at`),
  KEY `account_active` (`account_name`, `deleted_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Table: frag_leaderboard (matches production exactly)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `frag_leaderboard` (
  `id` int(11) NOT NULL auto_increment,
  `pid` bigint(20) NOT NULL,
  `account_name` varchar(255) NOT NULL,
  `char_name` varchar(255) NOT NULL,
  `total_frags` int(11) NOT NULL DEFAULT 0,
  `racewar` int(11) NOT NULL,
  `race` varchar(50) DEFAULT NULL,
  `class` varchar(50) DEFAULT NULL,
  `level` int(11) DEFAULT NULL,
  `deleted_at` datetime NULL DEFAULT NULL,
  `last_updated` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`),
  KEY `char_name` (`char_name`),
  KEY `account_name` (`account_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Table: lockers (simplified from production - owner tracking)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `lockers` (
  `id` int(11) NOT NULL auto_increment,
  `owner_pid` bigint(20) NOT NULL,
  `locker_name` varchar(255) NOT NULL,
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `owner_pid` (`owner_pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Table: locker_chests (public/private chests within a locker)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `locker_chests` (
  `id` int(11) NOT NULL auto_increment,
  `locker_id` int(11) NOT NULL,
  `chest_name` varchar(255) DEFAULT NULL,
  `chest_password` varchar(255) DEFAULT NULL,
  `is_public` tinyint(1) DEFAULT 1,
  PRIMARY KEY (`id`),
  KEY `locker_id` (`locker_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Table: locker_items (matches production INSERT columns)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `locker_items` (
  `id` int(11) NOT NULL auto_increment,
  `locker_id` int(11) NOT NULL,
  `chest_id` int(11) DEFAULT NULL,
  `vnum` int(11) NOT NULL,
  `container_id` int(11) DEFAULT NULL,
  `quantity` int(11) DEFAULT 1,
  `weight` float DEFAULT 0,
  `cost` int(11) DEFAULT 0,
  `timer` bigint(20) DEFAULT 0,
  `extra_flags` bigint(20) unsigned DEFAULT 0,
  `wear_flags` varchar(255) DEFAULT NULL,
  `item_type` varchar(50) DEFAULT NULL,
  `value0` int(11) DEFAULT 0,
  `value1` int(11) DEFAULT 0,
  `value2` int(11) DEFAULT 0,
  `value3` int(11) DEFAULT 0,
  `value4` int(11) DEFAULT 0,
  `value5` int(11) DEFAULT 0,
  `value6` int(11) DEFAULT 0,
  `value7` int(11) DEFAULT 0,
  `bitvector1` bigint(20) unsigned DEFAULT 0,
  `bitvector2` bigint(20) unsigned DEFAULT 0,
  `bitvector3` bigint(20) unsigned DEFAULT 0,
  `bitvector4` bigint(20) unsigned DEFAULT 0,
  `bitvector5` bigint(20) unsigned DEFAULT 0,
  `item_material` varchar(50) DEFAULT NULL,
  `name` varchar(255) DEFAULT 'none',
  `short_descr` varchar(255) DEFAULT NULL,
  `description` text,
  `action_descr` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `locker_id` (`locker_id`),
  KEY `chest_id` (`chest_id`),
  KEY `vnum` (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Table: player_pet_items (matches production - for pet equipment roundtrip)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `player_pet_items` (
  `id` int(11) NOT NULL auto_increment,
  `pet_id` int(11) NOT NULL,
  `vnum` int(11) NOT NULL,
  `equip_slot` int(11) DEFAULT 0,
  `container_id` int(11) DEFAULT NULL,
  `weight` float DEFAULT 0,
  `cost` int(11) DEFAULT 0,
  `timer` bigint(20) DEFAULT 0,
  `extra_flags` bigint(20) unsigned DEFAULT 0,
  `wear_flags` varchar(255) DEFAULT NULL,
  `item_type` varchar(50) DEFAULT NULL,
  `value0` int(11) DEFAULT 0,
  `value1` int(11) DEFAULT 0,
  `value2` int(11) DEFAULT 0,
  `value3` int(11) DEFAULT 0,
  `value4` int(11) DEFAULT 0,
  `value5` int(11) DEFAULT 0,
  `value6` int(11) DEFAULT 0,
  `value7` int(11) DEFAULT 0,
  `bitvector1` bigint(20) unsigned DEFAULT 0,
  `bitvector2` bigint(20) unsigned DEFAULT 0,
  `bitvector3` bigint(20) unsigned DEFAULT 0,
  `bitvector4` bigint(20) unsigned DEFAULT 0,
  `bitvector5` bigint(20) unsigned DEFAULT 0,
  `item_material` varchar(50) DEFAULT NULL,
  `name` varchar(255) DEFAULT 'none',
  `short_descr` varchar(255) DEFAULT NULL,
  `description` text,
  `action_descr` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `pet_id` (`pet_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- Table: latency_trace (for latency measurement tests)
-- ============================================================================
CREATE TABLE IF NOT EXISTS `latency_trace` (
  `id` int(11) NOT NULL auto_increment,
  `section_name` varchar(64) NOT NULL,
  `usec_spent` bigint(20) NOT NULL DEFAULT 0,
  `pulse_num` int(11) NOT NULL DEFAULT 0,
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `section_name` (`section_name`),
  KEY `pulse_num` (`pulse_num`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
