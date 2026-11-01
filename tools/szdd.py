#!/usr/bin/env python3
"""Expand Microsoft COMPRESS.EXE (SZDD) archives such as AARCADE.EX_.

Windows install media often ships ``*.ex_`` / ``*.dl_`` files compressed with
COMPRESS.EXE. The 8-byte magic is ``SZDD\\x88\\xf0\\x27\\x33``, followed by a
flag byte, the original filename's last character, and a little-endian
uncompressed size. The payload is an LZSS bitstream into a 4 KiB window.

As a library, ``setup_orig.py`` calls ``expand(data)``. As a CLI:

    .venv/bin/python tools/szdd.py bin/aarcade.ex_ /tmp/AARCADE.EXE
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

SZDD_MAGIC = b"SZDD\x88\xf0\x27\x33"


def expand(data: bytes) -> bytes:
    """Return the uncompressed bytes of an SZDD archive.

    Raises ValueError if the magic is wrong or the expanded length does not
    match the size stored in the header.
    """
    if data[:8] != SZDD_MAGIC:
        raise ValueError("not an SZDD archive")
    size = struct.unpack_from("<I", data, 10)[0]
    window = bytearray(b" " * 4096)
    pos = 4096 - 16
    out = bytearray()
    i = 14
    while i < len(data) and len(out) < size:
        flags = data[i]
        i += 1
        for bit in range(8):
            if i >= len(data) or len(out) >= size:
                break
            if flags & (1 << bit):
                c = data[i]
                i += 1
                out.append(c)
                window[pos] = c
                pos = (pos + 1) & 0xFFF
            else:
                src = data[i] | ((data[i + 1] & 0xF0) << 4)
                length = (data[i + 1] & 0x0F) + 3
                i += 2
                for k in range(length):
                    c = window[(src + k) & 0xFFF]
                    out.append(c)
                    window[pos] = c
                    pos = (pos + 1) & 0xFFF
    if len(out) != size:
        raise ValueError(f"expanded {len(out)} bytes, header says {size}")
    return bytes(out)


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("src", type=Path, help="SZDD-compressed input file")
    ap.add_argument("dst", type=Path, help="path to write the expanded bytes")
    args = ap.parse_args()
    data = expand(args.src.read_bytes())
    args.dst.write_bytes(data)
    print(f"wrote {len(data)} bytes to {args.dst}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
