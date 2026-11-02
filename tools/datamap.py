"""Layout of the rebuilt data segment (DGROUP), item by item.

Used by necmp.py for data-match coverage. TLINK's detailed segment map
(``/s``) gives each module's contribution to DGROUP. For compiled sources,
the assembly listings build.py writes with ``bcc -S`` (build/LST/*.ASM) split
a contribution into its labels with exact sizes; ``s@`` is the module's
string-literal pool. Assembly sources and library modules have no listing, so
their contribution is split at map publics.

Library callers use ``layout(map_path, listings_dir, data_seg, publics)``.
"""
from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path, PureWindowsPath

DETAIL_RE = re.compile(r"^\s*([0-9A-F]{4}):([0-9A-F]{4}) ([0-9A-F]{4}) C=(\S+)\s+S=(\S+)\s+G=\S*\s+M=(.+?)\s+ACBP=")
SEGMENT_RE = re.compile(r"^(\w+)\s+(segment|ends)\b")
LABEL_RE = re.compile(r"^(\S+)\s+label\s+\w+")
DIRECTIVE_RE = re.compile(r"^\s+(db|dw|dd)\s+(.*)$")
DUP_RE = re.compile(r"^(\d+)\s+dup\s*\(")
# Compiler markers at the start of a module's data and BSS.
MARKERS = {"d@", "d@w", "b@", "b@w"}
LITERALS = "s@"


@dataclass
class Contribution:
    seg: int
    off: int
    length: int
    cls: str
    segname: str
    module: str  # as the map shows it: a path for objects, a name for library modules

    @property
    def stem(self) -> str:
        return PureWindowsPath(self.module).stem.upper()


@dataclass
class Item:
    off: int
    size: int
    name: str | None  # None: bytes no label or public accounts for
    module: str
    sized: bool  # True when the size comes from a listing


def parse_detailed_map(path: Path) -> list[Contribution]:
    """Parse TLINK ``/s`` detailed-segment lines into per-module contributions."""
    out = []
    for line in path.read_text(encoding="latin-1").splitlines():
        m = DETAIL_RE.match(line)
        if m:
            seg, off, length, cls, segname, module = m.groups()
            out.append(Contribution(int(seg, 16), int(off, 16), int(length, 16), cls, segname, module))
    return out


def _directive_size(kind: str, operand: str) -> int:
    width = {"db": 1, "dw": 2, "dd": 4}[kind]
    m = DUP_RE.match(operand)
    if m:
        return width * int(m.group(1))
    if kind == "db" and operand.startswith("'"):
        return len(operand.strip()) - 2
    return width


def parse_listing(path: Path) -> dict[str, list[tuple[str, int]]]:
    """Labels and their sizes in each data segment of a `bcc -S` listing, in
    emission order. Consecutive labels on the same bytes get size 0."""
    items: dict[str, list[tuple[str, int]]] = {}
    current: str | None = None
    for line in path.read_text(encoding="latin-1").splitlines():
        m = SEGMENT_RE.match(line)
        if m:
            current = m.group(1) if m.group(2) == "segment" and m.group(1) in ("_DATA", "_BSS") else None
            continue
        if current is None:
            continue
        seq = items.setdefault(current, [])
        m = LABEL_RE.match(line)
        if m:
            seq.append((m.group(1), 0))
            continue
        m = DIRECTIVE_RE.match(line)
        if m:
            size = _directive_size(m.group(1), m.group(2))
            if seq:
                seq[-1] = (seq[-1][0], seq[-1][1] + size)
            else:
                seq.append(("", size))
    return items


def layout(map_path: Path, listings: Path, data_seg: int,
           publics: dict[str, tuple[int, int]]) -> tuple[list[Item], list[str]]:
    """Every byte of the data segment's contributions as items, plus warnings
    for listings that disagree with the map."""
    by_off: dict[int, list[str]] = {}
    for name, (seg, off) in publics.items():
        if seg == data_seg:
            by_off.setdefault(off, []).append(name)
    items: list[Item] = []
    warnings: list[str] = []
    for c in parse_detailed_map(map_path):
        if c.seg != data_seg or not c.length:
            continue
        listing = listings / f"{c.stem}.ASM"
        seq = parse_listing(listing).get(c.segname) if listing.exists() else None
        if seq is not None:
            total = sum(size for _, size in seq)
            if total != c.length:
                warnings.append(f"{listing.name} {c.segname}: listing has {total} bytes, map {c.length}")
            off = c.off
            for name, size in seq:
                if size:
                    items.append(Item(off, size, None if name in MARKERS else name or None, c.stem, True))
                off += size
            continue
        starts = sorted(o for o in by_off if c.off <= o < c.off + c.length)
        if not starts or starts[0] != c.off:
            items.append(Item(c.off, (starts[0] if starts else c.off + c.length) - c.off, None, c.stem, False))
        for o, nxt in zip(starts, starts[1:] + [c.off + c.length]):
            items.append(Item(o, nxt - o, sorted(by_off[o])[0], c.stem, False))
    # Word-alignment padding TLINK leaves between contributions: no module
    # claims those bytes. Attribute them to a synthetic module so coverage
    # can match them when the original and rebuilt bytes are identical.
    items.sort(key=lambda it: it.off)
    filled: list[Item] = []
    pos = 0
    for it in items:
        if it.off > pos:
            filled.append(Item(pos, it.off - pos, None, "(padding)", False))
        filled.append(it)
        pos = max(pos, it.off + it.size)
    return filled, warnings
