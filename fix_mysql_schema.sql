-- Fix missing MySQL columns and tables
-- Run this with: podman exec -it duris-mysql mysql -u root -p duris_dev < fix_mysql_schema.sql

-- Fix ip_info table for MySQL 8.0 compatibility
-- MySQL 8.0 doesn't allow '0000-00-00 00:00:00' as default values

-- Drop and recreate with proper defaults
DROP TABLE IF EXISTS ip_info;
CREATE TABLE ip_info (
  pid BIGINT NOT NULL DEFAULT 0,
  last_ip VARCHAR(50) NOT NULL DEFAULT 'none',
  last_connect DATETIME NULL DEFAULT NULL,
  last_disconnect DATETIME NULL DEFAULT NULL,
  racewar_side INT NOT NULL DEFAULT 0,
  PRIMARY KEY (pid)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS level_cap (
  id INT PRIMARY KEY AUTO_INCREMENT,
  most_frags FLOAT NOT NULL DEFAULT 0,
  racewar_leader INT NOT NULL DEFAULT 0,
  level INT NOT NULL DEFAULT 25,
  next_update DATETIME DEFAULT CURRENT_TIMESTAMP
);
-- Arih : artifacts table - main artifact tracking
-- IMPORTANT: Column order matters! The MUD code uses INSERT VALUES without column names
-- INSERT INTO artifacts VALUES( vnum, owned, locType, location, timer, type, SYSDATE())
-- IMPORTANT: ENUM order matters! Must match artifact.c:99
CREATE TABLE IF NOT EXISTS artifacts (
  vnum INT PRIMARY KEY,
  owned CHAR(1) NOT NULL,
  locType ENUM('NotInGame', 'OnNPC', 'OnPC', 'OnGround', 'OnCorpse') NOT NULL DEFAULT 'NotInGame',
  location INT NOT NULL,
  timer DATETIME,
  type INT NOT NULL,
  lastUpdate DATETIME
);

-- Arih : artifacts_mortal table structure matches the INSERT/SELECT query in artifact.c:474
CREATE TABLE IF NOT EXISTS artifacts_mortal (
  vnum INT PRIMARY KEY,
  owned CHAR(1) NOT NULL,
  locType INT NOT NULL,
  location INT NOT NULL,
  timer DATETIME,
  type INT NOT NULL
);

-- Storage locker access control table
-- Used in storage_lockers.c for managing locker permissions
CREATE TABLE IF NOT EXISTS locker_access (
  owner VARCHAR(255) NOT NULL,
  visitor VARCHAR(255) NOT NULL,
  PRIMARY KEY (owner, visitor)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
