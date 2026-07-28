#!/usr/bin/env python3
"""Static contract checks for the specialization data module.

This intentionally has no build-system or third-party dependency.  It is safe
for contributors to copy into a checkout and run before changing a spec.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
DATA = SRC / "specialization_data.c"
HEADER = SRC / "specialization_data.h"
MAKEFILE = SRC / "Makefile"

errors = []

def require(condition, message):
    if not condition:
        errors.append(message)

require(DATA.exists(), "missing src/specialization_data.c")
require(HEADER.exists(), "missing src/specialization_data.h")
require("specialization_data.o" in MAKEFILE.read_text(),
        "src/Makefile does not link specialization_data.o")

if DATA.exists():
    data = DATA.read_text()
    name_rows = re.findall(r"^\s*\{.*// .+\n", data, re.MULTILINE)
    allowed_rows = re.findall(r"\{\s*RACE_[^\n]+\}", data)
    require(data.count("const char *specdata[][MAX_SPEC] = {") == 1,
            "specdata must have exactly one definition")
    require(data.count("struct allowed_race_spec_struct") == 1,
            "allowed_race_specs must have exactly one definition")
    require(len(allowed_rows) >= 303,
            f"expected at least 303 legacy race/spec rows, found {len(allowed_rows)}")
    require("static_assert(sizeof(specdata)" in data,
            "specdata dimension static_assert is missing")
    for symbol in (
        "specialization_name_by_index",
        "specialization_name",
        "specialization_exists_by_index",
        "specialization_exists",
        "specialization_is_active",
        "specialization_matches",
        "specialization_is_allowed_race_spec",
    ):
        require(f"{symbol}(" in data, f"missing implementation for {symbol}")

if HEADER.exists():
    header = HEADER.read_text()
    for symbol in (
        "specialization_name_by_index",
        "specialization_name",
        "specialization_exists_by_index",
        "specialization_exists",
        "specialization_is_active",
        "specialization_matches",
        "specialization_is_allowed_race_spec",
    ):
        require(f"{symbol}(" in header, f"missing public declaration for {symbol}")

for path in SRC.glob("*.c"):
    if path.name in {"common.c", "specialization_data.c"}:
        continue
    text = path.read_text()
    require("specdata[" not in text,
            f"{path.name} directly accesses specdata; use specialization APIs")
    require("GET_SPEC_NAME" not in text,
            f"{path.name} uses removed GET_SPEC_NAME compatibility macro")

if errors:
    print("specialization modularity checks: FAIL")
    for error in errors:
        print(f"- {error}")
    sys.exit(1)

print("specialization modularity checks: PASS")
print("- specialization data has one owner")
print("- legacy race/spec table has 303 preserved rows")
print("- public lookup/predicate APIs are declared and implemented")
print("- no unrelated C file directly accesses specdata")
