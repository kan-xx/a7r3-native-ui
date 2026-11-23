#!/usr/bin/env python3
"""Offline unit tests for BC10 / D010 UI wire codecs and ELF DT_RELCOUNT=0 auditor."""
from __future__ import annotations

import struct
import sys
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT / "menu-ui"))
sys.path.insert(0, str(REPO_ROOT / "scripts"))

import codec as bc10_codec  # noqa: E402
import d010_codec  # noqa: E402


def _make_synthetic_reply(magic: bytes, op: int, nonce: bytes, *, chunk_bytes: int = 0, snap_idx: int = 0xFFFFFFFF, total: int = 0) -> bytes:
    header = (
        magic
        + struct.pack(
            "<10I",
            1,                      # version
            op,                     # op
            0,                      # status
            bc10_codec.REPORT,      # report_bytes = 1148
            snap_idx,               # snapshot_index
            0,                      # offset
            chunk_bytes,            # chunk_bytes
            total,                  # total
            0,                      # flags
            bc10_codec.BUILD,       # build = 0x3110d015
        )
        + nonce
    )
    report = bytearray(bc10_codec.REPORT)
    struct.pack_into("<3I", report, 0, 1, bc10_codec.REPORT, 2)  # version=1, bytes=1148, phase=resident(2)
    report[1132:1148] = nonce
    pad = bytes(bc10_codec.HEADER - 64 - bc10_codec.REPORT)
    return header + bytes(report) + pad + bytes(chunk_bytes)


class TestUiWireCodecs(unittest.TestCase):
    def test_bc10_noload_forbids_mutating_ops(self) -> None:
        nonce = bytes(range(1, 17))
        # BC10 only permits read-only/diagnostic ops {2, 5, 6, 7}; install(1)/remove(3)/drain(4) must fail
        for forbidden_op in (1, 3, 4):
            with self.assertRaises(ValueError):
                bc10_codec.request(forbidden_op, nonce)

        req_q = bc10_codec.request(2, nonce)
        self.assertEqual(req_q[:4], struct.pack("<HH", 0, 0xBC10))
        self.assertEqual(req_q[4:12], b"A7M16S01")

        raw_reply = _make_synthetic_reply(b"A7M16R01", 2, nonce)
        parsed = bc10_codec.reply(raw_reply, 2, nonce)
        self.assertEqual(parsed["report"]["phase_name"], "resident")
        self.assertEqual(parsed["report"]["nonce"], nonce.hex())

    def test_bc10_snapshot_bounds(self) -> None:
        nonce = b"\xaa" * 16
        with self.assertRaises(ValueError):
            bc10_codec.request(7, nonce, offset=bc10_codec.SNAPSHOT, index=0)
        with self.assertRaises(ValueError):
            bc10_codec.request(7, nonce, offset=0, index=4)

        req_snap = bc10_codec.request(7, nonce, offset=0, index=1)
        self.assertEqual(len(req_snap), 40)

        raw_snap = _make_synthetic_reply(
            b"A7M16R01", 7, nonce, chunk_bytes=4096, snap_idx=1, total=bc10_codec.SNAPSHOT
        )
        parsed = bc10_codec.reply(raw_snap, 7, nonce, offset=0, index=1)
        self.assertEqual(parsed["chunk_bytes"], 4096)

    def test_d010_bind_and_install_request(self) -> None:
        instance = bytes(range(16, 32))
        nonce = bytes(range(32, 48))
        b48 = d010_codec.bind(392, instance, nonce, backend_kind=2)
        self.assertEqual(len(b48), 48)

        req = d010_codec.request(1, nonce, binding=b48)
        self.assertEqual(req[:4], struct.pack("<HH", 0, 0xD010))
        self.assertEqual(req[4:12], b"A7PRD7S1")

        # Zero nonce must always be rejected
        with self.assertRaises(ValueError):
            d010_codec.request(2, bytes(16))


if __name__ == "__main__":
    unittest.main()
