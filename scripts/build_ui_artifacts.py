#!/usr/bin/env python3
"""Parameterized cross-compiler for Sony ILCE-7RM3 (FW 3.10) Native UI artifacts.

Builds from clean repository sources using `zig cc`:
  - `scene-current.elf`:            Static ARMv7 read-only ViewManager / WidgetServer topology inspector
  - `liba7r_menu_product_v21.so`:   Native View 0xFFC 6-row Chinese settings UI & SettingNode router
  - `libproBC.so`:                  Read-only RTLD_NOLOAD menu query/diagnostic sidecar (commandBC10)

All dynamic libraries undergo post-link `DT_RELCOUNT = 0` normalization for `ld-2.11.2.so`
compatibility and strict `W^X` / symbol export auditing. Zero proprietary blobs are stored here.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT / "scripts") not in sys.path:
    sys.path.insert(0, str(REPO_ROOT / "scripts"))

from elf_tools import normalize_and_audit_dso  # noqa: E402


def resolve_fw_libs(fw_root: Path) -> Path:
    for candidate in [
        fw_root / "filesystems/nflasha7/export/lib",
        fw_root / "nflasha7/export/lib",
        fw_root / "lib",
        fw_root,
    ]:
        if (candidate / "libc-2.11.2.so").is_file() and (candidate / "libstdc++.so.6.0.14").is_file():
            return candidate
    raise FileNotFoundError(
        f"Could not find unpacked FW 3.10 rootfs libraries (libc-2.11.2.so, libstdc++.so.6.0.14) under {fw_root}"
    )


def run_cmd(cmd: list[str], target_name: str) -> None:
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"Failed building {target_name}:\n{proc.stderr}")


def run_zig_dso(cmd: list[str], out_path: Path, exports: set[str]) -> dict[str, object]:
    run_cmd(cmd, out_path.name)
    audit = normalize_and_audit_dso(out_path, expected_exports=exports)
    return {
        "file": out_path.name,
        "bytes": audit.bytes_len,
        "sha256": audit.sha256,
        "soname": audit.soname,
        "needed": audit.needed,
        "exports": {k: {"va": v[0], "size": v[1]} for k, v in sorted(audit.exports.items())},
        "imports": audit.imports,
        "dt_relcount_patched": audit.relcount_patched,
    }


def build_scene_inspector(zig_bin: str, out_dir: Path) -> dict[str, object]:
    sc_dir = REPO_ROOT / "scene-inspector"
    ui_dir = REPO_ROOT / "menu-ui"
    sc_out = out_dir / "scene-current.elf"
    cmd = [
        zig_bin,
        "cc",
        "-target",
        "arm-linux-musleabi",
        "-mcpu=cortex_a9",
        "-marm",
        "-mfloat-abi=soft",
        "-Os",
        "-static",
        f"-I{sc_dir}",
        f"-I{ui_dir}",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wno-unused-function",
        str(sc_dir / "scene-current.c"),
        "-o",
        str(sc_out),
    ]
    run_cmd(cmd, sc_out.name)
    raw = sc_out.read_bytes()
    return {
        "file": sc_out.name,
        "bytes": len(raw),
        "sha256": hashlib.sha256(raw).hexdigest(),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fw-root",
        type=Path,
        default=Path(os.environ["A7R3_FW_ROOT"]) if os.environ.get("A7R3_FW_ROOT") else None,
        help="Path to unpacked A7R3 FW 3.10 root directory (or set $A7R3_FW_ROOT)",
    )
    parser.add_argument(
        "--scene-only",
        action="store_true",
        help="Build only the static musl scene-current.elf inspector (does not require --fw-root)",
    )
    parser.add_argument(
        "--zig",
        default=os.environ.get("ZIG", shutil.which("zig") or "zig"),
        help="Path to zig binary (default: $ZIG or zig on PATH)",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=REPO_ROOT / "build/arm-ui",
        help="Output directory for built UI artifacts (default: build/arm-ui)",
    )
    args = parser.parse_args(argv)

    out_dir = args.out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    built: list[dict[str, object]] = [build_scene_inspector(args.zig, out_dir)]

    if not args.scene_only:
        if args.fw_root is None:
            raise SystemExit(
                "Error: --fw-root or $A7R3_FW_ROOT is required to link UI DSOs (or pass --scene-only)."
            )
        lib_dir = resolve_fw_libs(args.fw_root.resolve())
        zig_arm = [
            args.zig,
            "cc",
            "-target",
            "arm-linux-gnueabi",
            "-mcpu=cortex_a9",
            "-marm",
            "-mfloat-abi=soft",
        ]
        common_link = [
            "-nostdlib",
            "-shared",
            "-fPIC",
            "-fvisibility=hidden",
            "-fno-builtin",
            "-fno-stack-protector",
            "-Os",
            "-Wl,--hash-style=sysv",
            "-Wl,-z,now",
            "-Wl,-z,noexecstack",
            "-Wl,-z,defs",
        ]

        libc = str(lib_dir / "libc-2.11.2.so")
        libdl = str(lib_dir / "libdl-2.11.2.so")
        libpthread = str(lib_dir / "libpthread-2.11.2.so")
        libstdcxx = str(lib_dir / "libstdc++.so.6.0.14")
        libgcc = str(lib_dir / "libgcc_s.so.1")

        ui_dir = REPO_ROOT / "menu-ui"
        mp_sources = [
            "power_lifecycle.c",
            "admission.c",
            "product.c",
            "native_port.c",
            "map_reader.c",
            "menu_maps.c",
            "menu_snapshot.c",
            "resident_route.c",
            "registry_catch.cpp",
            "lean_view.c",
            "graphics.c",
            "graphics_registry.c",
            "graphics_catch.cpp",
            "navigation.c",
            "navigation_catch.cpp",
            "stock_open.cpp",
            "audio_native.c",
            "audio_transaction.c",
            "options.c",
            "page_control.c",
            "diagnostic.c",
            "diagnostic_inject.cpp",
            "entry_route.c",
            "entry_route_catch.cpp",
        ]
        mp_exports = {
            "a7r_menu_product_v21",
            "a7r_menu_bootstrap_v21",
            "a7r_menu_power_v21",
            "A7ProductViewFactory21",
        }
        built.append(
            run_zig_dso(
                [
                    *zig_arm,
                    *common_link,
                    "-fomit-frame-pointer",
                    f"-I{ui_dir}",
                    f"-Wl,--version-script,{ui_dir / 'liba7r_menu_product_v21.so.map'}",
                    "-Wl,-soname,liba7r_menu_product_v21.so",
                    *[str(ui_dir / s) for s in mp_sources],
                    libdl,
                    libpthread,
                    libc,
                    libstdcxx,
                    libgcc,
                    "-o",
                    str(out_dir / "liba7r_menu_product_v21.so"),
                ],
                out_dir / "liba7r_menu_product_v21.so",
                mp_exports,
            )
        )
        built.append(
            run_zig_dso(
                [
                    *zig_arm,
                    *common_link,
                    "-Wl,-soname,libproBC.so",
                    f"-Wl,--version-script,{ui_dir / 'libproBC.so.map'}",
                    str(ui_dir / "control.c"),
                    libdl,
                    libc,
                    "-o",
                    str(out_dir / "libproBC.so"),
                ],
                out_dir / "libproBC.so",
                {"commandBC10"},
            )
        )

    manifest_path = out_dir / "build-manifest.json"
    manifest_path.write_text(json.dumps({"artifacts": built}, indent=2) + "\n", encoding="utf-8")
    print(f"Built {len(built)} ARMv7 UI artifacts into {out_dir}")
    for item in built:
        print(f"  - {item['file']}: {item['bytes']} B (sha256={item['sha256'][:16]}...)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
