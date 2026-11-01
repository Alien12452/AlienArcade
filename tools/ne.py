"""Parser and disassembly helpers for 16-bit Windows NE (New Executable) files.

Reference: Microsoft "Executable-File Header Format" (NE), as shipped with the
Windows 3.1 SDK. Segment numbers are 1-based throughout, as in the NE format.
"""
from __future__ import annotations

import re
import struct
from dataclasses import dataclass, field
from pathlib import Path

SEG_DATA = 0x0001
SEG_MOVABLE = 0x0010
SEG_RELOCINFO = 0x0100

SRC_LOBYTE = 0
SRC_SEGMENT = 2
SRC_FARPTR = 3
SRC_OFFSET = 5
SRC_SIZES = {0: 1, 2: 2, 3: 4, 5: 2, 11: 6, 13: 4}
SRC_NAMES = {0: "lobyte", 2: "seg", 3: "far", 5: "off", 11: "far48", 13: "off32"}

TGT_INTERNAL = 0
TGT_IMPORTORD = 1
TGT_IMPORTNAME = 2
TGT_OSFIXUP = 3
RELOC_ADDITIVE = 0x04

MOVABLE_SEG = 0xFF

# Ordinal tables from Wine's 16-bit spec files.
SPEC_DIR = Path(__file__).resolve().parent / "winapi16"
SPEC_FILES = {"KERNEL": "krnl386.exe16.spec", "USER": "user.exe16.spec", "GDI": "gdi.exe16.spec"}
_SPEC_RE = re.compile(r"^\s*(\d+)\s+\w+\s+(?:-\S+\s+)*([A-Za-z_]\w*)")
_api_names: dict[str, dict[int, str]] = {}


def api_name(module: str, ordinal: int) -> str | None:
    """Resolve a Win16 import ordinal via Wine's ``tools/winapi16/*.spec`` tables."""
    module = module.upper()
    if module not in _api_names:
        table: dict[int, str] = {}
        spec = SPEC_DIR / SPEC_FILES.get(module, module.lower() + ".spec")
        if spec.exists():
            for line in spec.read_text().splitlines():
                m = _SPEC_RE.match(line)
                if m:
                    table[int(m.group(1))] = m.group(2)
        _api_names[module] = table
    return _api_names[module].get(ordinal)


@dataclass
class Reloc:
    src_type: int
    flags: int
    offset: int
    a: int  # segment / module index / OS fixup type
    b: int  # offset / ordinal / name offset
    sites: list[int] = field(default_factory=list)

    @property
    def kind(self) -> int:
        return self.flags & 3

    @property
    def additive(self) -> bool:
        return bool(self.flags & RELOC_ADDITIVE)

    @property
    def size(self) -> int:
        return SRC_SIZES.get(self.src_type, 2)


@dataclass
class Segment:
    index: int
    file_offset: int
    length: int
    flags: int
    min_alloc: int
    data: bytes
    relocs: list[Reloc] = field(default_factory=list)

    @property
    def is_code(self) -> bool:
        return not self.flags & SEG_DATA

    @property
    def movable(self) -> bool:
        return bool(self.flags & SEG_MOVABLE)


@dataclass
class Entry:
    ordinal: int
    seg: int
    offset: int
    flags: int
    movable: bool


class NEFile:
    """Parsed 16-bit Windows NE executable or DLL.

    Exposes the DOS stub / NE header fields, segment table (1-based indices),
    import module names, entry table, resident/non-resident names, and helpers
    to resolve relocations. Used by nedump, necmp, verify, and extract_res.
    """

    def __init__(self, path: str):
        self.path = path
        with open(path, "rb") as f:
            self.raw = f.read()
        if self.raw[:2] != b"MZ":
            raise ValueError(f"{path}: no MZ header")
        self.ne_off = struct.unpack_from("<I", self.raw, 0x3C)[0]
        if self.raw[self.ne_off:self.ne_off + 2] != b"NE":
            raise ValueError(f"{path}: not an NE executable")
        self._parse_header()
        self._parse_segments()
        self._parse_modules()
        self._parse_entries()
        self._parse_names()

    def _w(self, rel: int) -> int:
        return struct.unpack_from("<H", self.raw, self.ne_off + rel)[0]

    def _parse_header(self) -> None:
        h = self.ne_off
        self.linker_version = (self.raw[h + 2], self.raw[h + 3])
        self.flags = self._w(0x0C)
        self.auto_data_seg = self._w(0x0E)
        self.heap_size = self._w(0x10)
        self.stack_size = self._w(0x12)
        self.entry_ip, self.entry_cs = self._w(0x14), self._w(0x16)
        self.stack_sp, self.stack_ss = self._w(0x18), self._w(0x1A)
        self.seg_count = self._w(0x1C)
        self.mod_count = self._w(0x1E)
        self.nonres_size = self._w(0x20)
        self.seg_table = self._w(0x22)
        self.res_table = self._w(0x24)
        self.resname_table = self._w(0x26)
        self.modref_table = self._w(0x28)
        self.impname_table = self._w(0x2A)
        self.nonres_offset = struct.unpack_from("<I", self.raw, h + 0x2C)[0]
        self.align_shift = self._w(0x32) or 9
        self.target_os = self.raw[h + 0x36]
        self.expected_win = (self.raw[h + 0x3F], self.raw[h + 0x3E])
        self.entry_table = self._w(0x04)
        self.entry_table_len = self._w(0x06)

    def _parse_segments(self) -> None:
        self.segments: dict[int, Segment] = {}
        for i in range(self.seg_count):
            off, length, flags, minalloc = struct.unpack_from(
                "<HHHH", self.raw, self.ne_off + self.seg_table + 8 * i)
            length = length or (0x10000 if off else 0)
            minalloc = minalloc or 0x10000
            file_offset = off << self.align_shift
            data = self.raw[file_offset:file_offset + length] if off else b""
            seg = Segment(i + 1, file_offset, length, flags, minalloc, data)
            if flags & SEG_RELOCINFO and off:
                self._parse_relocs(seg)
            self.segments[seg.index] = seg

    def _parse_relocs(self, seg: Segment) -> None:
        p = seg.file_offset + seg.length
        count = struct.unpack_from("<H", self.raw, p)[0]
        p += 2
        for _ in range(count):
            src, flags, offset, a, b = struct.unpack_from("<BBHHH", self.raw, p)
            p += 8
            if flags & 3 == TGT_INTERNAL:
                a &= 0xFF
            r = Reloc(src, flags, offset, a, b)
            r.sites = self._chain(seg, r)
            seg.relocs.append(r)

    @staticmethod
    def _chain(seg: Segment, r: Reloc) -> list[int]:
        if r.additive or r.src_type == SRC_LOBYTE:
            return [r.offset]
        sites, seen, off = [], set(), r.offset
        while off < len(seg.data) - 1 and off not in seen:
            seen.add(off)
            sites.append(off)
            nxt = struct.unpack_from("<H", seg.data, off)[0]
            if nxt == 0xFFFF:
                break
            off = nxt
        return sites

    def _pstr(self, abs_off: int) -> str:
        n = self.raw[abs_off]
        return self.raw[abs_off + 1:abs_off + 1 + n].decode("latin-1")

    def _parse_modules(self) -> None:
        self.modules: list[str] = []
        for i in range(self.mod_count):
            o = self._w(self.modref_table + 2 * i)
            self.modules.append(self._pstr(self.ne_off + self.impname_table + o))

    def import_name(self, name_off: int) -> str:
        return self._pstr(self.ne_off + self.impname_table + name_off)

    def _parse_entries(self) -> None:
        self.entries: dict[int, Entry] = {}
        p = self.ne_off + self.entry_table
        end = p + self.entry_table_len
        ordinal = 1
        while p < end:
            count, ind = self.raw[p], self.raw[p + 1] if p + 1 < end else 0
            if count == 0:
                break
            p += 2
            if ind == 0:
                ordinal += count
                continue
            for _ in range(count):
                if ind == 0xFF:
                    flags, _int3f, seg, off = struct.unpack_from("<BHBH", self.raw, p)
                    p += 6
                    self.entries[ordinal] = Entry(ordinal, seg, off, flags, True)
                else:
                    flags, off = struct.unpack_from("<BH", self.raw, p)
                    p += 3
                    self.entries[ordinal] = Entry(ordinal, ind, off, flags, False)
                ordinal += 1

    def _parse_name_table(self, p: int, end: int | None) -> list[tuple[str, int]]:
        out = []
        while (end is None or p < end) and self.raw[p]:
            name = self._pstr(p)
            p += 1 + len(name)
            out.append((name, struct.unpack_from("<H", self.raw, p)[0]))
            p += 2
        return out

    def _parse_names(self) -> None:
        res = self._parse_name_table(self.ne_off + self.resname_table, None)
        nonres = []
        if self.nonres_offset:
            nonres = self._parse_name_table(self.nonres_offset,
                                            self.nonres_offset + self.nonres_size)
        self.module_name = res[0][0] if res else ""
        self.description = nonres[0][0] if nonres else ""
        self.entry_names = {o: n for n, o in res[1:] + nonres[1:]}

    def resolve_internal(self, r: Reloc) -> tuple[int, int] | None:
        """Target (segment, offset) of an internal reference."""
        if r.kind != TGT_INTERNAL:
            return None
        if r.a == MOVABLE_SEG:
            e = self.entries.get(r.b)
            return (e.seg, e.offset) if e else None
        return (r.a, r.b)

    def reloc_map(self, segno: int) -> dict[int, tuple[Reloc, int]]:
        """Byte offset -> (relocation, start of the patched field)."""
        out = {}
        for r in self.segments[segno].relocs:
            for s in r.sites:
                for k in range(r.size):
                    out[s + k] = (r, s)
        return out

    def describe_target(self, r: Reloc) -> str:
        kind = r.kind
        if kind == TGT_IMPORTORD:
            module = self.modules[r.a - 1]
            name = api_name(module, r.b)
            return f"{module}.{name}" if name else f"{module}.@{r.b}"
        if kind == TGT_IMPORTNAME:
            return f"{self.modules[r.a - 1]}.{self.import_name(r.b)}"
        if kind == TGT_OSFIXUP:
            return f"osfixup{r.a}"
        t = self.resolve_internal(r)
        if t is None:
            return f"badref({r.a:#x},{r.b:#x})"
        seg, off = t
        if r.src_type == SRC_SEGMENT and r.a != MOVABLE_SEG:
            return f"seg{seg}"
        return f"{seg:04X}:{off:04X}"


def masked_code(ne: NEFile, segno: int) -> bytes:
    """Segment bytes with every relocated field zeroed, so link-time chain
    values do not leak into disassembly or comparisons."""
    data = bytearray(ne.segments[segno].data)
    for off in ne.reloc_map(segno):
        if off < len(data):
            data[off] = 0
    return bytes(data)


_cs = None


def capstone16():
    """Shared Capstone handle for 16-bit x86 (lazy import)."""
    global _cs
    if _cs is None:
        import capstone
        _cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
        _cs.skipdata = True
        _cs.detail = True
    return _cs


_BRANCHES = {"call", "jmp", "loop", "loope", "loopne", "jcxz"}
_STOPS = {"ret", "retf", "iret", "hlt"}


def is_branch(mnemonic: str) -> bool:
    return mnemonic in _BRANCHES or (mnemonic.startswith("j") and mnemonic != "jmp")


def near_target(insn) -> int | None:
    """Absolute target of a relative near/short branch, else None."""
    if not is_branch(insn.mnemonic) or insn.bytes[0] in (0x9A, 0xEA, 0xFF):
        return None
    try:
        return int(insn.op_str, 16)
    except ValueError:
        return None


PROLOGUES = [
    bytes.fromhex("8cd8904555 8bec1e8ed8"),  # mov ax,ds; nop; inc bp; push bp; mov bp,sp; push ds; mov ds,ax
    bytes.fromhex("1e58904555 8bec1e8ed8"),  # push ds; pop ax; nop; ...
    bytes.fromhex("45558bec"),  # inc bp; push bp; mov bp,sp (far frame)
    bytes.fromhex("558bec"),  # push bp; mov bp,sp
]


def find_functions(ne: NEFile, segno: int) -> dict[int, str]:
    """Function start offsets in a code segment, mapped to how each was found.

    Seeds come from the entry table, the program entry point and far-call
    relocations; recursive descent then follows near calls. Gaps left over
    are scanned for Borland prologue patterns.
    """
    code = masked_code(ne, segno)
    found: dict[int, str] = {}

    def add(off: int, why: str) -> None:
        if 0 <= off < len(code) and off not in found:
            found[off] = why

    for e in ne.entries.values():
        if e.seg == segno:
            add(e.offset, "entry")
    if ne.entry_cs == segno:
        add(ne.entry_ip, "start")
    for other in ne.segments.values():
        for r in other.relocs:
            if r.src_type == SRC_FARPTR:
                t = ne.resolve_internal(r)
                if t and t[0] == segno:
                    add(t[1], "farcall")

    visited: set[int] = set()
    cs = capstone16()

    def walk(start: int) -> None:
        todo = [start]
        while todo:
            pc = todo.pop()
            while 0 <= pc < len(code) and pc not in visited:
                insns = list(cs.disasm(code[pc:pc + 16], pc, 1))
                if not insns or insns[0].mnemonic == ".byte":
                    break
                insn = insns[0]
                for k in range(insn.size):
                    visited.add(pc + k)
                m = insn.mnemonic
                tgt = near_target(insn)
                if m == "call":
                    if tgt is not None:
                        add(tgt, "call")
                elif tgt is not None:
                    todo.append(tgt)
                if m in _STOPS or m == "jmp":
                    break
                pc += insn.size

    done: set[int] = set()
    while True:
        pending = [o for o in found if o not in done]
        if not pending:
            gaps = [o for o in range(len(code)) if o not in visited]
            gapset = set(gaps)
            new = False
            for pat in PROLOGUES:
                start = 0
                while (i := code.find(pat, start)) >= 0:
                    start = i + 1
                    if i in gapset and i not in found:
                        found[i] = "prologue"
                        gapset.difference_update(range(i, i + len(pat)))
                        new = True
            if not new:
                break
            continue
        for o in pending:
            done.add(o)
            walk(o)
    return dict(sorted(found.items()))


def function_extents(ne: NEFile, segno: int) -> list[tuple[int, int, str]]:
    """``(start, length, discovery_reason)`` for each function in ``segno``."""
    funcs = find_functions(ne, segno)
    starts = list(funcs)
    seglen = ne.segments[segno].length
    return [(s, (starts[i + 1] if i + 1 < len(starts) else seglen) - s, funcs[s])
            for i, s in enumerate(starts)]
