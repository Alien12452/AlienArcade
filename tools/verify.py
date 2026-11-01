#!/usr/bin/env python3
"""Check that a clean build reproduces the original AARCADE.EXE byte for byte.

By default this runs ``build.py --clean`` first, so nothing left over from an
earlier build can make the result look better than it is. The rebuilt file is
then hashed against orig/SHA256SUMS. When it differs, the file is compared
structure by structure (DOS stub, NE header, tables, each segment's bytes and
relocations, each resource) to show where the difference is.

    .venv/bin/python tools/verify.py
    .venv/bin/python tools/verify.py --no-build
    .venv/bin/python tools/verify.py --diff '#5:#270'

Exit status is 0 only when AARCADE.EXE is identical. With ``--dlls``, exit
status is 0 only when AARCADE.EXE, AACOLOR.DLL, and AABW.DLL all match
``orig/SHA256SUMS``. Without ``--dlls`` the picture DLLs are still reported
but do not affect the result.
"""
from __future__ import annotations

import argparse
import hashlib
import struct
import subprocess
import sys
from pathlib import Path

from ne import NEFile

ROOT = Path(__file__).resolve().parent.parent
ORIG = ROOT / "orig"
BUILD = ROOT / "build"
TARGET = "AARCADE.EXE"
EXTRA = ("AACOLOR.DLL", "AABW.DLL")

NE_FIELDS = [
    (0x02, 2, "linker version"), (0x04, 2, "entry table offset"), (0x06, 2, "entry table length"),
    (0x08, 4, "file CRC"), (0x0C, 2, "flags"), (0x0E, 2, "auto data segment"),
    (0x10, 2, "heap size"), (0x12, 2, "stack size"), (0x14, 4, "CS:IP"), (0x18, 4, "SS:SP"),
    (0x1C, 2, "segment count"), (0x1E, 2, "module reference count"),
    (0x20, 2, "non-resident name table size"), (0x22, 2, "segment table offset"),
    (0x24, 2, "resource table offset"), (0x26, 2, "resident name table offset"),
    (0x28, 2, "module reference table offset"), (0x2A, 2, "imported name table offset"),
    (0x2C, 4, "non-resident name table file offset"), (0x30, 2, "movable entry count"),
    (0x32, 2, "alignment shift"), (0x34, 2, "resource segment count"), (0x36, 1, "target OS"),
    (0x37, 1, "other flags"), (0x38, 2, "gangload offset"), (0x3A, 2, "gangload length"),
    (0x3C, 2, "minimum code swap size"), (0x3E, 2, "expected Windows version"),
]


def expected_hashes() -> dict[str, str]:
    sums = {}
    for line in (ORIG / "SHA256SUMS").read_text().splitlines():
        digest, _, name = line.partition("  ")
        if name:
            sums[name.strip()] = digest
    return sums


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def resources(ne: NEFile) -> dict[tuple[str, str], bytes]:
    """Resource data keyed by (type, name). Integer IDs are shown as `#n`."""
    raw = ne.raw
    base = ne.ne_off + ne.res_table
    if ne.res_table == ne.resname_table:
        return {}
    shift = struct.unpack_from("<H", raw, base)[0]

    def ident(value: int) -> str:
        if value & 0x8000:
            return f"#{value & 0x7FFF}"
        p = base + value
        return raw[p + 1:p + 1 + raw[p]].decode("latin-1")

    out = {}
    p = base + 2
    while True:
        rtype = struct.unpack_from("<H", raw, p)[0]
        if rtype == 0:
            break
        count = struct.unpack_from("<H", raw, p + 2)[0]
        p += 8
        for _ in range(count):
            off, length, _flags, rid = struct.unpack_from("<HHHH", raw, p)
            p += 12
            start = off << shift
            out[(ident(rtype), ident(rid))] = raw[start:start + (length << shift)]
    return out


def reloc_bytes(ne: NEFile, segno: int) -> bytes:
    seg = ne.segments[segno]
    if not seg.relocs:
        return b""
    p = seg.file_offset + seg.length
    count = struct.unpack_from("<H", ne.raw, p)[0]
    return ne.raw[p:p + 2 + 8 * count]


def table(ne: NEFile, start: int, end: int) -> bytes:
    return ne.raw[ne.ne_off + start:ne.ne_off + end]


def explain_resources(ra: dict[tuple[str, str], bytes], rb: dict[tuple[str, str], bytes]) -> list[str]:
    """Resource differences, grouped by type. A type present on one side only
    whose resources reappear, with the same names and data, under another
    type on the other side is reported as a renamed type."""
    def by_type(res: dict[tuple[str, str], bytes], other: dict[tuple[str, str], bytes]) -> dict[str, dict[str, bytes]]:
        groups: dict[str, dict[str, bytes]] = {}
        for (rtype, name), data in res.items():
            if (rtype, name) not in other:
                groups.setdefault(rtype, {})[name] = data.rstrip(b"\0")
        return groups

    only_a, only_b = by_type(ra, rb), by_type(rb, ra)
    out = []
    for ta in list(only_a):
        tb = next((t for t, items in only_b.items() if items == only_a[ta]), None)
        if tb is not None:
            out.append(f"resource type {ta} in the original is {tb} rebuilt "
                       f"({len(only_a[ta])} resources, same data)")
            del only_a[ta], only_b[tb]
    for label, groups in (("only in the original", only_a), ("only in the rebuilt file", only_b)):
        for rtype, items in groups.items():
            out.append(f"resource type {rtype}: {', '.join(items)} {label}")
    for key in ra:
        if key in rb and ra[key].rstrip(b"\0") != rb[key].rstrip(b"\0"):
            out.append(f"resource {key[0]} {key[1]}: data differs")
    common = [k for k in ra if k in rb]
    if common != [k for k in rb if k in ra]:
        out.append("resource table: shared resources are in a different order")
    return out


def explain(orig: NEFile, new: NEFile) -> list[str]:
    """Where two NE files differ, one line per difference."""
    out = []
    if orig.raw[:0x40] != new.raw[:0x40] or orig.ne_off != new.ne_off:
        out.append(f"DOS header/stub: NE header at {orig.ne_off:#x} in the original, {new.ne_off:#x} rebuilt")
    elif orig.raw[:orig.ne_off] != new.raw[:new.ne_off]:
        out.append("DOS stub: same size, different bytes")

    for off, size, name in NE_FIELDS:
        a = orig.raw[orig.ne_off + off:orig.ne_off + off + size]
        b = new.raw[new.ne_off + off:new.ne_off + off + size]
        if a != b:
            out.append(f"NE header {name} (+{off:#04x}): {a.hex()} vs {b.hex()}")

    tables = [
        ("segment table (lengths, flags)", lambda f: b"".join(
            struct.pack("<HHH", s.length & 0xFFFF, s.flags, s.min_alloc & 0xFFFF)
            for s in f.segments.values())),
        ("resident name table", lambda f: table(f, f.resname_table, f.modref_table)),
        ("module reference table", lambda f: table(f, f.modref_table, f.impname_table)),
        ("imported name table", lambda f: table(f, f.impname_table, f.entry_table)),
        ("entry table", lambda f: table(f, f.entry_table, f.entry_table + f.entry_table_len)),
        ("non-resident name table", lambda f: f.raw[f.nonres_offset:f.nonres_offset + f.nonres_size]),
    ]
    for name, get in tables:
        if get(orig) != get(new):
            out.append(f"{name} differs")

    for segno in sorted(orig.segments):
        a, b = orig.segments[segno], new.segments.get(segno)
        if b is None:
            out.append(f"segment {segno}: missing")
            continue
        if bytes(a.data) != bytes(b.data):
            n = sum(x != y for x, y in zip(a.data, b.data)) + abs(len(a.data) - len(b.data))
            out.append(f"segment {segno}: {n} bytes differ")
        if reloc_bytes(orig, segno) != reloc_bytes(new, segno):
            out.append(f"segment {segno}: relocations differ")

    out.extend(explain_resources(resources(orig), resources(new)))
    if len(orig.raw) != len(new.raw):
        out.append(f"file size: {len(orig.raw)} vs {len(new.raw)}")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--no-build", action="store_true", help="compare the existing build without rebuilding")
    ap.add_argument("--borland", type=Path, help="pass through to build.py")
    ap.add_argument("--rc-tree", type=Path, help="pass through to build.py")
    ap.add_argument("--rc-compile", help="pass through to build.py")
    ap.add_argument("--rc-bind", help="pass through to build.py")
    ap.add_argument("--dlls", action="store_true",
                    help="require AACOLOR.DLL and AABW.DLL to match SHA256SUMS too")
    ap.add_argument("--diff", metavar="TYPE:ID",
                    help="hexdump the first differing bytes of one resource (e.g. #5:#270)")
    args = ap.parse_args()

    if not args.no_build:
        cmd = [sys.executable, str(ROOT / "tools" / "build.py"), "--clean"]
        if args.borland:
            cmd += ["--borland", str(args.borland)]
        if args.rc_tree:
            cmd += ["--rc-tree", str(args.rc_tree)]
        if args.rc_compile:
            cmd += ["--rc-compile", args.rc_compile]
        if args.rc_bind:
            cmd += ["--rc-bind", args.rc_bind]
        done = subprocess.run(cmd)
        if done.returncode:
            print("verify: build failed", file=sys.stderr)
            return done.returncode

    sums = expected_hashes()
    if sha256(ORIG / TARGET) != sums[TARGET]:
        print(f"verify: orig/{TARGET} does not match orig/SHA256SUMS; run setup_orig.py", file=sys.stderr)
        return 2
    rebuilt = BUILD / TARGET
    if not rebuilt.exists():
        print(f"verify: {rebuilt.relative_to(ROOT)} is missing; build first", file=sys.stderr)
        return 2

    identical = sha256(rebuilt) == sums[TARGET]
    print(f"{TARGET}: {'identical' if identical else 'differs'}")
    if not identical:
        for line in explain(NEFile(str(ORIG / TARGET)), NEFile(str(rebuilt))):
            print(f"  {line}")
    if args.diff:
        key = tuple(args.diff.split(":", 1))
        if len(key) != 2:
            print("verify: --diff needs TYPE:ID", file=sys.stderr)
            return 2
        ra, rb = resources(NEFile(str(ORIG / TARGET))), resources(NEFile(str(rebuilt)))
        if key not in ra and key not in rb:
            print(f"verify: resource {args.diff} not found on either side", file=sys.stderr)
            return 2
        a = ra.get(key, b"")
        b = rb.get(key, b"")
        n = max(len(a), len(b))
        print(f"--diff {args.diff}: orig {len(a)} bytes, rebuilt {len(b)} bytes")
        for i in range(0, n, 16):
            oa, ob = a[i:i + 16], b[i:i + 16]
            if oa == ob:
                continue
            print(f"  {i:04X}: {oa.hex(' '):<47} | {ob.hex(' '):<47}")
            if i > 128:
                print("  ...")
                break
    dlls_ok = True
    for name in EXTRA:
        path = BUILD / name
        state = "missing" if not path.exists() else ("identical" if sha256(path) == sums.get(name) else "differs")
        if state != "identical":
            dlls_ok = False
        print(f"{name}: {state}")
        if args.dlls and path.exists() and (ORIG / name).exists() and state != "identical":
            for line in explain(NEFile(str(ORIG / name)), NEFile(str(path))):
                print(f"  {line}")
    if args.dlls:
        return 0 if identical and dlls_ok else 1
    return 0 if identical else 1


if __name__ == "__main__":
    sys.exit(main())
