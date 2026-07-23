#!/usr/bin/env python3
"""Source contracts for player-controlled SQL boundary hardening."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AUCTION = (ROOT / "src" / "auction_houses.c").read_text()
SQL = (ROOT / "src" / "sql.c").read_text()
FIGHT = (ROOT / "src" / "fight.c").read_text()
ASSOCS = (ROOT / "src" / "assocs.c").read_text()


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


def test_player_identity_sinks_escape_before_sql() -> None:
    require("string killer_name_esc = escape_str(GET_NAME(killer));", FIGHT,
            "hardcore death persistence must escape the killer name")
    require("killer_name_esc.c_str(), GET_PID(ch)", FIGHT,
            "killed_by update must use the escaped killer name")

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
    test_player_identity_sinks_escape_before_sql()
    test_shared_message_sinks_use_dynamic_escaping()
    test_auction_identity_writes_escape_names()
    test_escape_str_sizes_for_mysql_worst_case()
    print("SQL input hardening contracts passed")
