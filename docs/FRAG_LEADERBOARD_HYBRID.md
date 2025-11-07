# Frag Leaderboard Hybrid System

## Overview

The frag leaderboard hybrid system provides dual storage for frag data:
- **MUD continues using flat files** - Zero performance impact, zero risk to gameplay
- **Database tables for web statistics** - Rich queries, historical tracking, account-level stats

## Architecture

### Data Flow

1. **When character saves** (`sql_save_player_core`)
   - Updates `players_core` table (existing)
   - Updates `account_characters` table (new)
   - Updates `frag_leaderboard` table (new)

2. **When frags change** (`sql_modify_frags`)
   - Writes to `progress` table for history (existing)
   - Incrementally updates `frag_leaderboard.total_frags` (new)
   - Checks level cap (existing)

3. **When character deleted** (`deleteCharacter`)
   - Soft deletes from leaderboard tables (sets `deleted_at` timestamp)
   - Preserves frag history for statistics

4. **Fraglist command** (unchanged)
   - Still reads from flat files in `Fraglists/` directory
   - No performance impact to players

### Database Schema

#### `account_characters` Table
Maps characters to accounts with soft delete support.

```sql
CREATE TABLE `account_characters` (
  `id` int(11) NOT NULL auto_increment,
  `account_name` varchar(255) NOT NULL,
  `pid` bigint(20) NOT NULL,
  `char_name` varchar(255) NOT NULL,
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP,
  `deleted_at` datetime NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`),
  KEY `account_name` (`account_name`),
  KEY `deleted_at` (`deleted_at`)
) ENGINE=InnoDB;
```

#### `frag_leaderboard` Table
Denormalized leaderboard for fast web queries.

```sql
CREATE TABLE `frag_leaderboard` (
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
  KEY `total_frags_active` (`deleted_at`, `total_frags`),
  KEY `racewar_leaderboard` (`deleted_at`, `racewar`, `total_frags`),
  KEY `race_leaderboard` (`deleted_at`, `race`, `total_frags`),
  KEY `class_leaderboard` (`deleted_at`, `class`, `total_frags`)
) ENGINE=InnoDB;
```

## Deployment Instructions

### Phase 1: Apply Database Schema (Production Safe)

```bash
# Connect to MySQL
mysql -u duris -p duris

# Apply the migration
source sql/migrations/add_frag_leaderboard_tables.sql

# Verify tables were created
SHOW TABLES LIKE '%frag%';
SHOW TABLES LIKE '%account_characters%';

# Check table structure
DESCRIBE account_characters;
DESCRIBE frag_leaderboard;

# Exit MySQL
EXIT;
```

**Note:** This is non-breaking - adding tables doesn't affect existing code.

### Phase 2: Populate Historical Data (Optional but Recommended)

**Important:** This script needs to be tested and may require adjustments based on your actual character file format.

```bash
# Review the script first
less scripts/populate_frag_leaderboard.py

# Test on a few characters (recommended)
# You may need to adjust the parsing logic in parse_character_file()

# Run the population script
cd /home/resakse/Coding/DurisMUD
python3 scripts/populate_frag_leaderboard.py
```

**Alternative:** Skip the script and let data populate naturally as players save.

### Phase 3: Recompile MUD

```bash
cd /home/resakse/Coding/DurisMUD/src
make clean
make

# Check for compilation errors
# Fix any issues before proceeding
```

### Phase 4: Test (Critical!)

**DO NOT SKIP TESTING**

1. **Start test MUD on different port:**
   ```bash
   # Make a backup first
   cp -r /home/resakse/Coding/DurisMUD /home/resakse/Coding/DurisMUD.backup

   # Start test instance
   cd /home/resakse/Coding/DurisMUD
   ./duris 4001  # Or whatever test port
   ```

2. **Test scenarios:**
   - Login with existing character
   - Check database: `SELECT * FROM frag_leaderboard WHERE char_name = 'YourChar';`
   - Gain/lose frags (test PK)
   - Verify incremental update in database
   - Save character and quit
   - Verify full update in database
   - Create new character
   - Verify account mapping in `account_characters`

3. **Performance check:**
   - Monitor query times: `SHOW PROCESSLIST;`
   - Check for slow queries
   - Ensure no lag on character save

### Phase 5: Deploy to Production

**Only after thorough testing!**

```bash
# Stop production MUD gracefully
# (Use your normal shutdown procedure)

# Backup current binary
cp /path/to/duris /path/to/duris.old

# Copy new binary
cp src/duris /path/to/duris

# Start production MUD
# (Use your normal startup procedure)

# Monitor logs for any database errors
tail -f log/syslog
```

## Web Frontend Queries

### Top 100 Overall Leaderboard (Active Only)

```sql
SELECT char_name,
       total_frags / 100.0 AS frags,
       racewar,
       race,
       class,
       level
FROM frag_leaderboard
WHERE deleted_at IS NULL
ORDER BY total_frags DESC
LIMIT 100;
```

### Top 50 by Racewar Side

```sql
-- Good (racewar = 1)
SELECT char_name, total_frags / 100.0 AS frags, race, class, level
FROM frag_leaderboard
WHERE deleted_at IS NULL AND racewar = 1
ORDER BY total_frags DESC
LIMIT 50;

-- Evil (racewar = 2)
-- Undead (racewar = 3)
-- Illithid (racewar = 4)
```

### Top 50 by Race

```sql
SELECT char_name, total_frags / 100.0 AS frags, class, level
FROM frag_leaderboard
WHERE deleted_at IS NULL AND race = 'drow_elf'
ORDER BY total_frags DESC
LIMIT 50;
```

### Top 50 by Class

```sql
SELECT char_name, total_frags / 100.0 AS frags, race, level
FROM frag_leaderboard
WHERE deleted_at IS NULL AND class = 'necromancer'
ORDER BY total_frags DESC
LIMIT 50;
```

### All Characters for an Account

```sql
SELECT char_name,
       total_frags / 100.0 AS frags,
       racewar,
       race,
       class,
       level,
       CASE WHEN deleted_at IS NULL THEN 'Active' ELSE 'Deleted' END AS status,
       deleted_at,
       last_updated
FROM frag_leaderboard
WHERE account_name = 'Resakse'
ORDER BY total_frags DESC;
```

### Frag Distribution by Class

```sql
SELECT class,
       AVG(total_frags / 100.0) AS avg_frags,
       MIN(total_frags / 100.0) AS min_frags,
       MAX(total_frags / 100.0) AS max_frags,
       COUNT(*) AS char_count
FROM frag_leaderboard
WHERE deleted_at IS NULL
GROUP BY class
ORDER BY avg_frags DESC;
```

### Monthly Frag Gains (using progress table)

```sql
SELECT p.pid,
       fl.char_name,
       fl.account_name,
       SUM(p.delta) / 100.0 AS frags_gained
FROM progress p
JOIN frag_leaderboard fl ON p.pid = fl.pid
WHERE p.var_type = 'FRAGS'
  AND p.stamp >= DATE_SUB(NOW(), INTERVAL 1 MONTH)
  AND fl.deleted_at IS NULL
GROUP BY p.pid, fl.char_name, fl.account_name
ORDER BY frags_gained DESC
LIMIT 50;
```

### 1000+ Frag Club

```sql
SELECT char_name,
       total_frags / 100.0 AS frags,
       account_name,
       race,
       class,
       level
FROM frag_leaderboard
WHERE deleted_at IS NULL
  AND total_frags >= 100000
ORDER BY total_frags DESC;
```

## Code Integration Points

### Files Modified

1. **src/sql.h** - Added function declarations
   - `sql_update_frag_leaderboard()`
   - `sql_update_account_character()`
   - `sql_soft_delete_character()`

2. **src/sql.c** - Added implementations
   - Helper function: `get_account_name_safe()`
   - Three new functions for table updates

3. **src/sql.c** - Modified `sql_save_player_core()`
   - Added calls to update leaderboard tables after character save

4. **src/sql.c** - Modified `sql_modify_frags()`
   - Added incremental update to `frag_leaderboard.total_frags`

5. **src/files.c** - Modified `deleteCharacter()`
   - Added call to `sql_soft_delete_character()` for soft delete

### Files Created

1. **sql/migrations/add_frag_leaderboard_tables.sql** - Schema migration
2. **scripts/populate_frag_leaderboard.py** - Historical data population
3. **docs/FRAG_LEADERBOARD_HYBRID.md** - This documentation

## Troubleshooting

### Characters have "Unknown" account

**Cause:** Character not connected when saved, or account system not initialized.

**Solution:** Will auto-correct next time character saves while logged in.

### Frag counts don't match flat file

**Cause:** Population script parsing issues or database not synced.

**Solution:**
```sql
-- Check progress table totals
SELECT pid, SUM(delta) as total FROM progress WHERE var_type = 'FRAGS' GROUP BY pid;

-- Recalculate for specific character
UPDATE frag_leaderboard
SET total_frags = (
  SELECT COALESCE(SUM(delta), 0)
  FROM progress
  WHERE progress.pid = frag_leaderboard.pid
    AND var_type = 'FRAGS'
)
WHERE pid = <character_pid>;
```

### Compilation errors

**Error:** `undefined reference to sql_update_frag_leaderboard`

**Solution:** Make sure sql.h declarations match sql.c implementations. Run `make clean && make`.

### Performance issues

**Problem:** Database queries slowing down saves.

**Solution:** Check indexes are created:
```sql
SHOW INDEX FROM frag_leaderboard;
SHOW INDEX FROM account_characters;
```

If missing, run the migration script again.

## Rollback Instructions

If you need to rollback (issues found after deployment):

```bash
# Revert to old binary
cp /path/to/duris.old /path/to/duris

# Restart MUD
# (Use your normal restart procedure)

# Database tables can be left in place (won't hurt anything)
# Or drop them:
# DROP TABLE frag_leaderboard;
# DROP TABLE account_characters;
```

## Future Enhancements

Possible additions for the web frontend:

1. **Frag timeline graphs** - Plot frag gains over time
2. **PK event correlation** - Link frag changes to PK events
3. **Account statistics** - Total frags across all chars
4. **Leaderboard history** - Track rank changes over time
5. **Achievement tracking** - "First to 1000", etc.
6. **Rivalry detection** - Who frags whom most often

All of this is now possible with the database backend!

## Support

If you encounter issues:

1. Check syslog for MySQL errors
2. Verify database connectivity
3. Check table structure matches migration
4. Test queries manually in MySQL
5. Review character file parsing in population script

## License

This is part of DurisMUD codebase.
