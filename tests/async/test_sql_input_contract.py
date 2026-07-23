#!/usr/bin/env python3
"""Source contracts for player-controlled SQL boundary hardening."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AUCTION = (ROOT / "src" / "auction_houses.c").read_text()


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


if __name__ == "__main__":
    test_auction_fallback_uses_escaped_object_blob()
    print("SQL input hardening contracts passed")
