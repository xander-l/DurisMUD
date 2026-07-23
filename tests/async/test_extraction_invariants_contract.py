"""
Contract: Character/object extraction invariants (Priority 6)

Verifies that extract_char and extract_obj properly maintain linked-list
invariants and guard against double-extraction:
1. extract_char sets ch->next = NULL after unlink from character_list
2. extract_char returns early on 'not in character_list' error
3. extract_obj guards against double-extract (prev==NULL && next==NULL && not at head)
"""

import re
import sys
import os

ROOT = os.path.join(os.path.dirname(__file__), "..", "..", "src")

def read_file_lines(path):
    full = os.path.join(ROOT, path)
    with open(full) as f:
        return f.readlines()

def find_line(needle, lines, start=0):
    for i in range(start, len(lines)):
        if needle in lines[i]:
            return i
    return -1

def run():
    errors = 0
    lines = read_file_lines("handler.c")

    print("=== P6: Extraction Invariants Contract ===\n")

    # 1. extract_char: ch->next = NULL after head unlink
    print("1. extract_char: ch->next = NULL after head unlink...")
    # Look for "character_list = ch->next;" followed by "ch->next = NULL;"
    found_head_next_null = False
    for i, line in enumerate(lines):
        if "character_list = ch->next;" in line:
            # Check a few lines ahead for ch->next = NULL
            for j in range(i, min(i+5, len(lines))):
                if "ch->next = NULL" in lines[j]:
                    found_head_next_null = True
                    print(f"   OK: ch->next = NULL after head unlink at line {j+1}")
                    break

    if not found_head_next_null:
        # The unlink might be in a block, check differently
        # Find lines after "if (ch == character_list)"
        at_char_list = False
        for i, line in enumerate(lines):
            if "ch == character_list" in line:
                at_char_list = True
                continue
            if at_char_list and "ch->next = NULL" in line:
                found_head_next_null = True
                print(f"   OK: ch->next = NULL in character_list branch at line {i+1}")
                break

    if not found_head_next_null:
        print("   FAIL: No ch->next = NULL found near character_list head unlink")
        errors += 1

    # 2. extract_char: ch->next = NULL after non-head unlink
    print("2. extract_char: ch->next = NULL after non-head unlink...")
    found_nonhead_next_null = False
    for i, line in enumerate(lines):
        if "k->next = ch->next" in line:
            for j in range(i, min(i+5, len(lines))):
                if "ch->next = NULL" in lines[j]:
                    found_nonhead_next_null = True
                    print(f"   OK: ch->next = NULL after k->next = ch->next at line {j+1}")
                    break
            break

    if not found_nonhead_next_null:
        print("   FAIL: No ch->next = NULL found after k->next = ch->next")
        errors += 1

    # 3. extract_char: return on 'not in character_list'
    print("3. extract_char: return on 'not in character_list'...")
    found_return = False
    for i, line in enumerate(lines):
        if "Char not in character_list" in line:
            for j in range(i, min(i+5, len(lines))):
                if "return;" in lines[j]:
                    found_return = True
                    print(f"   OK: return after not-in-character_list at line {j+1}")
                    break
            break

    if not found_return:
        print("   FAIL: No return after 'Char not in character_list' log")
        errors += 1

    # 4. extract_obj: double-extract guard
    print("4. extract_obj: double-extract guard...")
    found_guard = False
    for i, line in enumerate(lines):
        if "double extraction" in line:
            found_guard = True
            print(f"   OK: double-extract guard at line {i+1}")
            break

    if not found_guard:
        print("   FAIL: No double-extract guard in extract_obj")
        errors += 1

    # 5. extract_obj: obj->prev = NULL and obj->next = NULL
    print("5. extract_obj: prev/next NULL after unlink...")
    found_prev_null = False
    found_next_null = False
    for i, line in enumerate(lines):
        if "obj->prev = NULL" in line:
            found_prev_null = True
        if "obj->next = NULL" in line:
            found_next_null = True

    if found_prev_null and found_next_null:
        print("   OK: obj->prev = NULL and obj->next = NULL found")
    else:
        if not found_prev_null:
            print("   FAIL: obj->prev = NULL not found")
            errors += 1
        if not found_next_null:
            print("   FAIL: obj->next = NULL not found")
            errors += 1

    print(f"\n=== RESULT: {'PASS' if errors == 0 else f'FAIL ({errors} error(s))'} ===")
    return errors == 0

if __name__ == "__main__":
    success = run()
    sys.exit(0 if success else 1)
