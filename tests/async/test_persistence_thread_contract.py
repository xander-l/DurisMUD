"""
Source contract: Persistence thread ownership and shutdown

Verifies that:
- Child process waitpid() calls handle ECHILD (SIGCHLD handler race)
- Shutdown cleanup waits for outstanding persistence children
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
REDIS = (ROOT / "src" / "redis.c").read_text()


def require(needle: str, haystack: str, message: str) -> None:
    if needle not in haystack:
        raise AssertionError(message)


def test_flush_dirty_handles_echild() -> None:
    """flush_dirty_players must handle waitpid returning -1/ECHILD."""
    f = "flush_dirty_players"
    idx = REDIS.index(f)
    block = REDIS[idx:idx + 12000]  # look within ~4000 chars of the function
    require("ECHILD", block, "flush_dirty_players must handle waitpid ECHILD race")


def test_world_state_save_handles_echild() -> None:
    """redis_save_world_state must handle waitpid returning -1/ECHILD."""
    f = "redis_save_world_state"
    idx = REDIS.index(f)
    block = REDIS[idx:idx + 12000]
    require("ECHILD", block, "redis_save_world_state must handle waitpid ECHILD race")


def test_redis_cleanup_waits_for_children() -> None:
    """redis_cleanup must wait for persistence children."""
    clean = "redis_cleanup"
    idx = REDIS.index(clean)
    block = REDIS[idx:idx + 3000]
    require("dirty_flush_pid", block, "redis_cleanup must wait for dirty_flush_pid")
    require("world_state_save_pid", block, "redis_cleanup must wait for world_state_save_pid")


if __name__ == "__main__":
    test_flush_dirty_handles_echild()
    test_world_state_save_handles_echild()
    test_redis_cleanup_waits_for_children()
    print("Persistence thread ownership contracts passed")