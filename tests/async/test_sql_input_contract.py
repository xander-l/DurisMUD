#!/usr/bin/env python3
"""Source contracts for player-controlled SQL boundary hardening."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AUCTION = (ROOT / "src" / "auction_houses.c").read_text()
SQL = (ROOT / "src" / "sql.c").read_text()
FIGHT = (ROOT / "src" / "fight.c").read_text()
ASSOCS = (ROOT / "src" / "assocs.c").read_text()
WHITELIST = (ROOT / "src" / "multiplay_whitelist.c").read_text()
BOON = (ROOT / "src" / "boon.c").read_text()
TIMERS = (ROOT / "src" / "timers.c").read_text()
SQL_PLAYER = (ROOT / "src" / "sql_player.c").read_text()


def require(needle: str, haystack: str, message: str) -> None:
    if needle not in haystack:
        raise AssertionError(message)


def function_body(source: str, signature: str, next_signature: str) -> str:
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


def test_auction_fallback_uses_escaped_object_blob() -> None:
    offer = function_body(AUCTION, "bool auction_offer(P_char ch, char *args)", "bool auction_list(")
    fallback = offer[offer.index("trying old insert"):]
    require("esc_buff,", fallback,
            "legacy auction insert must use the escaped serialized object blob")
    if "\n\t\t        buff," in fallback:
        raise AssertionError("legacy auction insert must not interpolate the player-controlled parse buffer")


def test_private_chest_queries_do_not_truncate_escaped_input() -> None:
    create = function_body(SQL_PLAYER, "int sql_create_private_chest", "bool sql_delete_private_chest")
    require("string name_esc = escape_str(chest_name);", create,
            "private chest creation must dynamically escape the chest name")
    require("string password_esc = escape_str(password);", create,
            "private chest creation must dynamically escape the password")
    if "char query[" in create or "snprintf(query" in create:
        raise AssertionError("private chest creation must not truncate escaped input into a fixed query")

    lookup = function_body(SQL_PLAYER, "int sql_get_chest_id", "bool sql_verify_chest_password")
    require("string name_esc = escape_str(chest_name);", lookup,
            "private chest lookup must dynamically escape the chest name")
    if "char query[" in lookup or "snprintf(query" in lookup:
        raise AssertionError("private chest lookup must not truncate escaped input into a fixed query")

    verify = function_body(SQL_PLAYER, "bool sql_verify_chest_password", "int sql_count_private_chests")
    require("string password_esc = escape_str(password);", verify,
            "private chest password checks must dynamically escape the password")
    if "char query[" in verify or "snprintf(query" in verify:
        raise AssertionError("private chest password checks must not truncate escaped input into a fixed query")


def test_shared_text_keys_escape_before_queries() -> None:
    boon = function_body(BOON, "int extend_boon(int id, int extend, const char *name)", "static void boon_mob_label")
    require("string author_esc = escape_str(name);", boon,
            "boon author names must be escaped before update")
    require("author_esc.c_str()", boon,
            "boon update must use the escaped author")

    create = function_body(BOON, "int create_boon(BoonData *bdata)", "int create_boon_progress(")
    require("string author_esc = escape_str(bdata->author.c_str());", create,
            "boon creation must escape the author name")
    require("author_esc.c_str()", create,
            "boon insert must use the escaped author")

    mud_info_start = SQL.rindex("string get_mud_info(const char *name)")
    mud_info = SQL[mud_info_start:SQL.index("void send_mud_info", mud_info_start)]
    require("string name_esc = escape_str(name);", mud_info,
            "mud-info lookup keys must be escaped")
    require("name_esc.c_str()", mud_info,
            "mud-info lookup must use the escaped key")

    production = TIMERS[TIMERS.index("#else"):TIMERS.index("#endif")]
    if production.count("string name_esc = escape_str(name);") != 2:
        raise AssertionError("timer set and get paths must both escape timer names")
    if production.count("name_esc.c_str()") != 2:
        raise AssertionError("timer queries must use escaped timer names")


def test_command_filters_escape_without_post_escape_truncation() -> None:
    auction_list = function_body(AUCTION, "bool auction_list(P_char ch, char *args)", "bool auction_info(")
    require("string seller_filter_esc = escape_str(list_arg);", auction_list,
            "auction player filters must use dynamic SQL escaping")
    if "seller_name like '%.100s'" in auction_list:
        raise AssertionError("auction filters must not truncate after SQL escaping")
    if "mysql_real_escape_string(DB, buff, list_arg" in auction_list:
        raise AssertionError("auction list filters must not reuse the shared display buffer for SQL escaping")

    add = function_body(WHITELIST, "bool add_to_whitelist", "bool remove_from_whitelist")
    for field in ("admin", "player", "pattern", "description"):
        if not re.search(rf"string\s+{field}_esc\s*=\s*escape_str", add):
            raise AssertionError(f"whitelist inserts must escape {field}")
    remove = function_body(WHITELIST, "bool remove_from_whitelist", "void do_whitelist_help")
    require("string pattern_esc = escape_str(pattern);", remove,
            "whitelist deletes must escape the host pattern")


def test_player_identity_sinks_escape_before_sql() -> None:
    require("string killer_name_esc = escape_str(GET_NAME(killer));", FIGHT,
            "hardcore death persistence must escape the killer name")
    require("killer_name_esc.c_str(), GET_PID(ch)", FIGHT,
            "killed_by update must use the escaped killer name")

    race_sig = "int sql_find_racewar_for_ip(char *ip, int *racewar_side)"
    race_start = SQL.rindex(race_sig)
    race = SQL[race_start:SQL.index("void perform_wiki_search", race_start)]
    if not re.search(r"string\s+ip_esc\s*=\s*escape_str\(ip\);", race):
        raise AssertionError("racewar IP lookup must escape the descriptor address")
    require("ip_esc.c_str()", race,
            "racewar IP query must use the escaped address")

    connect_ip = function_body(SQL, "void sql_connectIP(P_char ch)", "void sql_world_quest_finished(")
    require("string host_esc = escape_str(ch->desc->host);", connect_ip,
            "connection history must escape descriptor host text")
    require("host_esc.c_str()", connect_ip,
            "connection history update must use the escaped host")

    quest = function_body(SQL, "void sql_world_quest_finished(P_char ch, P_obj reward)", "int sql_world_quest_can_do_another(")
    require("string player_name_esc = escape_str(GET_NAME(ch));", quest,
            "world quest history must escape the player name")
    require("player_name_esc.c_str()", quest,
            "world quest insert must use the escaped player name")

    ledger = function_body(ASSOCS, "void Guild::write_transaction_to_ledger", "void Guild::deposit(")
    require("string transaction_esc = escape_str", ledger,
            "guild ledger text must be composed then escaped")
    require("transaction_esc.c_str()", ledger,
            "guild ledger insert must use the escaped transaction")


def test_shared_message_sinks_use_dynamic_escaping() -> None:
    offline = function_body(SQL, "void send_to_pid_offline(const char *msg, int pid)", "void send_offline_messages(")
    require("string escaped_msg = escape_str(msg);", offline,
            "offline messages must use dynamically sized SQL escaping")
    if "mysql_real_escape_string" in offline:
        raise AssertionError("offline messages must not escape into a same-size fixed buffer")

    sql_log_sig = "void sql_log(P_char ch, char *kind, char *format, ...)"
    sql_log_start = SQL.rindex(sql_log_sig)
    sql_log = SQL[sql_log_start:SQL.index("bool get_zone_info(", sql_log_start)]
    for field in ("kind", "ip", "player_name", "message"):
        if not re.search(rf"string\s+{field}_esc\s*=\s*escape_str", sql_log):
            raise AssertionError(f"sql_log must dynamically escape {field}")
    if "ip_buff[15]" in sql_log or "snprintf(ip_buff, 50" in sql_log:
        raise AssertionError("sql_log must not write a hostname through the undersized legacy IP buffer")


def test_auction_identity_writes_escape_names() -> None:
    offer = function_body(AUCTION, "bool auction_offer(P_char ch, char *args)", "bool auction_list(")
    require("string seller_name_esc = escape_str(GET_NAME(ch));", offer,
            "auction offers must escape the persisted seller name")
    if offer.count("seller_name_esc.c_str()") != 2:
        raise AssertionError("both modern and legacy auction inserts must use the escaped seller name")

    bid = function_body(AUCTION, "bool auction_bid(P_char ch, char *args)", "bool auction_pickup(")
    require("string bidder_name_esc = escape_str(GET_NAME(ch));", bid,
            "auction bids must escape the persisted bidder name")
    if bid.count("bidder_name_esc.c_str()") != 4:
        raise AssertionError("all auction winner and bid-history writes must use the escaped bidder name")


def test_escape_str_sizes_for_mysql_worst_case() -> None:
    production = SQL[SQL.index("/* Escapes a string. */"):]
    body = function_body(production, "string escape_str(const char *str)", "void sql_populate_lookup_tables()")
    if "static char buff[MAX_STRING_LENGTH]" in body:
        raise AssertionError("escape_str must not use a fixed buffer for potentially doubled SQL data")
    if not re.search(r"string\s+escaped\(len \* 2 \+ 1", body):
        raise AssertionError("escape_str must allocate MySQL's worst-case escaped length")
    require("escaped.resize(escaped_len);", body,
            "escape_str must return only bytes written by mysql_real_escape_string")


if __name__ == "__main__":
    test_auction_fallback_uses_escaped_object_blob()
    test_private_chest_queries_do_not_truncate_escaped_input()
    test_shared_text_keys_escape_before_queries()
    test_command_filters_escape_without_post_escape_truncation()
    test_player_identity_sinks_escape_before_sql()
    test_shared_message_sinks_use_dynamic_escaping()
    test_auction_identity_writes_escape_names()
    test_escape_str_sizes_for_mysql_worst_case()
    print("SQL input hardening contracts passed")
