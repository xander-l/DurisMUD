-- Populate frag leaderboard from existing database tables
-- This pulls data from players_core and progress tables
-- Run this ONCE after creating the frag leaderboard tables

-- Populate frag_leaderboard from players_core + progress
INSERT INTO frag_leaderboard (pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at)
SELECT
  pc.pid,
  'Unknown' as account_name,  -- Will be updated when players login
  pc.name as char_name,
  COALESCE((SELECT SUM(p.delta) FROM progress p WHERE p.pid = pc.pid AND p.var_type = 'FRAGS'), 0) as total_frags,
  pc.racewar,
  pc.race,
  pc.classname,
  pc.level,
  NULL as deleted_at  -- All existing characters are active
FROM players_core pc
WHERE pc.active = 1  -- Only active characters
ON DUPLICATE KEY UPDATE
  char_name = VALUES(char_name),
  total_frags = VALUES(total_frags),
  racewar = VALUES(racewar),
  race = VALUES(race),
  class = VALUES(class),
  level = VALUES(level);

-- Populate account_characters (will be filled with real account names when players login)
INSERT INTO account_characters (account_name, pid, char_name, created_at, deleted_at)
SELECT
  'Unknown' as account_name,  -- Will be updated when players login
  pc.pid,
  pc.name as char_name,
  NOW() as created_at,
  NULL as deleted_at
FROM players_core pc
WHERE pc.active = 1
ON DUPLICATE KEY UPDATE
  char_name = VALUES(char_name);

-- Show results
SELECT 'Populated frag_leaderboard:' as status;
SELECT COUNT(*) as total_characters FROM frag_leaderboard WHERE deleted_at IS NULL;
SELECT char_name, total_frags / 100.0 as frags, race, class, level
FROM frag_leaderboard
WHERE deleted_at IS NULL
ORDER BY total_frags DESC
LIMIT 10;
