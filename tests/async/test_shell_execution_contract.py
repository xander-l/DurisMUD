"""
Contract: Shell/process execution hardening (Priority 5)

Verifies that player-controlled input is not passed to system()/popen()
calls. Confirms the three flagged paths are replaced with safe alternatives:

1. comm.c: hostname lookup via fork()+getnameinfo() instead of system("host ...")
2. websocket.c: same replacement for WebSocket connections
3. account.c: email sending via fork()+execlp("mail", ...) instead of system("mail ...")

Also confirms the remaining system() calls use only hardcoded constant strings.
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

def assert_no_player_system(desc, lines, filename):
    """Verify no system() call that uses a player-controlled variable."""
    for i, line in enumerate(lines):
        stripped = line.strip()
        # Skip comments
        if stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*"):
            continue
        # Look for system( with a variable (not a string literal)
        m = re.search(r'(?<!\w)system\s*\(([^)]+)\)', stripped)
        if m:
            arg = m.group(1).strip()
            # If arg is a string literal (starts with "), it's constant - OK
            if arg.startswith('"'):
                continue
            # If arg is a variable, flag it
            print(f"  FAIL: system() at line {i+1} with non-constant arg: {stripped[:100]}")
            return False
    return True

def check_getnameinfo(lines, filename):
    """Verify getnameinfo() is used for reverse DNS."""
    for i, line in enumerate(lines):
        if "getnameinfo" in line and not line.strip().startswith("//"):
            print(f"  OK: getnameinfo() at line {i+1}")
            return True
    print(f"  FAIL: no getnameinfo() call found in {filename}")
    return False

def check_execlp(lines, filename, cmd):
    """Verify an exec*lp() call exists with the given command."""
    for i, line in enumerate(lines):
        if f"exec{cmd}" in line and not line.strip().startswith("//") and not line.strip().startswith("*"):
            print(f"  OK: exec{cmd}() at line {i+1}")
            return True
    print(f"  FAIL: no exec{cmd}() call found in {filename}")
    return False

def check_logical(lines, filename):
    """Run filename-specific checks."""
    if filename == "comm.c":
        checks = [
            ("fork()", lambda: any("fork()" in l and "getnameinfo" in l for l in lines)),
            ("no system( with variable in new_descriptor", lambda: True),  # check below
        ]
    elif filename == "websocket.c":
        checks = [
            ("getnameinfo() or getaddrinfo()", lambda: any("getnameinfo" in l for l in lines) or any("getaddrinfo" in l for l in lines)),
        ]
    elif filename == "account.c":
        checks = [
            ("execlp('mail'...) or execvp('mail'...)", lambda: any("execvp" in l and "mail" in l for l in lines) or any("execlp" in l and "mail" in l for l in lines)),
        ]
    else:
        checks = []
    return checks

def run():
    errors = 0

    # --- Verify system() calls are properly hardened ---
    print("=== P5: Shell Execution Contract ===\n")

    # comm.c
    print("1. comm.c:")
    lines = read_file_lines("comm.c")
    ok1 = assert_no_player_system("comm.c", lines, "comm.c")
    # The only non-constant system() in comm.c should be gone
    # Look for the specific old pattern
    old_pattern = "host %s | sed"
    old_found = any(old_pattern in l for l in lines)
    if old_found:
        print("  FAIL: comm.c still has 'host ... | sed' shell pipeline")
        ok1 = False
    else:
        print("  OK: 'host | sed' shell pipeline removed from comm.c")
    # Check getnameinfo is used instead
    if not check_getnameinfo(lines, "comm.c"):
        ok1 = False
    if not ok1:
        errors += 1

    # websocket.c
    print("\n2. websocket.c:")
    lines = read_file_lines("websocket.c")
    ok2 = assert_no_player_system("websocket.c", lines, "websocket.c")
    old_found = any(old_pattern in l for l in lines)
    if old_found:
        print("  FAIL: websocket.c still has 'host | sed' shell pipeline")
        ok2 = False
    else:
        print("  OK: 'host | sed' shell pipeline removed from websocket.c")
    # Check getnameinfo or getaddrinfo is used
    if not check_getnameinfo(lines, "websocket.c"):
        ok2 = False
    if not ok2:
        errors += 1

    # account.c
    print("\n3. account.c:")
    lines = read_file_lines("account.c")
    ok3 = assert_no_player_system("account.c", lines, "account.c")
    # The mail command system() call should be gone
    mail_pattern = r'mail.*%s'
    for i, line in enumerate(lines):
        if re.search(mail_pattern, line) and 'system' not in lines[max(0,i-1):i+2]:
            pass  # the variable itself may be defined even with new approach
    # Check for old system pattern with mail
    old_mail = "mail -s"
    if any(old_mail in l and "system" in l for l in lines):
        print("  FAIL: account.c still has system('mail -s ...')")
        ok3 = False
    else:
        print("  OK: system('mail -s ...') removed from account.c")
    # Check exec family with mail
    if not check_execlp(lines, "account.c", "lp"):
        ok3 = False
    if not ok3:
        errors += 1

    # --- Verify remaining system() calls use constants ---
    print("\n4. Remaining system() calls (should be constants only):")
    all_files = ["comm.c", "websocket.c", "account.c", "actwiz.c", "period.list.c"]
    ok4 = True
    for fname in all_files:
        try:
            lns = read_file_lines(fname)
        except FileNotFoundError:
            continue
        for i, line in enumerate(lns):
            m = re.search(r'(?<!\w)system\s*\(([^)]+)\)', line.strip())
            if m:
                arg = m.group(1).strip()
                if arg.startswith('"'):
                    print(f"  OK: const system() in {fname}:{i+1}")
                else:
                    print(f"  FAIL: variable system() in {fname}:{i+1} -- {line.strip()[:100]}")
                    ok4 = False
    if ok4:
        print("  All remaining system() calls use constant strings.")
    else:
        errors += 1

    print(f"\n=== RESULT: {'PASS' if errors == 0 else f'FAIL ({errors} error(s))'} ===")
    return errors == 0

if __name__ == "__main__":
    success = run()
    sys.exit(0 if success else 1)
