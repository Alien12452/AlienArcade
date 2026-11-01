"""Symbolized 16-bit disassembly of NE code, shared by nedump and necmp.

Relocated fields are zeroed before disassembly and their targets are printed
as names instead, so two builds that differ only in link layout produce the
same text.
"""
from __future__ import annotations

import bisect
import re
from dataclasses import dataclass
from typing import Callable

from ne import (NEFile, SRC_FARPTR, SRC_SEGMENT, TGT_INTERNAL, MOVABLE_SEG,
                capstone16, masked_code, near_target)


@dataclass
class Line:
    offset: int
    raw: bytes
    text: str
    symbolic: bool = False  # raw bytes depend on link layout; compare the text only
    # The text with immediates left as numbers, set when an immediate was named.
    plain_imm: str | None = None
    # The text with memory displacements left as numbers, set when one was named.
    plain_mem: str | None = None
    # The text with displacements named from the next global (`G_X-0x2`), set
    # when that differs from the normal naming.
    before: str | None = None


class Symbolizer:
    """Turns addresses and relocations in one NE file into printable names.

    ``symbols`` maps ``(seg, off)`` to names from annotations; ``data_sizes``
    (DGROUP offset → size) lets memory operands name globals by range.
    """

    def __init__(self, ne: NEFile, symbols: dict[tuple[int, int], str] | None = None,
                 seg_alias: Callable[[int], str | None] | None = None, prefix: str = "",
                 data_sizes: dict[int, int] | None = None):
        self.ne = ne
        self.symbols = dict(symbols or {})
        self.data = {off: name for (seg, off), name in self.symbols.items()
                     if seg == ne.auto_data_seg}
        self.data_sizes = data_sizes or {}
        for e in ne.entries.values():
            name = ne.entry_names.get(e.ordinal)
            if name:
                self.symbols.setdefault((e.seg, e.offset), name)
        self.seg_alias = seg_alias or (lambda s: None)
        self.prefix = prefix
        self.dgroup = bytes(ne.segments[ne.auto_data_seg].data)

    def string_at(self, off: int) -> str | None:
        """The NUL-terminated string at a DGROUP offset, quoted, or None."""
        end = self.dgroup.find(b"\0", off)
        if off >= len(self.dgroup) or end < 0 or end - off > 60:
            return None
        s = self.dgroup[off:end]
        # A Pascal literal ("\x06" "000000") starts with its length byte.
        text = s[1:] if s and s[0] == len(s) - 1 else s
        if any(b < 0x20 and b not in (9, 10, 13) or b > 0x7E for b in text):
            return None
        return '"' + s.decode("latin-1").encode("unicode_escape").decode().replace('"', '\\"') + '"'

    def segment(self, seg: int) -> str:
        if seg == self.ne.auto_data_seg:
            return "DGROUP"
        return self.seg_alias(seg) or f"{self.prefix}seg{seg}"

    def address(self, seg: int, off: int) -> str:
        name = self.symbols.get((seg, off))
        if name:
            return name
        if seg == self.ne.auto_data_seg:
            return f"DGROUP:{off:04X}"
        return f"{self.prefix}{seg:04X}:{off:04X}"

    NEAR_TAIL = 0x20

    def code_address(self, seg: int, off: int) -> str:
        """A near jump target outside the current function. Runtime library
        entry points jump a few bytes into the next one (LUDIV@ into the
        body shared after LUMOD@); name those name+offset. Anything further
        from a symbol is unrelated code and keeps its address."""
        if (seg, off) in self.symbols:
            return self.symbols[(seg, off)]
        for d in range(1, self.NEAR_TAIL):
            name = self.symbols.get((seg, off - d))
            if name:
                return f"{name}+0x{d:x}"
        return self.address(seg, off)

    def reloc(self, r) -> str:
        if r.kind != TGT_INTERNAL:
            return self.ne.describe_target(r)
        t = self.ne.resolve_internal(r)
        if t is None:
            return self.ne.describe_target(r)
        if r.src_type == SRC_SEGMENT and r.a != MOVABLE_SEG:
            return self.segment(t[0])
        return self.address(*t)


def disassemble(ne: NEFile, segno: int, start: int, length: int,
                sym: Symbolizer) -> list[Line]:
    """Disassemble ``length`` bytes at ``segno:start`` with named targets.

    Relocated fields are zeroed before Capstone runs; branch targets and data
    operands are rewritten through ``sym`` so two link layouts compare equal.
    """
    code = masked_code(ne, segno)
    rmap = ne.reloc_map(segno)
    end = min(start + length, len(code))
    lines: list[Line] = []
    after_ds = False
    for insn in capstone16().disasm(code[start:end], start):
        relocs = []
        for k in range(insn.size):
            hit = rmap.get(insn.address + k)
            if hit and hit not in relocs:
                relocs.append(hit)
        text = f"{insn.mnemonic} {insn.op_str}".strip()
        plain_imm = plain_mem = before = None
        if insn.mnemonic != ".byte":
            named = _name_data_operands(insn, text, sym, after_ds)
            plain_imm = _name_data_operands(insn, text, sym, after_ds, immediates=False)
            plain_mem = _name_data_operands(insn, text, sym, after_ds, memory="plain")
            before = _name_data_operands(insn, text, sym, after_ds, memory="before")
            text = named
            plain_imm = None if plain_imm == text else plain_imm
            plain_mem = None if plain_mem == text else plain_mem
            before = None if before == text else before
        after_ds = (insn.mnemonic in ("push", "mov") and insn.op_str.split(", ")[-1] == "ds")
        if insn.mnemonic == ".byte":
            text = "db " + ", ".join(f"{b:02X}h" for b in insn.bytes)
        symbolic = bool(relocs)
        tgt = near_target(insn)
        if tgt is not None:
            if start <= tgt < end:
                label = f"L{tgt - start:04X}"
            else:
                label = sym.code_address(segno, tgt)
                symbolic = True
            text = f"{insn.mnemonic} {label}"
        if relocs:
            names = [sym.reloc(r) for r, _ in relocs]
            if insn.bytes[0] in (0x9A, 0xEA) and relocs[0][0].src_type == SRC_FARPTR:
                text = f"{insn.mnemonic} far {names[0]}"
            else:
                note = "  ; " + ", ".join(names)
                text += note
                # The relocation is the immediate (a function pointer stored
                # through BX). Naming [bx+disp] after a DGROUP global makes
                # the two layouts disagree, so keep the numeric displacement
                # and carry the target note with it.
                if plain_mem is not None:
                    plain_mem += note
                if before is not None:
                    before += note
        raw = bytes(ne.segments[segno].data[insn.address:insn.address + insn.size])
        if tgt is not None:
            plain_imm = plain_mem = before = None
        elif relocs:
            plain_imm = None
        lines.append(Line(insn.address, raw, text, symbolic, plain_imm, plain_mem, before))
    return lines


def _data_name(data: dict[int, str], sizes: dict[int, int], addr: int) -> str | None:
    """Name of the global at addr, or name+offset inside a global of known size."""
    if addr in data:
        return data[addr]
    keys = sorted(data)
    i = bisect.bisect_right(keys, addr) - 1
    if i >= 0 and addr - keys[i] < sizes.get(keys[i], 0):
        return f"{data[keys[i]]}+0x{addr - keys[i]:x}"
    return None


def _data_name_before(data: dict[int, str], addr: int, limit: int = 0x10) -> str | None:
    """`name-0xN` for an address just below a global: `g_a[i - 1]` indexes from
    `&g_a[-1]`, which lands in whatever global precedes g_a in that layout."""
    keys = sorted(data)
    i = bisect.bisect_left(keys, addr)
    if i < len(keys) and keys[i] != addr and keys[i] - addr <= limit:
        return f"{data[keys[i]]}-0x{keys[i] - addr:x}"
    return None


IMMEDIATE_MNEMONICS = {"push", "mov", "add"}


def data_operands(insn) -> list[tuple[str, int]]:
    """Possible DGROUP offsets used by an instruction: ("mem", disp) for
    DS-relative memory operands, ("imm", value) for push/mov immediates and
    `add reg, imm` (indexing a global array: `shl ax, 8; add ax, offset`)."""
    import capstone.x86 as x86
    out = []
    for op in insn.operands:
        if op.type == x86.X86_OP_IMM and insn.mnemonic in IMMEDIATE_MNEMONICS:
            out.append(("imm", op.imm & 0xFFFF))
        elif (op.type == x86.X86_OP_MEM and op.mem.base not in (
                x86.X86_REG_BP, x86.X86_REG_SP, x86.X86_REG_SI, x86.X86_REG_DI)
              and op.mem.segment in (0, x86.X86_REG_DS)):
            # SI/DI bases are struct fields (`[si+0x70]`). Naming those by a
            # DGROUP global they numerically collide with makes the two builds
            # disagree once the data segment shifts. BX is an index into a
            # global and keeps the name.
            out.append(("mem", op.mem.disp & 0xFFFF))
    return out


def _name_data_operands(insn, text: str, sym: Symbolizer, after_ds: bool,
                        immediates: bool = True, memory: str = "name") -> str:
    """Replace DS-relative displacements and `push`/`mov`/`add` immediates
    that fall inside a known global with its name, plus an offset when the
    address is not the global's base (`&g_img[n]` is `offset G_IMG+0xN`).
    An immediate right after `push ds` / `mov x, ds` is the offset half of a
    far pointer into DGROUP; a string literal there keeps its text, since the
    previous global's size is only the gap until the next symbol.
    Data offsets carry no relocation in NE files, so this is the only way to
    compare code before the data segment layout matches. `memory` names
    displacements normally ("name"), leaves them as numbers ("plain"), or
    names them from the next global when just below one ("before")."""
    data, sizes = sym.data, sym.data_sizes
    for kind, value in data_operands(insn):
        if kind == "imm":
            if not immediates:
                continue
            # An exact global wins. A far-pointer offset that is a string
            # literal wins over a neighbouring global whose size is only the
            # gap to the next symbol. Anything else inside a global is
            # `name+0xN`, which is how `&array[i]` shows up.
            name = data.get(value)
            if not name and after_ds:
                name = sym.string_at(value)
            if not name:
                named = _data_name(data, sizes, value)
                if named and "+" in named:
                    name = named
            if name:
                text = re.sub(rf"\b0x{value:x}$", lambda _: f"offset {name}", text)
        elif memory != "plain":
            name = _data_name(data, sizes, value)
            if memory == "before" and value not in data:
                name = _data_name_before(data, value) or name
            if name:
                text = text.replace(f"0x{value:x}]", f"{name}]", 1)
    return text


def format_lines(lines: list[Line], base: int | None = None) -> list[str]:
    """Pretty-print disassembly lines as ``OFF [Lrel] BYTES  text``.

    When ``base`` is set, each line also shows a label relative to that offset
    (``L0000``, ``L0003``, …), which is what ``nedump.py disasm`` prints.
    """
    out = []
    for ln in lines:
        rel = f"L{ln.offset - base:04X}" if base is not None else ""
        out.append(f"{ln.offset:04X} {rel:6} {ln.raw.hex(' '):<24} {ln.text}")
    return out
