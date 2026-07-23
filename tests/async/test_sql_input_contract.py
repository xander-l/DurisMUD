#!/usr/bin/env python3
"""Source contracts for player-controlled SQL boundary hardening."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AUCTION = (ROOT / "src" / "auction_houses.c").read_text()
SQL = (ROOT / "src" / "sql.c").read_text()


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
    test_auction_identity_writes_escape_names()
    test_escape_str_sizes_for_mysql_worst_case()
    print("SQL input hardening contracts passed")
