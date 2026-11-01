#!/usr/bin/env python3
"""Expand the installer packs in bin/ into orig/ and check their SHA-256 hashes.

Place your own copy of the game files in bin/ (not committed):

    aarcade.ex_   ->  AARCADE.EXE
    aabw.dl_      ->  AABW.DLL
    aacolor.dl_   ->  AACOLOR.DLL
    meter.dll     ->  METER.DLL

SZDD-compressed packs (Microsoft COMPRESS.EXE, magic ``SZDD``) are expanded
via szdd.py; plain files are copied as-is. Digests are checked against
orig/SHA256SUMS when that file exists (it is the only orig/ file committed).

    .venv/bin/python tools/setup_orig.py

Exit status is 0 when every file expands and matches the expected hash (or
when SHA256SUMS is missing and hashes are only printed).
"""
from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

from szdd import expand

ROOT = Path(__file__).resolve().parent.parent
FILES = {
    "aarcade.ex_": "AARCADE.EXE",
    "aabw.dl_": "AABW.DLL",
    "aacolor.dl_": "AACOLOR.DLL",
    "meter.dll": "METER.DLL",
}


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.parse_args()
    src, dst = ROOT / "bin", ROOT / "orig"
    if not src.is_dir():
        print(f"missing {src}: place the installer packs there first", file=sys.stderr)
        return 1
    dst.mkdir(exist_ok=True)
    expected = {}
    sums = dst / "SHA256SUMS"
    if sums.exists():
        for line in sums.read_text().splitlines():
            digest, name = line.split()
            expected[name] = digest
    ok = True
    for packed, name in FILES.items():
        path = src / packed
        if not path.is_file():
            print(f"{name:12} missing {path}", file=sys.stderr)
            ok = False
            continue
        data = path.read_bytes()
        if data[:4] == b"SZDD":
            data = expand(data)
        (dst / name).unlink(missing_ok=True)
        (dst / name).write_bytes(data)
        digest = hashlib.sha256(data).hexdigest()
        status = "ok" if expected.get(name, digest) == digest else "MISMATCH"
        ok &= status == "ok"
        print(f"{name:12} {digest}  {status}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
