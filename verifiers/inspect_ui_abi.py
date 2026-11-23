#!/usr/bin/env python3
"""Read-only ELF symbol & Thumb instruction inspector for Sony UI menu tables.

Inspects `CmnViewMenuData` / `SettingNode` / `View` dynamic symbols inside user-supplied
unpacked ILCE-7RM3 FW 3.10 libraries (`libObj.so`, `viewUnified2.so`, `CautionConfig.so`)
without executing or modifying any target binary.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path


def resolve_usr_lib(fw_root: Path) -> Path:
    for candidate in [
        fw_root / "filesystems/nflasha15/export/lib",
        fw_root / "nflasha15/export/lib",
        fw_root / "usr/lib",
        fw_root,
    ]:
        if (candidate / "libObj.so").is_file():
            return candidate
    raise FileNotFoundError(f"Could not locate libObj.so under {fw_root}")


def inspect_ui_symbols(lib_path: Path, pattern: str, max_size: int = 128) -> dict[str, object]:
    from capstone import CS_ARCH_ARM, CS_MODE_THUMB, Cs
    from elftools.elf.elffile import ELFFile

    raw = lib_path.read_bytes()
    file_sha256 = hashlib.sha256(raw).hexdigest()
    rows: list[dict[str, object]] = []
    cs = Cs(CS_ARCH_ARM, CS_MODE_THUMB)

    with lib_path.open("rb") as fh:
        elf = ELFFile(fh)
        dynsym = elf.get_section_by_name(".dynsym")
        if dynsym is None:
            return {"library": lib_path.name, "sha256": file_sha256, "functions": []}
        segments = [seg for seg in elf.iter_segments() if seg["p_type"] == "PT_LOAD"]
        for sym in dynsym.iter_symbols():
            size = int(sym["st_size"])
            if pattern not in sym.name or size <= 0 or size > max_size:
                continue
            va = int(sym["st_value"]) & ~1
            for seg in segments:
                vaddr = int(seg["p_vaddr"])
                filesz = int(seg["p_filesz"])
                if vaddr <= va < va + size <= vaddr + filesz:
                    offset = int(seg["p_offset"]) + (va - vaddr)
                    code = raw[offset : offset + size]
                    break
            else:
                continue
            insns = [
                f"{insn.address:08x} {insn.bytes.hex()} {insn.mnemonic} {insn.op_str}".rstrip()
                for insn in cs.disasm(code, va)
            ]
            rows.append(
                {
                    "symbol": sym.name,
                    "va": hex(va),
                    "thumb": bool(int(sym["st_value"]) & 1),
                    "bytes": len(code),
                    "sha256": hashlib.sha256(code).hexdigest(),
                    "instructions": insns,
                }
            )

    return {
        "library": lib_path.name,
        "sha256": file_sha256,
        "method": "read-only ELF dynsym + Capstone Thumb disassembly",
        "pattern": pattern,
        "count": len(rows),
        "functions": rows,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fw-root",
        type=Path,
        default=Path(os.environ["A7R3_FW_ROOT"]) if os.environ.get("A7R3_FW_ROOT") else None,
        help="Path to unpacked A7R3 FW 3.10 root directory (or set $A7R3_FW_ROOT)",
    )
    parser.add_argument("--library", default="libObj.so", help="Target library name (default: libObj.so)")
    parser.add_argument("--pattern", default="CmnViewMenuData", help="Symbol substring filter")
    parser.add_argument("--max-size", type=int, default=128, help="Max symbol byte length to disassemble")
    parser.add_argument("--output", type=Path, help="Optional JSON output file path")
    args = parser.parse_args(argv)

    if args.fw_root is None:
        raise SystemExit("Error: --fw-root or $A7R3_FW_ROOT is required.")

    usr_lib = resolve_usr_lib(args.fw_root.resolve())
    report = inspect_ui_symbols(usr_lib / args.library, args.pattern, max_size=args.max_size)
    rendered = json.dumps(report, indent=2)
    if args.output:
        args.output.write_text(rendered + "\n", encoding="utf-8")
    else:
        print(rendered)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
