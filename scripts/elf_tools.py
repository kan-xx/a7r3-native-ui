"""Pure-stdlib ELF32 ARM EABI5 inspector and Sony `ld-2.11.2.so` `DT_RELCOUNT=0` patcher.

Eliminates external `pyelftools` build-time requirements while enforcing every safety invariant:
- Patches `DT_RELCOUNT (0x6ffffffa) -> 0` in `.dynamic` to prevent Sony's `ld-2.11.2.so`
  non-contiguous `DT_REL`/`DT_JMPREL` relocation corruption bug under `DT_BIND_NOW`.
- Audits absence of `DT_INIT`, `DT_INIT_ARRAY`, `DT_PREINIT_ARRAY`, `DT_TEXTREL`, RWX `PT_LOAD`
  segments, and executable `PT_GNU_STACK`.
- Resolves `.dynsym` exports/imports and virtual-address slices across `PT_LOAD` segments.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
from pathlib import Path
import struct

PT_LOAD = 1
PT_GNU_STACK = 0x6474E551

PF_X = 0x1
PF_W = 0x2

SHT_DYNSYM = 11
SHT_DYNAMIC = 6
SHN_UNDEF = 0
STB_GLOBAL = 1
STB_WEAK = 2

DT_NULL = 0
DT_NEEDED = 1
DT_INIT = 12
DT_FINI = 13
DT_SONAME = 14
DT_TEXTREL = 22
DT_INIT_ARRAY = 25
DT_FINI_ARRAY = 26
DT_PREINIT_ARRAY = 32
DT_FLAGS_1 = 0x6FFFFFFB
DT_RELCOUNT = 0x6FFFFFFA


@dataclass(frozen=True)
class Elf32Audit:
    bytes_len: int
    sha256: str
    flags: int
    soname: list[str]
    needed: list[str]
    exports: dict[str, tuple[int, int]]
    imports: list[str]
    lifecycle_tags: list[str]
    relcount_patched: list[tuple[int, int]]


def _read_cstr(blob: bytes | bytearray, offset: int) -> str:
    if offset < 0 or offset >= len(blob):
        return ""
    end = blob.find(b"\x00", offset)
    if end < 0:
        end = len(blob)
    return bytes(blob[offset:end]).decode("latin-1")


def _parse_headers(raw: bytes | bytearray) -> tuple[int, list[tuple[int, int, int, int, int, int]], dict[str, tuple[int, int, int, int, int]]]:
    if len(raw) < 52 or raw[:4] != b"\x7fELF":
        raise ValueError("Not an ELF binary")
    ei_class, ei_data = raw[4], raw[5]
    if ei_class != 1 or ei_data != 1:
        raise ValueError(f"Expected 32-bit little-endian ELF, got class={ei_class} data={ei_data}")
    (
        e_type,
        e_machine,
        _e_version,
        _e_entry,
        e_phoff,
        e_shoff,
        e_flags,
        _e_ehsize,
        e_phentsize,
        e_phnum,
        e_shentsize,
        e_shnum,
        e_shstrndx,
    ) = struct.unpack_from("<HHIIIIIHHHHHH", raw, 16)
    if e_machine != 40 or e_type != 3:
        raise ValueError(f"Expected ARM ET_DYN shared object, got e_machine={e_machine} e_type={e_type}")

    phdrs: list[tuple[int, int, int, int, int, int]] = []
    for i in range(e_phnum):
        p_type, p_offset, p_vaddr, _p_paddr, p_filesz, p_memsz, p_flags, _p_align = struct.unpack_from(
            "<8I", raw, e_phoff + i * e_phentsize
        )
        phdrs.append((p_type, p_offset, p_vaddr, p_filesz, p_memsz, p_flags))

    sh_raw: list[tuple[int, int, int, int, int, int]] = []
    for i in range(e_shnum):
        sh_name, sh_type, _sh_flags, sh_addr, sh_offset, sh_size, sh_link, _sh_info, _sh_align, sh_entsize = (
            struct.unpack_from("<10I", raw, e_shoff + i * e_shentsize)
        )
        sh_raw.append((sh_name, sh_type, sh_offset, sh_size, sh_link, sh_entsize))

    shstr_off, shstr_size = sh_raw[e_shstrndx][2], sh_raw[e_shstrndx][3]
    shstrtab = bytes(raw[shstr_off : shstr_off + shstr_size])

    sections: dict[str, tuple[int, int, int, int, int]] = {}
    for idx, (sh_name, sh_type, sh_offset, sh_size, sh_link, sh_entsize) in enumerate(sh_raw):
        name = _read_cstr(shstrtab, sh_name)
        sections[name] = (sh_type, sh_offset, sh_size, sh_link, sh_entsize)
        sections[f"__idx_{idx}"] = (sh_type, sh_offset, sh_size, sh_link, sh_entsize)

    return e_flags, phdrs, sections


def normalize_and_audit_dso(
    path: Path,
    *,
    expected_exports: set[str] | None = None,
    allow_fini: bool = False,
) -> Elf32Audit:
    """Zero `DT_RELCOUNT` in-place on `path`, enforce loader safety invariants, and return metadata."""
    raw = bytearray(path.read_bytes())
    e_flags, phdrs, sections = _parse_headers(raw)

    if ".dynamic" not in sections or ".dynsym" not in sections:
        raise ValueError(f"{path.name}: missing .dynamic or .dynsym section")

    _dyn_type, dyn_off, dyn_size, dyn_link, _dyn_ent = sections[".dynamic"]
    dynstr_info = sections[f"__idx_{dyn_link}"]
    dynstr = bytes(raw[dynstr_info[1] : dynstr_info[1] + dynstr_info[2]])

    patched: list[tuple[int, int]] = []
    for pos in range(0, dyn_size, 8):
        d_tag, d_val = struct.unpack_from("<II", raw, dyn_off + pos)
        if d_tag == DT_NULL:
            break
        if d_tag == DT_RELCOUNT and d_val != 0:
            struct.pack_into("<I", raw, dyn_off + pos + 4, 0)
            patched.append((d_val, 0))

    path.write_bytes(raw)

    # Re-audit post-normalization bytes
    needed: list[str] = []
    soname: list[str] = []
    lifecycle_tags: list[str] = []
    tag_map = {
        DT_INIT: "DT_INIT",
        DT_FINI: "DT_FINI",
        DT_INIT_ARRAY: "DT_INIT_ARRAY",
        DT_FINI_ARRAY: "DT_FINI_ARRAY",
        DT_PREINIT_ARRAY: "DT_PREINIT_ARRAY",
    }
    for pos in range(0, dyn_size, 8):
        d_tag, d_val = struct.unpack_from("<II", raw, dyn_off + pos)
        if d_tag == DT_NULL:
            break
        if d_tag == DT_NEEDED:
            needed.append(_read_cstr(dynstr, d_val))
        elif d_tag == DT_SONAME:
            soname.append(_read_cstr(dynstr, d_val))
        elif d_tag == DT_TEXTREL:
            raise ValueError(f"{path.name}: forbidden DT_TEXTREL present")
        elif d_tag == DT_RELCOUNT and d_val != 0:
            raise ValueError(f"{path.name}: DT_RELCOUNT was not cleared")
        elif d_tag == DT_FLAGS_1 and (d_val & 0x8):
            raise ValueError(f"{path.name}: forbidden DF_1_NODELETE set")
        elif d_tag in tag_map:
            lifecycle_tags.append(tag_map[d_tag])

    forbidden_lifecycle = {"DT_INIT", "DT_INIT_ARRAY", "DT_PREINIT_ARRAY"}
    if not allow_fini:
        forbidden_lifecycle |= {"DT_FINI", "DT_FINI_ARRAY"}
    bad_lc = forbidden_lifecycle.intersection(lifecycle_tags)
    if bad_lc:
        raise ValueError(f"{path.name}: forbidden lifecycle tags {sorted(bad_lc)}")

    for p_type, _p_off, _p_va, _p_fsz, _p_msz, p_flags in phdrs:
        if p_type == PT_LOAD and (p_flags & (PF_W | PF_X)) == (PF_W | PF_X):
            raise ValueError(f"{path.name}: forbidden RWX PT_LOAD segment")
        if p_type == PT_GNU_STACK and (p_flags & PF_X):
            raise ValueError(f"{path.name}: forbidden executable PT_GNU_STACK")

    _sym_type, sym_off, sym_size, sym_link, _sym_ent = sections[".dynsym"]
    symstr_info = sections[f"__idx_{sym_link}"]
    symstr = bytes(raw[symstr_info[1] : symstr_info[1] + symstr_info[2]])

    exports: dict[str, tuple[int, int]] = {}
    imports: list[str] = []
    for pos in range(0, sym_size, 16):
        st_name, st_value, st_size, st_info, _st_other, st_shndx = struct.unpack_from(
            "<IIIBBH", raw, sym_off + pos
        )
        name = _read_cstr(symstr, st_name)
        if not name:
            continue
        bind = st_info >> 4
        if st_shndx == SHN_UNDEF:
            imports.append(name)
        elif bind in (STB_GLOBAL, STB_WEAK):
            exports[name] = (st_value, st_size)

    if expected_exports is not None and set(exports) != expected_exports:
        raise ValueError(
            f"{path.name}: export mismatch; got {sorted(exports)}, expected {sorted(expected_exports)}"
        )

    return Elf32Audit(
        bytes_len=len(raw),
        sha256=hashlib.sha256(raw).hexdigest(),
        flags=e_flags,
        soname=soname,
        needed=needed,
        exports=exports,
        imports=sorted(imports),
        lifecycle_tags=lifecycle_tags,
        relcount_patched=patched,
    )


def read_va_bytes(path: Path, va: int, length: int) -> bytes:
    """Read `length` bytes at virtual address `va` from an ELF32 image's `PT_LOAD` segments."""
    raw = path.read_bytes()
    _flags, phdrs, _sections = _parse_headers(raw)
    for p_type, p_offset, p_vaddr, p_filesz, _p_memsz, _p_flags in phdrs:
        if p_type == PT_LOAD and p_vaddr <= va and va + length <= p_vaddr + p_filesz:
            rel = va - p_vaddr
            return raw[p_offset + rel : p_offset + rel + length]
    raise ValueError(f"{path.name}: virtual address {va:#x} (+{length}) not in any PT_LOAD file slice")
