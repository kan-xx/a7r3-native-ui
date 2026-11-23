#!/usr/bin/env python3
"""Offline host test runner for `a7r3-native-ui` (invoked by `make test`).

Runs 100% offline with zero proprietary Sony firmware binaries:
1. Python `unittest` suites (`test_ui_codecs.py` for BC10/D010 wire protocols)
2. Host `clang` C11 suites (with ASan + UBSan):
   - `test_page`: View 0xFFC 6-row navigation, pending cursor movement, slot-3 silence, INACT/ACT power gate
   - `test_audio_transaction`: UI-to-backend REVOKE -> DRAIN -> REBIND transaction state machine
   - `test_map_reader`: Bounded /proc/self/maps parser & segment reader
   - `test_control`: libproBC.so (commandBC10) RTLD_NOLOAD read-only enforcement & nonce binding
   - `test_menu_snapshot`: 39-slot ViewManager & PVM doubly-linked registry snapshotter
"""
from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

REPO_ROOT = Path(__file__).resolve().parents[1]
TESTS_DIR = REPO_ROOT / "tests"
UI_DIR = REPO_ROOT / "menu-ui"


def run_cmd(cmd: list[str], *, env: dict[str, str] | None = None) -> str:
    proc = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if proc.returncode != 0:
        sys.stderr.write(f"Command failed: {' '.join(cmd)}\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}\n")
        raise SystemExit(proc.returncode)
    return proc.stdout.strip()


def main() -> int:
    cc = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not cc:
        raise SystemExit("No host C compiler (clang/cc) found.")

    print("=== [1/2] Running Python Unit Tests (BC10 / D010 UI Wire Codecs) ===")
    subprocess.run(
        [sys.executable, "-m", "unittest", "discover", "-s", str(TESTS_DIR), "-p", "test_*.py", "-v"],
        check=True,
    )

    print("\n=== [2/2] Compiling & Running Host C11 UI Verification Suites ===")
    cflags = [cc, "-std=c11", "-O1", "-g", "-pthread"]
    env = os.environ.copy()

    with tempfile.TemporaryDirectory(prefix="a7r3-ui-tests-") as tmp:
        tmp_dir = Path(tmp)
        probe_c = tmp_dir / "san_probe.c"
        probe_exe = tmp_dir / "san_probe"
        probe_c.write_text("int main(void){return 0;}\n", encoding="utf-8")
        san_ok = (
            subprocess.run(
                [*cflags, "-fsanitize=address,undefined", str(probe_c), "-o", str(probe_exe)],
                capture_output=True,
            ).returncode
            == 0
        )
        if san_ok:
            cflags.append("-fsanitize=address,undefined")
            env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
            env["UBSAN_OPTIONS"] = "halt_on_error=1"

        suites = [
            ("test_audio_transaction", [UI_DIR / "audio_transaction.c", TESTS_DIR / "test_audio_transaction.c"]),
            (
                "test_page",
                [
                    UI_DIR / "power_lifecycle.c",
                    UI_DIR / "admission.c",
                    UI_DIR / "page_control.c",
                    UI_DIR / "options.c",
                    UI_DIR / "audio_transaction.c",
                    TESTS_DIR / "test_page.c",
                ],
            ),
            ("test_map_reader", [UI_DIR / "map_reader.c", UI_DIR / "menu_maps.c", TESTS_DIR / "test_map_reader.c"]),
            ("test_control", [TESTS_DIR / "test_control.c"]),
            ("test_menu_snapshot", [UI_DIR / "menu_snapshot.c", TESTS_DIR / "test_menu_snapshot.c"]),
        ]

        for name, srcs in suites:
            exe = tmp_dir / name
            run_cmd([*cflags, f"-I{UI_DIR}", *[str(s) for s in srcs], "-o", str(exe)])
            print(f"  [PASS] {name}:", run_cmd([str(exe)], env=env))

    print("\nAll offline Python & C11 UI tests passed (zero firmware blobs required).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
