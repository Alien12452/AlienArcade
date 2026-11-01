#!/usr/bin/env python3
"""Explore an NE executable: header, segments, imports, relocations,
functions and symbolized disassembly.

Defaults to orig/AARCADE.EXE and names from annotations under src/. Segment
numbers are hex NE indices (1-based); addresses are ``SEG`` or ``SEG:OFF``.

    .venv/bin/python tools/nedump.py info
    .venv/bin/python tools/nedump.py funcs 5
    .venv/bin/python tools/nedump.py disasm 5:00AD          # one function
    .venv/bin/python tools/nedump.py disasm 5               # whole segment
    .venv/bin/python tools/nedump.py xrefs 5:00AD           # far refs to an address
    .venv/bin/python tools/nedump.py xrefs 24:38B8 --size 0x100   # DGROUP global
    .venv/bin/python tools/nedump.py def                    # linker .DEF sketch
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from annotations import parse_tree
from necmp import data_sizes, parse_map
from ne import (NEFile, SRC_NAMES, capstone16, function_extents, masked_code,
                TGT_IMPORTORD, TGT_IMPORTNAME)
from nedis import Symbolizer, data_operands, disassemble, format_lines

ROOT = Path(__file__).resolve().parent.parent


def parse_addr(text: str) -> tuple[int, int | None]:
    if ":" in text:
        s, o = text.split(":")
        return int(s, 16), int(o, 16)
    return int(text, 16), None


def symbolizer(ne: NEFile, module: str, src: Path) -> Symbolizer:
    """Names from the annotations; global sizes from the rebuilt map if there is one."""
    annotations = parse_tree(src, module)
    syms = {(a.seg, a.off): a.name for a in annotations}
    sizes = {}
    mapfile = ROOT / "build" / f"{module}.MAP"
    if mapfile.exists():
        rebuilt = NEFile(str(ROOT / "build" / f"{module}.EXE"))
        by_name = {n.lstrip("_").upper(): s
                   for n, s in data_sizes(parse_map(mapfile), rebuilt.auto_data_seg).items()}
        sizes = {a.off: by_name[a.name.upper()] for a in annotations
                 if a.kind == "GLOBAL" and a.name.upper() in by_name}
    return Symbolizer(ne, syms, data_sizes=sizes)


def cmd_info(ne: NEFile, args) -> None:
    code = sum(s.length for s in ne.segments.values() if s.is_code)
    print(f"file          {ne.path}")
    print(f"module        {ne.module_name}  ({ne.description})")
    print(f"linker        {ne.linker_version[0]}.{ne.linker_version[1]}")
    print(f"windows       {ne.expected_win[0]}.{ne.expected_win[1]}")
    print(f"flags         {ne.flags:#06x}")
    print(f"entry         {ne.entry_cs:04X}:{ne.entry_ip:04X}")
    print(f"auto data     segment {ne.auto_data_seg}, heap {ne.heap_size:#x}, stack {ne.stack_size:#x}")
    print(f"segments      {ne.seg_count}, code bytes {code}")
    print(f"imports       {', '.join(ne.modules)}")
    print("exports:")
    for o, e in sorted(ne.entries.items()):
        print(f"  @{o:<3} {e.seg:04X}:{e.offset:04X}  {ne.entry_names.get(o, '')}")


def cmd_segments(ne: NEFile, args) -> None:
    print("seg  file_off  length  minalloc  flags   kind  relocs")
    for s in ne.segments.values():
        kind = "CODE" if s.is_code else "DATA"
        print(f"{s.index:3}  {s.file_offset:8X}  {s.length:6X}  {s.min_alloc:8X}  "
              f"{s.flags:04X}  {kind}  {len(s.relocs):5}")


def cmd_imports(ne: NEFile, args) -> None:
    used: dict[str, set[int]] = {}
    for s in ne.segments.values():
        for r in s.relocs:
            if r.kind in (TGT_IMPORTORD, TGT_IMPORTNAME):
                desc = ne.describe_target(r)
                used.setdefault(desc, set()).add(s.index)
    for name in sorted(used):
        print(f"{name:20} used by segments {sorted(used[name])}")


def cmd_relocs(ne: NEFile, args) -> None:
    seg = ne.segments[int(args.seg, 16)]
    for r in seg.relocs:
        sites = ", ".join(f"{s:04X}" for s in r.sites)
        add = " additive" if r.additive else ""
        print(f"{SRC_NAMES.get(r.src_type, r.src_type):6} {ne.describe_target(r):20}{add}  at {sites}")


def cmd_funcs(ne: NEFile, args) -> None:
    sym = symbolizer(ne, args.module, args.src)
    segs = [int(args.seg, 16)] if args.seg else [s.index for s in ne.segments.values() if s.is_code]
    for segno in segs:
        for start, size, why in function_extents(ne, segno):
            name = sym.symbols.get((segno, start), "")
            print(f"{segno:04X}:{start:04X}  {size:5}  {why:8}  {name}")


def cmd_disasm(ne: NEFile, args) -> None:
    sym = symbolizer(ne, args.module, args.src)
    segno, off = parse_addr(args.addr)
    extents = function_extents(ne, segno)
    if off is not None:
        extents = [e for e in extents if e[0] == off] or [(off, args.length or 0x40, "manual")]
    for start, size, why in extents:
        size = args.length or size
        name = sym.symbols.get((segno, start), "")
        print(f"\n; ---- {segno:04X}:{start:04X} {name}  ({size} bytes, found by {why})")
        print("\n".join(format_lines(disassemble(ne, segno, start, size, sym), start)))


def _seg_attrs(flags: int, data: bool) -> str:
    attrs = ["PRELOAD" if flags & 0x0040 else "LOADONCALL",
             "MOVEABLE" if flags & 0x0010 else "FIXED"]
    if flags & 0x1000:
        attrs.append("DISCARDABLE")
    if data:
        attrs.append("MULTIPLE")
    return " ".join(attrs)


def cmd_def(ne: NEFile, args) -> None:
    """Module-definition (.DEF) file reproducing the original header."""
    code = [s for s in ne.segments.values() if s.is_code]
    data = ne.segments[ne.auto_data_seg]
    resident = set(ne._parse_name_table(ne.ne_off + ne.resname_table, None)[1:])
    print(f"NAME {ne.module_name}")
    print(f"DESCRIPTION '{ne.description}'")
    print("EXETYPE WINDOWS")
    if ne.flags & 0x0008:
        print("PROTMODE")
    print(f"CODE {_seg_attrs(code[0].flags, False)}")
    for s in code:
        if s.flags & ~0x0100 != code[0].flags & ~0x0100:
            print(f"; segment {s.index} differs: {_seg_attrs(s.flags, False)}")
    print(f"DATA {_seg_attrs(data.flags, True)}")
    print(f"HEAPSIZE {ne.heap_size}")
    print(f"STACKSIZE {ne.stack_size}")
    print("EXPORTS")
    for o in sorted(ne.entries):
        name = ne.entry_names.get(o, f"ORD{o}")
        res = " RESIDENTNAME" if (name, o) in resident else ""
        print(f"\t{name} @{o}{res}")


def cmd_xrefs(ne: NEFile, args) -> None:
    segno, off = parse_addr(args.addr)
    if segno == ne.auto_data_seg:
        _data_xrefs(ne, args, off)
        return
    for s in ne.segments.values():
        for r in s.relocs:
            if ne.resolve_internal(r) == (segno, off):
                for site in r.sites:
                    print(f"{s.index:04X}:{site:04X}  {SRC_NAMES.get(r.src_type)}")


def _data_xrefs(ne: NEFile, args, off: int) -> None:
    """DGROUP offsets have no relocations, so scan the code of every found
    function for memory operands and push/mov/add immediates in [off, off+size).
    Immediates can be plain numbers that happen to match."""
    sym = symbolizer(ne, args.module, args.src)
    for s in ne.segments.values():
        if not s.is_code:
            continue
        code = masked_code(ne, s.index)
        for start, size, _ in function_extents(ne, s.index):
            for insn in capstone16().disasm(code[start:start + size], start):
                if insn.mnemonic == ".byte":
                    continue
                hits = [k for k, v in data_operands(insn) if off <= v < off + args.size]
                if hits:
                    func = sym.symbols.get((s.index, start), f"{s.index:04X}:{start:04X}")
                    note = "  (immediate)" if "mem" not in hits else ""
                    print(f"{s.index:04X}:{insn.address:04X}  {func:24} "
                          f"{insn.mnemonic} {insn.op_str}{note}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=str(ROOT / "orig" / "AARCADE.EXE"),
                    help="NE file to inspect (default: orig/AARCADE.EXE)")
    ap.add_argument("--module", default="AARCADE", help="annotation module name")
    ap.add_argument("--src", type=Path, default=ROOT / "src",
                    help="annotated sources for symbol names (default: src/)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("info", help="header summary, entry point, segment count")
    sub.add_parser("segments", help="list NE segments and flags")
    sub.add_parser("imports", help="imported modules and ordinals")
    sub.add_parser("def", help="emit a module-definition sketch for the linker")
    p = sub.add_parser("relocs", help="relocations in one segment")
    p.add_argument("seg", help="segment number, hex")
    p = sub.add_parser("funcs", help="discovered functions in a segment (or all)")
    p.add_argument("seg", nargs="?", help="segment number, hex (omit for every code segment)")
    p = sub.add_parser("disasm", help="symbolized disassembly of a function or segment")
    p.add_argument("addr", help="SEG or SEG:OFF, hex")
    p.add_argument("--length", type=lambda x: int(x, 0),
                   help="byte length when addr is SEG:OFF (default: to next function)")
    p = sub.add_parser("xrefs", help="code that references an address")
    p.add_argument("addr", help="SEG:OFF, hex")
    p.add_argument("--size", type=lambda x: int(x, 0), default=1,
                   help="for DGROUP addresses: bytes covered by the global")
    args = ap.parse_args()
    ne = NEFile(args.exe)
    globals()["cmd_" + args.cmd](ne, args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
