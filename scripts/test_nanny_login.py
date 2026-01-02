#!/usr/bin/env python3
"""
Nanny login regression harness.

Attack vectors covered:
- Menu parsing abuse: non-numeric, negative, zero, float, whitespace/escape sequences,
  oversized lines, and rapid-fire sequences at the initial menu prompt.
- Name parsing abuse: empty/whitespace-only names, overlong names, embedded control/null
  bytes, backspaces/tabs/CRLF, high-ASCII and UTF-8 bytes, emoji, and various punctuation
  (quotes, slashes, tilde, percent, pipes, brackets/braces).
- Telnet/IAC sequences: short IAC, repeated IAC bytes, subnegotiation payloads, and
  specific TELNET negotiation commands (WONT ECHO, DO TERMINAL-TYPE, GA, EOR).
- Password handling: control/null bytes, CRLF, UTF-8 bytes, empty passwords, chunked
  delivery, and large (4K) password payloads.
- Reconnect flow: attempts to log in twice with the same name to exercise reconnect
  prompts and state transitions.

State flows exercised:
- CON_GET_TERM/CON_GET_ACCT_NAME/CON_NAME via menu selection and name entry.
- CON_PWD_NORM/CON_PWD_GET via password prompts for existing names.
- CON_NAME_CONF/CON_MAIN_MENU/CON_RMOTD/CON_PLAYING via reconnect attempts.
- Telnet negotiation bytes are injected during early menu/name phases to stress parser
  handling around CON_GET_TERM and CON_NAME.

Harness considerations:
- Each vector is executed in sequence and failure is defined as server process exit
  before/after the input; input handling issues that do not crash are recorded as ok.
- Connection errors during fuzzing are treated as early termination of that vector so
  subsequent vectors can still run.

Fuzzing:
- Fuzzing is applied per vector by generating mutated variants of the base payload(s)
  (bit flips, truncation, repetition, random bytes, insertion/deletion).
- Each vector runs the base input plus N mutated variants; N is configurable via
  --fuzz-iterations and uses a deterministic RNG seed via --fuzz-seed.
"""
import argparse
import os
import shutil
import socket
import subprocess
import random
import tempfile
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
FUZZ_ITERATIONS = 0
FUZZ_RNG = random.Random(1337)
MAX_FUZZ_BYTES = 8192


def set_fuzzing(iterations: int, seed: int) -> None:
    global FUZZ_ITERATIONS, FUZZ_RNG
    FUZZ_ITERATIONS = max(0, iterations)
    FUZZ_RNG = random.Random(seed)


def mutate_payload(payload: bytes) -> bytes:
    if not payload:
        return payload
    choice = FUZZ_RNG.choice(
        ["flip", "truncate", "repeat", "random", "insert", "delete"]
    )
    if choice == "flip":
        data = bytearray(payload)
        flips = min(4, len(data))
        for _ in range(flips):
            idx = FUZZ_RNG.randrange(len(data))
            data[idx] ^= 1 << FUZZ_RNG.randrange(8)
        return bytes(data)
    if choice == "truncate":
        new_len = FUZZ_RNG.randrange(len(payload) + 1)
        return payload[:new_len]
    if choice == "repeat":
        repeats = FUZZ_RNG.randint(2, 4)
        repeated = payload * repeats
        return repeated[:MAX_FUZZ_BYTES]
    if choice == "random":
        new_len = FUZZ_RNG.randint(1, min(MAX_FUZZ_BYTES, max(1, len(payload))))
        return bytes(FUZZ_RNG.getrandbits(8) for _ in range(new_len))
    if choice == "insert":
        insert_len = FUZZ_RNG.randint(1, min(16, MAX_FUZZ_BYTES))
        insert_bytes = bytes(FUZZ_RNG.getrandbits(8) for _ in range(insert_len))
        idx = FUZZ_RNG.randrange(len(payload) + 1)
        merged = payload[:idx] + insert_bytes + payload[idx:]
        return merged[:MAX_FUZZ_BYTES]
    if choice == "delete":
        if len(payload) == 1:
            return b""
        start = FUZZ_RNG.randrange(len(payload) - 1)
        end = FUZZ_RNG.randrange(start + 1, len(payload))
        return payload[:start] + payload[end:]
    return payload


def iter_fuzzed_payloads(payload: bytes) -> list[bytes]:
    variants = [payload]
    for _ in range(FUZZ_ITERATIONS):
        variants.append(mutate_payload(payload))
    return variants


def iter_fuzzed_actions(actions: list[tuple[bytes, float]]) -> list[list[tuple[bytes, float]]]:
    variants = [actions]
    for _ in range(FUZZ_ITERATIONS):
        fuzzed: list[tuple[bytes, float]] = []
        for payload, delay in actions:
            fuzzed.append((mutate_payload(payload), delay))
        variants.append(fuzzed)
    return variants


def find_binary(path_hint: str | None) -> Path:
    if path_hint:
        candidate = Path(path_hint).expanduser()
        if candidate.is_file():
            return candidate.resolve()
        raise FileNotFoundError(f"Binary not found: {candidate}")
    for candidate in [REPO_ROOT / "dms", REPO_ROOT / "src" / "dms_new"]:
        if candidate.is_file():
            return candidate.resolve()
    raise FileNotFoundError("Could not find dms binary; use --bin to specify.")


def make_data_dir(mini_mode: bool) -> Path:
    temp_dir = Path(tempfile.mkdtemp(prefix="duris-test-data-"))
    for entry in REPO_ROOT.iterdir():
        if entry.name.startswith("."):
            continue
        if entry.name in {"logs", "Players", "Accounts", "areas"}:
            continue
        target = temp_dir / entry.name
        try:
            os.symlink(entry, target, target_is_directory=entry.is_dir())
        except FileExistsError:
            continue
    if mini_mode:
        areas_target = temp_dir / "areas"
        areas_target.mkdir(exist_ok=True)
        mini_source = REPO_ROOT / "areas_mini"
        if mini_source.exists():
            for entry in mini_source.iterdir():
                try:
                    os.symlink(entry, areas_target / entry.name, target_is_directory=entry.is_dir())
                except FileExistsError:
                    continue
    else:
        try:
            os.symlink(REPO_ROOT / "areas", temp_dir / "areas", target_is_directory=True)
        except FileExistsError:
            pass
    for subdir in [temp_dir / "logs" / "log", temp_dir / "logs" / "old-logs", temp_dir / "logs" / "player-log"]:
        subdir.mkdir(parents=True, exist_ok=True)
    players_dir = temp_dir / "Players"
    players_dir.mkdir(exist_ok=True)
    for subdir in ["Corpses", "SavedItems", "ShopKeepers"]:
        (players_dir / subdir).mkdir(exist_ok=True)
    (temp_dir / "Accounts").mkdir(exist_ok=True)
    return temp_dir


def start_server(
    binary: Path, data_dir: Path, port: int, mini_mode: bool
) -> tuple[subprocess.Popen, Path, Path, object, object]:
    cmd = [str(binary)]
    if mini_mode:
        cmd.append("-m")
    cmd.extend(["-d", str(data_dir), str(port)])
    stdout_path = data_dir / "server_stdout.log"
    stderr_path = data_dir / "server_stderr.log"
    stdout_file = open(stdout_path, "wb")
    stderr_file = open(stderr_path, "wb")
    process = subprocess.Popen(
        cmd,
        cwd=str(data_dir),
        stdout=stdout_file,
        stderr=stderr_file,
        start_new_session=True,
    )
    return process, stdout_path, stderr_path, stdout_file, stderr_file


def wait_for_port(port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.5):
                return
        except OSError:
            time.sleep(0.1)
    raise TimeoutError(f"Server did not open port {port} within {timeout} seconds.")


def allocate_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def recv_until(sock: socket.socket, markers: tuple[str, ...], timeout: float) -> bytes:
    buffer = b""
    deadline = time.time() + timeout
    sock.settimeout(0.5)
    while time.time() < deadline:
        try:
            chunk = sock.recv(4096)
        except ConnectionResetError:
            break
        except socket.timeout:
            continue
        if not chunk:
            break
        buffer += chunk
        lower = buffer.decode("utf-8", errors="ignore").lower()
        if any(marker in lower for marker in markers):
            break
    return buffer


def simple_exchange(port: int, actions: list[tuple[bytes, float]]) -> bytes:
    data = b""
    with socket.create_connection(("127.0.0.1", port), timeout=2.0) as sock:
        sock.settimeout(1.0)
        data += recv_until(sock, ("name", "ansi", "color"), 1.5)
        for payload, delay in actions:
            if payload:
                try:
                    sock.sendall(payload)
                except (BrokenPipeError, ConnectionResetError):
                    break
            if delay:
                time.sleep(delay)
            data += recv_until(sock, (), 0.5)
    return data


def run_exchanges(port: int, actions: list[tuple[bytes, float]]) -> None:
    for variant in iter_fuzzed_actions(actions):
        try:
            simple_exchange(port, variant)
        except ConnectionRefusedError:
            return


def run_password_flow(port: int, name_payload: bytes, password_payload: bytes) -> None:
    for name_variant in iter_fuzzed_payloads(name_payload):
        for password_variant in iter_fuzzed_payloads(password_payload):
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=2.0) as sock:
                    recv_until(sock, ("name", "ansi", "color"), 1.0)
                    sock.sendall(b"1\n")
                    time.sleep(0.2)
                    sock.sendall(name_variant + b"\n")
                    data = recv_until(sock, ("password",), 1.0)
                    if b"password" in data.lower():
                        sock.sendall(password_variant + b"\n")
                        recv_until(sock, (), 0.5)
            except ConnectionRefusedError:
                return


def test_overlong_terminal_id(port: int) -> tuple[bool, str]:
    payload = b"1 " + (b"A" * 5000) + b"\n"
    run_exchanges(port, [(payload, 0.2)])
    return True, "sent overlong terminal id (fuzzed)"


def test_non_ascii_login(port: int) -> tuple[bool, str]:
    run_password_flow(port, b"Te\xffst", b"pa\xffss")
    return True, "sent non-ascii login bytes (fuzzed)"


def test_empty_name(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"\n", 0.2)])
    return True, "sent empty name (fuzzed)"

def test_overlong_name(port: int) -> tuple[bool, str]:
    payload = b"1\n" + (b"N" * 1024) + b"\n"
    run_exchanges(port, [(payload, 0.2)])
    return True, "sent overlong name (fuzzed)"

def test_overlong_password(port: int) -> tuple[bool, str]:
    run_password_flow(port, b"BufferTest", b"P" * 1024)
    return True, "sent overlong password (fuzzed)"


def test_short_iac_sequence(port: int) -> tuple[bool, str]:
    actions = [(b"1\n", 0.2), (b"RaceTest\n", 0.2), (b"y\n", 0.2), (b"\xff\n", 0.2)]
    run_exchanges(port, actions)
    return True, "sent short IAC sequence (fuzzed)"


def test_menu_garbage(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"notanumber\n", 0.2)])
    return True, "sent non-numeric menu input (fuzzed)"


def test_menu_negative(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"-1\n", 0.2)])
    return True, "sent negative menu choice (fuzzed)"


def test_menu_zero(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"0\n", 0.2)])
    return True, "sent zero menu choice (fuzzed)"


def test_menu_float(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1.5\n", 0.2)])
    return True, "sent float menu choice (fuzzed)"


def test_menu_whitespace(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b" 1 \n", 0.2)])
    return True, "sent whitespace-padded menu choice (fuzzed)"


def test_menu_escape_sequence(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"\x1b[31m1\x1b[0m\n", 0.2)])
    return True, "sent escape sequence in menu choice (fuzzed)"


def test_menu_large_number(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"999999999\n", 0.2)])
    return True, "sent large menu choice (fuzzed)"


def test_menu_long_line(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1" * 4096 + b"\n", 0.2)])
    return True, "sent long menu line (fuzzed)"


def test_name_with_null(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Null\x00Byte\n", 0.2)])
    return True, "sent null byte in name (fuzzed)"


def test_name_with_control_bytes(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"\x01\x02\x03\n", 0.2)])
    return True, "sent control bytes in name (fuzzed)"


def test_name_whitespace_only(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"   \n", 0.2)])
    return True, "sent whitespace-only name (fuzzed)"


def test_name_with_backspaces(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Back\b\b\b\n", 0.2)])
    return True, "sent backspace characters in name (fuzzed)"


def test_name_with_tabs(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Tab\tName\n", 0.2)])
    return True, "sent tabs in name (fuzzed)"


def test_name_with_crlf(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\r\n", 0.2), (b"CrLf\r\n", 0.2)])
    return True, "sent CRLF in name (fuzzed)"


def test_name_utf8(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), ("N\xc3\xa1me\n".encode("utf-8"), 0.2)])
    return True, "sent utf-8 name (fuzzed)"


def test_name_with_del(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Del\x7fName\n", 0.2)])
    return True, "sent DEL byte in name (fuzzed)"


def test_name_with_slash(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Name/Slash\n", 0.2)])
    return True, "sent slash in name (fuzzed)"


def test_name_with_quotes(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"\"Quoted\"\n", 0.2)])
    return True, "sent quotes in name (fuzzed)"


def test_name_with_tilde(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Name~Tilde\n", 0.2)])
    return True, "sent tilde in name (fuzzed)"


def test_name_with_percent(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Name%Percent\n", 0.2)])
    return True, "sent percent in name (fuzzed)"


def test_name_with_pipe(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Name|Pipe\n", 0.2)])
    return True, "sent pipe in name (fuzzed)"


def test_name_with_brackets(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Name[Bracket]\n", 0.2)])
    return True, "sent brackets in name (fuzzed)"


def test_name_with_braces(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Name{Brace}\n", 0.2)])
    return True, "sent braces in name (fuzzed)"


def test_name_with_emoji(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), ("Name\U0001f600\n".encode("utf-8"), 0.2)])
    return True, "sent emoji in name (fuzzed)"


def test_name_leading_spaces(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"   Lead\n", 0.2)])
    return True, "sent leading spaces in name (fuzzed)"


def test_name_trailing_spaces(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"Trail   \n", 0.2)])
    return True, "sent trailing spaces in name (fuzzed)"


def test_name_partial_line(port: int) -> tuple[bool, str]:
    actions = [(b"1\n", 0.2), (b"A" * 2048, 0.2), (b"\n", 0.2)]
    run_exchanges(port, actions)
    return True, "sent long name without newline then newline (fuzzed)"


def test_iac_double(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"\xff\xff\xff\xff\n", 0.2)])
    return True, "sent repeated IAC bytes (fuzzed)"


def test_iac_subnegotiation(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"\xff\xfa\x18\x01\xff\xf0\n", 0.2)])
    return True, "sent IAC subnegotiation payload (fuzzed)"


def test_password_control_bytes(port: int) -> tuple[bool, str]:
    run_password_flow(port, b"ControlPass", b"\x7f\x00\x1b")
    return True, "sent control bytes in password (fuzzed)"


def test_password_overlong_4096(port: int) -> tuple[bool, str]:
    run_password_flow(port, b"LongPass", b"P" * 4096)
    return True, "sent 4096-byte password (fuzzed)"


def test_password_with_null(port: int) -> tuple[bool, str]:
    run_password_flow(port, b"NullPass", b"pw\x00rd")
    return True, "sent null byte in password (fuzzed)"


def test_password_with_crlf(port: int) -> tuple[bool, str]:
    run_password_flow(port, b"CrLfPass", b"pw\r\n")
    return True, "sent CRLF in password (fuzzed)"


def test_password_utf8(port: int) -> tuple[bool, str]:
    run_password_flow(port, b"Utf8Pass", "p\xc3\xa1ss".encode("utf-8"))
    return True, "sent utf-8 password (fuzzed)"


def test_password_empty(port: int) -> tuple[bool, str]:
    run_password_flow(port, b"EmptyPass", b"")
    return True, "sent empty password (fuzzed)"


def test_password_partial_then_newline(port: int) -> tuple[bool, str]:
    for password_variant in iter_fuzzed_payloads(b"partial"):
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=2.0) as sock:
                recv_until(sock, ("name", "ansi", "color"), 1.0)
                sock.sendall(b"1\n")
                time.sleep(0.2)
                sock.sendall(b"PartialPass\n")
                data = recv_until(sock, ("password",), 1.0)
                if b"password" in data.lower():
                    sock.sendall(password_variant)
                    time.sleep(0.2)
                    sock.sendall(b"\n")
                    recv_until(sock, (), 0.5)
        except ConnectionRefusedError:
            break
    return True, "sent password in chunks (fuzzed)"


def test_multiple_newlines(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"\n\n\n", 0.2)])
    return True, "sent multiple newlines (fuzzed)"


def test_rapid_fire_inputs(port: int) -> tuple[bool, str]:
    actions = [
        (b"1\n", 0.0),
        (b"Rapid\n", 0.0),
        (b"y\n", 0.0),
        (b"1\n", 0.0),
    ]
    run_exchanges(port, actions)
    return True, "sent rapid-fire inputs (fuzzed)"


def test_telnet_wont_echo(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"\xff\xfc\x01\n", 0.2)])
    return True, "sent IAC WONT ECHO (fuzzed)"


def test_telnet_do_terminal(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"\xff\xfd\x18\n", 0.2)])
    return True, "sent IAC DO TERMINAL-TYPE (fuzzed)"


def test_telnet_ga(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"\xff\xf9\n", 0.2)])
    return True, "sent IAC GA (fuzzed)"


def test_telnet_eor(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"\xff\xef\n", 0.2)])
    return True, "sent IAC EOR (fuzzed)"


def test_high_ascii_name(port: int) -> tuple[bool, str]:
    run_exchanges(port, [(b"1\n", 0.2), (b"\x80\xfe\xff\n", 0.2)])
    return True, "sent high-ascii name bytes (fuzzed)"


def test_reconnect_flow(port: int) -> tuple[bool, str]:
    name = b"ReconnectTester"
    run_exchanges(port, [(b"1\n", 0.2), (name + b"\n", 0.2)])
    with socket.create_connection(("127.0.0.1", port), timeout=2.0) as first:
        recv_until(first, ("name", "ansi", "color"), 1.0)
        first.sendall(b"1\n")
        time.sleep(0.2)
        first.sendall(name + b"\n")
        time.sleep(0.2)
        recv_until(first, ("confirm", "right", "yes", "new"), 1.0)
        first.sendall(b"y\n")
        time.sleep(0.2)
        with socket.create_connection(("127.0.0.1", port), timeout=2.0) as second:
            recv_until(second, ("name", "ansi", "color"), 1.0)
            second.sendall(b"1\n")
            time.sleep(0.2)
            second.sendall(name + b"\n")
            data = recv_until(second, ("reconnect", "already", "playing"), 1.5)
            lower = data.decode("utf-8", errors="ignore").lower()
            if "reconnect" in lower or "already" in lower:
                second.sendall(b"y\n")
                recv_until(second, (), 0.5)
                return True, "attempted reconnect"
    return True, "reconnect prompt not observed"


def run_suite(binary: Path, mini_mode: bool, timeout: float) -> tuple[bool, list[str]]:
    data_dir = make_data_dir(mini_mode)
    port = allocate_port()
    server, stdout_path, stderr_path, stdout_file, stderr_file = start_server(
        binary, data_dir, port, mini_mode
    )
    ok = True
    details: list[str] = []
    try:
        try:
            wait_for_port(port, timeout)
        except TimeoutError:
            if server.poll() is not None:
                stderr_output = stderr_path.read_text(errors="ignore")
                stdout_output = stdout_path.read_text(errors="ignore")
                details.append("server exited before opening port")
                if stderr_output.strip():
                    details.append(f"stderr: {stderr_output.strip()}")
                if stdout_output.strip():
                    details.append(f"stdout: {stdout_output.strip()}")
                return False, details
            raise
        tests = [
            ("overlong_terminal_id", test_overlong_terminal_id),
            ("overlong_name", test_overlong_name),
            ("overlong_password", test_overlong_password),
            ("short_iac_sequence", test_short_iac_sequence),
            ("menu_garbage", test_menu_garbage),
            ("menu_negative", test_menu_negative),
            ("menu_zero", test_menu_zero),
            ("menu_float", test_menu_float),
            ("menu_whitespace", test_menu_whitespace),
            ("menu_escape_sequence", test_menu_escape_sequence),
            ("menu_large_number", test_menu_large_number),
            ("menu_long_line", test_menu_long_line),
            ("name_with_null", test_name_with_null),
            ("name_with_control_bytes", test_name_with_control_bytes),
            ("name_whitespace_only", test_name_whitespace_only),
            ("name_with_backspaces", test_name_with_backspaces),
            ("name_with_tabs", test_name_with_tabs),
            ("name_with_crlf", test_name_with_crlf),
            ("name_utf8", test_name_utf8),
            ("name_with_del", test_name_with_del),
            ("name_with_slash", test_name_with_slash),
            ("name_with_quotes", test_name_with_quotes),
            ("name_with_tilde", test_name_with_tilde),
            ("name_with_percent", test_name_with_percent),
            ("name_with_pipe", test_name_with_pipe),
            ("name_with_brackets", test_name_with_brackets),
            ("name_with_braces", test_name_with_braces),
            ("name_with_emoji", test_name_with_emoji),
            ("name_leading_spaces", test_name_leading_spaces),
            ("name_trailing_spaces", test_name_trailing_spaces),
            ("name_partial_line", test_name_partial_line),
            ("iac_double", test_iac_double),
            ("iac_subnegotiation", test_iac_subnegotiation),
            ("password_control_bytes", test_password_control_bytes),
            ("password_overlong_4096", test_password_overlong_4096),
            ("password_with_null", test_password_with_null),
            ("password_with_crlf", test_password_with_crlf),
            ("password_utf8", test_password_utf8),
            ("password_empty", test_password_empty),
            ("password_partial_then_newline", test_password_partial_then_newline),
            ("multiple_newlines", test_multiple_newlines),
            ("rapid_fire_inputs", test_rapid_fire_inputs),
            ("telnet_wont_echo", test_telnet_wont_echo),
            ("telnet_do_terminal", test_telnet_do_terminal),
            ("telnet_ga", test_telnet_ga),
            ("telnet_eor", test_telnet_eor),
            ("high_ascii_name", test_high_ascii_name),
            ("non_ascii_login", test_non_ascii_login),
            ("empty_name", test_empty_name),
            ("reconnect_flow", test_reconnect_flow),
        ]
        for name, test in tests:
            if server.poll() is not None:
                ok = False
                details.append(f"{name}: server exited unexpectedly")
                break
            passed, note = test(port)
            if server.poll() is not None:
                ok = False
                details.append(f"{name}: server exited unexpectedly after input")
                break
            if passed:
                details.append(f"{name}: ok ({note})")
            else:
                ok = False
                details.append(f"{name}: failed ({note})")
    finally:
        try:
            server.terminate()
            server.wait(timeout=5)
        except Exception:
            server.kill()
        stdout_file.close()
        stderr_file.close()
        shutil.rmtree(data_dir, ignore_errors=True)
    return ok, details


def ensure_clean_repo() -> None:
    status = subprocess.check_output(["git", "status", "--porcelain"], cwd=REPO_ROOT).decode()
    if status.strip():
        raise RuntimeError("Working tree is dirty; commit or stash changes before using --baseline.")


def git_checkout(revision: str) -> None:
    subprocess.check_call(["git", "checkout", revision], cwd=REPO_ROOT)


def current_revision() -> str:
    branch = subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], cwd=REPO_ROOT).decode().strip()
    if branch == "HEAD":
        return subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO_ROOT).decode().strip()
    return branch


def main() -> int:
    parser = argparse.ArgumentParser(description="Run nanny login regression harness.")
    parser.add_argument("--bin", dest="bin_path", help="Path to dms binary")
    parser.add_argument("--baseline", help="Git revision to test before current")
    parser.add_argument("--timeout", type=float, default=10.0, help="Seconds to wait for server start")
    parser.add_argument("--mini", action="store_true", help="Enable mini mode")
    parser.add_argument("--no-mini", action="store_true", help="Disable mini mode")
    parser.add_argument("--fuzz-iterations", type=int, default=3, help="Fuzz iterations per test")
    parser.add_argument("--fuzz-seed", type=int, default=1337, help="Seed for fuzzing")
    args = parser.parse_args()

    if args.mini and args.no_mini:
        raise SystemExit("Choose only one of --mini or --no-mini.")
    mini_mode = args.mini
    set_fuzzing(args.fuzz_iterations, args.fuzz_seed)

    if args.baseline:
        ensure_clean_repo()
        original = current_revision()
        print(f"Running baseline {args.baseline}...")
        try:
            git_checkout(args.baseline)
            binary = find_binary(args.bin_path)
            ok, details = run_suite(binary, mini_mode, args.timeout)
            for line in details:
                print(f"baseline: {line}")
            if not ok:
                print("Baseline run failed.")
        finally:
            git_checkout(original)
        print("Running current revision...")

    binary = find_binary(args.bin_path)
    ok, details = run_suite(binary, mini_mode, args.timeout)
    for line in details:
        print(f"current: {line}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
