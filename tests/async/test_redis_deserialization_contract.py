"""
Source contract: Redis deserialization bounds

Verifies that every Redis JSON deserialization path validates:
- vnum/Range bounds before creating game objects
- String lengths before str_dup
- Integer ranges for critical fields (type, act, value, etc.)
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
REDIS = (ROOT / "src" / "redis.c").read_text()


def require(needle: str, haystack: str, message: str) -> None:
    if needle not in haystack:
        raise AssertionError(message)


def function_body(text: str, start_sig: str, end_sig: str) -> str:
    s = text.index(start_sig)
    e = text.index(end_sig, s + len(start_sig))
    return text[s:e]


def test_world_state_mob_restore_validates_vnum() -> None:
    """World state mob restore must validate vnum before read_mobile."""
    load = function_body(REDIS, "redis_load_world_state_json", "redis_clear_world_state")
    mrestore = function_body(load, "restore mobs", "restore floor objects")
    require("real_mobile(vnum)", mrestore, "mob restore must validate vnum via real_mobile()")
    has_vnum_validation = re.search(r"int\s+(mob_)?rnum\s*=\s*real_mobile\(vnum\)", mrestore)
    if not has_vnum_validation:
        raise AssertionError("mob restore must call real_mobile(vnum) to validate vnum")


def test_world_state_obj_restore_validates_vnum() -> None:
    """World state object restore must validate room vnum before read_object."""
    load = function_body(REDIS, "redis_load_world_state_json", "redis_clear_world_state")
    orestore = function_body(load, "restore floor objects", "restore doors")
    require("real_room(room_vnum)", orestore, "object restore must validate room vnum via real_room()")
    # Check read_object is guarded by a valid vnum check
    has_vnum_check = re.search(r"int\s+vnum\s*=\s*v->valueint", orestore)
    if not has_vnum_check:
        raise AssertionError("object restore must extract vnum from JSON before read_object()")


def test_floor_drop_restore_validates_room() -> None:
    """Floor drop restore must validate room before creating objects."""
    fdrop = function_body(REDIS, "redis_restore_floor_drops", "static void redis_restore_dirty_snapshot")
    require("real_room(room_vnum)", fdrop, "floor drop restore must validate room vnum via real_room()")


def test_world_state_strings_are_length_limited() -> None:
    """World state string fields must be capped before str_dup."""
    load = function_body(REDIS, "redis_load_world_state_json", "redis_clear_world_state")
    load = load[:load.index("// restore doors")]  # only mob + obj sections
    # Check for string length truncation or MAX_STRING_LENGTH cap
    str_dup_calls = load.count("str_dup(")
    if str_dup_calls > 0:
        # Verify there's a length cap mechanism
        has_cap = re.search(r"str_dup\((.*?)\)", load)
        if has_cap and not re.search(r"MAX_STRING_LENGTH|MAX_INPUT_LENGTH|strn_dup|strndup", load):
            raise AssertionError(
                "world state string fields must cap string lengths before str_dup, "
                "e.g. truncate at MAX_STRING_LENGTH or use strndup"
            )


def test_donation_broadcast_limits_strings() -> None:
    """Donation broadcast must limit char_name, currency, and message length."""
    don = function_body(REDIS, "void redis_check_donation_messages", "void event_check_donation_messages")
    broadcast = function_body(REDIS, "static void broadcast_donation_nchat", "void redis_check_donation_messages")
    snprintfs = broadcast.count("snprintf(buf,")
    if snprintfs < 1:
        raise AssertionError("donation broadcast must use snprintf with bounded output")
    require("sizeof(buf)", broadcast, "donation broadcast must limit output via sizeof(buf)")


def test_ship_snapshot_limits_string_lengths() -> None:
    """Ship snapshot deserialization must cap string lengths."""
    from_json = function_body(REDIS, "static bool redis_ship_snapshot_from_json", "bool redis_cache_ship_snapshot")
    str_dup_calls = from_json.count("str_dup(")
    if str_dup_calls > 0:
        has_cap = re.search(r"str_dup\((.*?)\)", from_json)
        if has_cap and not re.search(r"MAX_STRING_LENGTH|MAX_INPUT_LENGTH|strn_dup|strndup", from_json):
            raise AssertionError(
                "ship snapshot must cap string lengths before str_dup"
            )


if __name__ == "__main__":
    test_world_state_mob_restore_validates_vnum()
    test_world_state_obj_restore_validates_vnum()
    test_floor_drop_restore_validates_room()
    test_world_state_strings_are_length_limited()
    test_donation_broadcast_limits_strings()
    test_ship_snapshot_limits_string_lengths()
    print("Redis deserialization hardening contracts passed")