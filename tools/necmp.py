#!/usr/bin/env python3
"""Compare annotated functions in a rebuilt NE executable against the original.

Needs orig/AARCADE.EXE, a rebuilt build/AARCADE.EXE + AARCADE.MAP from
build.py, and ``// FUNCTION:`` / ``// LIBRARY:`` annotations under src/. The
rebuilt side is located through the TLINK map (link with ``/m``). Each
annotated function is disassembled on both sides with relocations and
out-of-function branch targets replaced by names, so code that differs only
in link layout still compares equal.

A placeholder named ``FUN_SSSS_OOOO`` (any case, optional leading underscore)
still stands for the original at ``SSSS:OOOO`` if matching mid-function.

A full run also refreshes docs/progress.json for ByteMatch / report.py.

    .venv/bin/python tools/necmp.py
    .venv/bin/python tools/necmp.py --diff WinMain
    .venv/bin/python tools/necmp.py --data
    .venv/bin/python tools/necmp.py --selftest
"""
from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from dataclasses import dataclass, asdict
from pathlib import Path

from annotations import Annotation, linker_candidates, parse_tree
from datamap import LITERALS, Item, layout
from ne import NEFile, function_extents
from nedis import Line, Symbolizer, disassemble

ROOT = Path(__file__).resolve().parent.parent
PUBLIC_RE = re.compile(r"^\s*([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})\s+(?:\w+\s+)?(\S+)\s*$")
PLACEHOLDER_RE = re.compile(r"^_?(?:FUN|SUB)_([0-9A-F]{4})_([0-9A-F]{4})$", re.I)


def parse_map(path: Path) -> dict[str, tuple[int, int]]:
    publics: dict[str, tuple[int, int]] = {}
    in_publics = False
    for line in path.read_text(encoding="latin-1").splitlines():
        if "Publics by" in line:
            in_publics = True
            continue
        if not in_publics:
            continue
        m = PUBLIC_RE.match(line)
        if m and int(m.group(1), 16):
            publics.setdefault(m.group(3), (int(m.group(1), 16), int(m.group(2), 16)))
    return publics


def normalize(name: str) -> str:
    m = PLACEHOLDER_RE.match(name)
    return f"{m.group(1).upper()}:{m.group(2).upper()}" if m else name


def resolve_symbol(a: Annotation, publics: dict[str, tuple[int, int]]) -> str | None:
    if a.symbol:
        return a.symbol if a.symbol in publics else None
    upper = {k.upper(): k for k in publics}
    for cand in linker_candidates(a.name):
        if cand in publics:
            return cand
        if cand.endswith("$"):
            hit = next((k for k in publics if k.startswith(cand)), None)
            if hit:
                return hit
    return upper.get(a.name.upper())


def place_library_statics(annotations: list[Annotation], publics: dict[str, tuple[int, int]]) -> None:
    """Add LIBRARY annotations whose symbol is not public (static helpers in
    the startup code and the runtime library) to `publics`. Library code is
    linked unchanged, so such a helper lies at the same distance from the
    nearest public library annotation as in the original."""
    lib = sorted((a for a in annotations if a.kind == "LIBRARY"), key=lambda a: (a.seg, a.off))
    anchors = [a for a in lib if a.symbol in publics]
    for a in lib:
        if a.symbol in publics:
            continue
        same = [b for b in anchors if b.seg == a.seg]
        before = [b for b in same if b.off < a.off]
        ref = before[-1] if before else next((b for b in same if b.off > a.off), None)
        if ref:
            seg, off = publics[ref.symbol]
            publics[a.symbol] = (seg, off + a.off - ref.off)


def data_sizes(publics: dict[str, tuple[int, int]], data_seg: int,
               end: int | None = None) -> dict[str, int]:
    """Approximate size of each data symbol: the distance to the next one, or
    to `end` (the segment's size) for the last."""
    items = sorted((off, name) for name, (seg, off) in publics.items() if seg == data_seg)
    sizes = {name: nxt - off for (off, name), (nxt, _) in zip(items, items[1:])}
    if items and end is not None and end > items[-1][0]:
        sizes[items[-1][1]] = end - items[-1][0]
    return sizes


def extent(starts: list[int], seglen: int, off: int) -> int:
    nxt = [s for s in starts if s > off]
    return (nxt[0] if nxt else seglen) - off


def _trim_padding(lines: list[Line]) -> list[Line]:
    """Drop zero bytes the linker put between modules for word alignment."""
    while lines and not any(lines[-1].raw):
        lines = lines[:-1]
    return lines


@dataclass
class Result:
    addr: str
    name: str
    symbol: str | None
    orig_size: int
    new_size: int
    score: float
    status: str
    file: str


class Comparator:
    def __init__(self, orig: NEFile, new: NEFile, publics: dict[str, tuple[int, int]],
                 annotations: list[Annotation]):
        self.orig, self.new, self.publics = orig, new, publics
        self.symbol_of = {id(a): resolve_symbol(a, publics) for a in annotations}

        orig_syms = {}
        segmap: dict[int, int] = {}
        for a in annotations:
            sym = self.symbol_of[id(a)]
            orig_syms[(a.seg, a.off)] = normalize(sym or a.name)
            if sym and a.kind != "GLOBAL":
                segmap.setdefault(publics[sym][0], a.seg)
        globals_ = {self.symbol_of[id(a)] for a in annotations if a.kind == "GLOBAL"}
        new_syms = {addr: normalize(n) for n, addr in publics.items()
                    if addr[0] != new.auto_data_seg or n in globals_}
        sizes = data_sizes(publics, new.auto_data_seg, new.segments[new.auto_data_seg].min_alloc)
        orig_sizes = {a.off: sizes[self.symbol_of[id(a)]] for a in annotations
                      if a.kind == "GLOBAL" and self.symbol_of[id(a)] in sizes}
        new_sizes = {publics[n][1]: sizes[n] for n in globals_ if n in sizes}
        # Segments holding no decompiled code yet: the original's are real
        # segments, the rebuilt ones hold the generated stubs.
        self.open_orig = {s.index for s in orig.segments.values() if s.is_code} - set(segmap.values())
        self.open_new = {s.index for s in new.segments.values() if s.is_code} - set(segmap)
        self.orig_sym = Symbolizer(orig, orig_syms, data_sizes=orig_sizes)
        self.new_sym = Symbolizer(new, new_syms,
                                  seg_alias=lambda s: f"seg{segmap[s]}" if s in segmap else None,
                                  prefix="new:", data_sizes=new_sizes)

        orig_starts = {s.index: {f[0] for f in function_extents(orig, s.index)}
                       for s in orig.segments.values() if s.is_code}
        # Annotations also start functions the scan cannot see, such as the
        # five-byte MMSYSTEM thunks that are only reached by far calls.
        for a in annotations:
            if a.kind != "GLOBAL" and a.seg in orig_starts:
                orig_starts[a.seg].add(a.off)
        self.orig_starts = {seg: sorted(v) for seg, v in orig_starts.items()}
        new_starts = {s.index: {f[0] for f in function_extents(new, s.index)}
                      for s in new.segments.values() if s.is_code}
        for seg, off in publics.values():
            if seg in new_starts:
                new_starts[seg].add(off)
        self.new_starts = {seg: sorted(v) for seg, v in new_starts.items()}

    def lines(self, a: Annotation) -> tuple[list[Line], list[Line]]:
        ol = extent(self.orig_starts.get(a.seg, []), self.orig.segments[a.seg].length, a.off)
        old = disassemble(self.orig, a.seg, a.off, ol, self.orig_sym)
        sym = self.symbol_of[id(a)]
        if not sym:
            return old, []
        seg, off = self.publics[sym]
        nl = extent(self.new_starts.get(seg, []), self.new.segments[seg].length, off)
        if a.kind == "LIBRARY" and nl < ol:
            # Library code is linked unchanged; unnamed library helpers that
            # follow it in the original are not part of this function.
            old = disassemble(self.orig, a.seg, a.off, nl, self.orig_sym)
        return _trim_padding(old), _trim_padding(disassemble(self.new, seg, off, nl, self.new_sym))

    def compare(self, a: Annotation) -> Result:
        old, new = self.lines(a)
        sym = self.symbol_of[id(a)]
        osize = sum(len(x.raw) for x in old)
        nsize = sum(len(x.raw) for x in new)
        base = Result(a.addr(), a.name, sym, osize, nsize, 0.0, "", f"{a.file.relative_to(ROOT)}:{a.line}")
        if not sym:
            base.status = "not in map"
            return base
        ot, nt = [self._key(x, self.open_orig, "") for x in old], [self._key(x, self.open_new, "new:") for x in new]
        base.score = difflib.SequenceMatcher(None, ot, nt, autojunk=False).ratio()
        if (osize == nsize and len(old) == len(new)
                and all(self.same(o, n) for o, n in zip(old, new))):
            exact = all(o.raw == n.raw or o.symbolic for o, n in zip(old, new))
            base.status = "exact" if exact else "equivalent"
            base.score = 1.0
        else:
            base.status = "differs"
        if a.kind == "STUB":
            base.status = "stub"
        return base

    @staticmethod
    def _key(line: Line, open_segs: set[int], prefix: str) -> str:
        """Comparison text: segments without decompiled code become `seg?`,
        since a stub's segment says nothing about where the original lives.
        A near target with no public keeps its address; drop the rebuilt
        side's prefix so `0001:0614` matches `new:0001:0614`."""
        text = re.sub(rf"\b{prefix}seg(\d+)\b",
                      lambda m: "seg?" if int(m.group(1)) in open_segs else m.group(0), line.text)
        return text.replace(prefix, "") if prefix else text

    def same(self, o: Line, n: Line) -> bool:
        """Equal text, or identical bytes without relocations (an immediate
        that merely happens to equal a global's address on one side)."""
        if self._key(o, self.open_orig, "") == self._key(n, self.open_new, "new:"):
            return True
        # A constant that is a plain number on one side and falls inside a
        # global on the other only because the data layouts differ.
        # Both sides can name it, as different globals, when the layouts
        # each contain something at that address (`mov [g], 0x5a`).
        if o.plain_imm is not None and o.plain_imm == n.plain_imm:
            return True
        if (o.plain_imm is None) != (n.plain_imm is None):
            if (o.plain_imm or o.text) == (n.plain_imm or n.text):
                return True
        # The same for a displacement (a struct offset in `[bx+0x6a]`).
        if o.plain_mem is not None and o.plain_mem == n.plain_mem:
            return True
        if (o.plain_mem is None) != (n.plain_mem is None):
            if (o.plain_mem or o.text) == (n.plain_mem or n.text):
                return True
        # `g_a[i - 1]`: the displacement is `&g_a[-1]`, inside a different
        # neighbour in each layout.
        if o.before is not None and o.before == n.before:
            return True
        return o.raw == n.raw and not o.symbolic and not n.symbolic

    def diff(self, a: Annotation) -> str:
        old, new = self.lines(a)
        ot = [f"{x.offset - old[0].offset:04X}  {x.text}" for x in old]
        nt = [f"{x.offset - new[0].offset:04X}  {x.text}" for x in new]
        width = max((len(s) for s in ot), default=10) + 2
        out = [f"{'original ' + a.addr():<{width}}| rebuilt"]
        sm = difflib.SequenceMatcher(None, [self._key(x, self.open_orig, "") for x in old],
                                     [self._key(x, self.open_new, "new:") for x in new], autojunk=False)
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            for k in range(max(i2 - i1, j2 - j1)):
                o = old[i1 + k] if i1 + k < i2 else None
                n = new[j1 + k] if j1 + k < j2 else None
                mark = " " if tag == "equal" or (o and n and self.same(o, n)) else "!"
                left = ot[i1 + k] if o else ""
                right = nt[j1 + k] if n else ""
                out.append(f"{left:<{width}}{mark} {right}")
        return "\n".join(out)


def _bytes_match(odata: bytes | bytearray, ndata: bytes | bytearray,
                 o_off: int, n_off: int, size: int) -> bool:
    return bytes(odata[o_off:o_off + size]) == bytes(ndata[n_off:n_off + size])


def data_coverage(orig: NEFile, new: NEFile, items: list[Item], annotations: list[Annotation],
                  publics: dict[str, tuple[int, int]]) -> tuple[str, list[str], dict[str, dict[str, int]]]:
    """Summary line for the data segment and the items no GLOBAL annotation
    names. An annotation covers the rebuilt item at its symbol's address
    (statics: at its own address) when the original has the same bytes there.

    The third return value maps each module stem to total and matched byte
    counts. Matched means a GLOBAL annotation covers the item with identical
    bytes, or the item is a string-literal pool / alignment padding whose
    original and rebuilt bytes match."""
    seg = orig.auto_data_seg
    odata, ndata = orig.segments[seg].data, new.segments[new.auto_data_seg].data
    total = orig.segments[seg].min_alloc
    at = {it.off: it for it in items}
    covered: set[int] = set()
    notes = []
    for a in annotations:
        if a.kind != "GLOBAL" or a.seg != seg:
            continue
        sym = resolve_symbol(a, publics)
        off = publics[sym][1] if sym else a.off
        it = at.get(off)
        if not it:
            notes.append(f"{a.addr()} {a.name}: no data item starts at rebuilt {off:04X}")
        elif not _bytes_match(odata, ndata, a.off, it.off, it.size):
            notes.append(f"{a.addr()} {a.name}: bytes differ from the rebuilt {it.name or 'item'} at {off:04X}")
        else:
            covered.add(it.off)
    # Literal pools and inter-contribution padding have no GLOBAL; match them
    # when the rebuilt bytes at the same offset are identical.
    for it in items:
        if it.off in covered:
            continue
        if it.name == LITERALS or it.module == "(padding)":
            if _bytes_match(odata, ndata, it.off, it.off, it.size):
                covered.add(it.off)
    named = sum(it.size for it in items if it.off in covered and it.name != LITERALS
                and it.module != "(padding)")
    lit_total = sum(it.size for it in items if it.name == LITERALS)
    lit_matched = sum(it.size for it in items if it.name == LITERALS and it.off in covered)
    pad_total = sum(it.size for it in items if it.module == "(padding)")
    pad_matched = sum(it.size for it in items if it.module == "(padding)" and it.off in covered)
    matched = sum(it.size for it in items if it.off in covered)
    same = sum(o == n for o, n in zip(bytes(odata).ljust(total, b"\0")[:total],
                                      bytes(ndata).ljust(total, b"\0")[:total]))
    line = (f"{matched}/{total} data bytes ({matched / total:.2%}) matched "
            f"({named} annotated, {lit_matched}/{lit_total} string literals, "
            f"{pad_matched}/{pad_total} padding); {same}/{total} identical to the original")
    for it in items:
        if it.off not in covered and it.name != LITERALS and it.module != "(padding)":
            what = it.name or "(unnamed)"
            how = "" if it.sized else " (size from the next public)"
            notes.append(f"0024:{it.off:04X} {it.size:5}  {what:24} {it.module}{how}")
    by_module: dict[str, dict[str, int]] = {}
    for it in items:
        slot = by_module.setdefault(it.module, {"total": 0, "matched": 0})
        slot["total"] += it.size
        if it.off in covered:
            slot["matched"] += it.size
    # Trailing bytes past the last contribution (rare) still count toward the
    # segment total; attribute them as padding when they match.
    claimed = sum(it.size for it in items)
    if claimed < total:
        tail = total - claimed
        slot = by_module.setdefault("(padding)", {"total": 0, "matched": 0})
        slot["total"] += tail
        if _bytes_match(odata, ndata, claimed, claimed, tail):
            slot["matched"] += tail
    return line, notes, by_module


def code_gaps(results: list[Result], annotations: list[Annotation],
              orig: NEFile) -> list[dict]:
    """Uncovered stretches of each code segment. `zero` is true when every
    byte is 0x00 (word-alignment padding the linker inserts between modules)."""
    by_addr = {a.addr(): a for a in annotations}
    by_seg: dict[int, list[tuple[int, int]]] = {}
    for r in results:
        a = by_addr[r.addr]
        by_seg.setdefault(a.seg, []).append((a.off, r.orig_size))
    gaps = []
    for seg in sorted(s.index for s in orig.segments.values() if s.is_code):
        length = orig.segments[seg].length
        data = bytes(orig.segments[seg].data)
        funcs = sorted(by_seg.get(seg, []), key=lambda t: t[0])
        if not funcs:
            chunk = data[:length]
            gaps.append({
                "addr": f"{seg:04X}:0000",
                "size": length,
                "zero": length == 0 or all(b == 0 for b in chunk),
            })
            continue
        prev_end = 0
        for off, size in funcs:
            if off > prev_end:
                chunk = data[prev_end:off]
                gaps.append({
                    "addr": f"{seg:04X}:{prev_end:04X}",
                    "size": off - prev_end,
                    "zero": all(b == 0 for b in chunk),
                })
            prev_end = off + size
        if prev_end < length:
            chunk = data[prev_end:length]
            gaps.append({
                "addr": f"{seg:04X}:{prev_end:04X}",
                "size": length - prev_end,
                "zero": all(b == 0 for b in chunk),
            })
    return gaps


MODULES = (
    ("AARCADE", "AARCADE.EXE", "AARCADE.MAP"),
    ("AACOLOR", "AACOLOR.DLL", "AACOLOR.MAP"),
    ("AABW", "AABW.DLL", "AABW.MAP"),
)


def module_paths(name: str, orig: Path | None = None, rebuilt: Path | None = None,
                 map_path: Path | None = None) -> tuple[Path, Path, Path]:
    """Default orig/rebuilt/map paths for a module name."""
    binary = next(b for n, b, _ in MODULES if n == name)
    map_name = next(m for n, _, m in MODULES if n == name)
    return (
        orig or ROOT / "orig" / binary,
        rebuilt or ROOT / "build" / binary,
        map_path or ROOT / "build" / map_name,
    )


def module_inventory(results: list[Result], annotations: list[Annotation],
                     orig: NEFile, data_by_module: dict[str, dict[str, int]] | None,
                     data_total: int | None, linked: bool) -> dict:
    """One module's slice of the ByteMatch inventory."""
    by_addr = {a.addr(): a for a in annotations}
    functions = []
    for r in results:
        a = by_addr[r.addr]
        entry = {
            "addr": r.addr,
            "name": r.name,
            "file": r.file.split(":")[0],
            "segment": a.seg,
            "orig_size": r.orig_size,
            "score": r.score,
            "status": r.status,
        }
        if a.kind == "LIBRARY":
            entry["library"] = True
        functions.append(entry)
    segments = {
        f"{s.index:04X}": s.length
        for s in sorted(orig.segments.values(), key=lambda s: s.index)
        if s.is_code
    }
    data = {"total": data_total or 0, "modules": {}}
    if data_by_module is not None:
        data["modules"] = {
            mod: {"total": v["total"], "matched": v["matched"]}
            for mod, v in sorted(data_by_module.items())
        }
        if data_total is None:
            data["total"] = sum(v["total"] for v in data_by_module.values())
    return {
        "linked": linked,
        "functions": functions,
        "segments": segments,
        "gaps": code_gaps(results, annotations, orig),
        "data": data,
    }


def write_progress(path: Path, modules: dict[str, dict]) -> None:
    """Committed inventory for tools/report.py. Deterministic JSON so the file
    only changes when progress does. Top-level `linked` is true only when every
    module's rebuilt file is identical to the original."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({
        "linked": all(m.get("linked") for m in modules.values()),
        "modules": {name: modules[name] for name in sorted(modules)},
    }, indent=2) + "\n")


def compare_one(module: str, orig_path: Path, rebuilt_path: Path, map_path: Path,
                src: Path, selftest: bool = False
                ) -> tuple[list[Result], list[Annotation], NEFile, dict | None, int | None, bool]:
    """Compare one NE module. Returns results, code annotations, orig NE,
    data_by_module, data_total, and whether the files are byte-identical."""
    everything = parse_tree(src, module)
    annotations = [a for a in everything if a.kind in ("FUNCTION", "STUB", "LIBRARY")]
    orig = NEFile(str(orig_path))
    if selftest:
        new = orig
        publics = {f"FUN_{s.index:04X}_{off:04X}": (s.index, off)
                   for s in orig.segments.values() if s.is_code
                   for off, _, _ in function_extents(orig, s.index)}
        exports = {(e.seg, e.offset): orig.entry_names[o]
                   for o, e in orig.entries.items() if o in orig.entry_names}
        annotated = {(a.seg, a.off) for a in everything}
        publics = {k: v for k, v in publics.items() if v not in annotated and v not in exports}
        publics.update({name: addr for addr, name in exports.items()})
        for a in everything:
            publics[a.symbol or linker_candidates(a.name)[0]] = (a.seg, a.off)
        linked = True
    else:
        if not rebuilt_path.exists() or not map_path.exists():
            raise FileNotFoundError(f"missing {rebuilt_path} or {map_path}")
        new = NEFile(str(rebuilt_path))
        publics = parse_map(map_path)
        place_library_statics(everything, publics)
        linked = rebuilt_path.read_bytes() == orig_path.read_bytes()

    cmp = Comparator(orig, new, publics, everything)
    results = [cmp.compare(a) for a in sorted(annotations, key=lambda a: (a.seg, a.off))]
    data_by_module: dict[str, dict[str, int]] | None = None
    data_total: int | None = None
    listings = map_path.parent / "LST"
    if not selftest and listings.is_dir():
        items, warnings = layout(map_path, listings, new.auto_data_seg, publics)
        line, notes, data_by_module = data_coverage(orig, new, items, everything, publics)
        data_total = orig.segments[orig.auto_data_seg].min_alloc
        # Stash for the caller to print.
        compare_one.last_data = (line, notes, warnings)  # type: ignore[attr-defined]
    else:
        compare_one.last_data = None  # type: ignore[attr-defined]
    compare_one.last_cmp = cmp  # type: ignore[attr-defined]
    compare_one.last_annotations = annotations  # type: ignore[attr-defined]
    return results, annotations, orig, data_by_module, data_total, linked


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--orig", type=Path, default=None,
                    help="original NE file (default: orig/<module>)")
    ap.add_argument("--rebuilt", type=Path, default=None,
                    help="rebuilt NE file (default: build/<module>)")
    ap.add_argument("--map", type=Path, default=None,
                    help="TLINK map with publics (default: build/<module>.MAP)")
    ap.add_argument("--src", type=Path, default=ROOT / "src",
                    help="directory of annotated sources (default: src/)")
    ap.add_argument("--module", default="AARCADE",
                    help="annotation module name to filter on (default: AARCADE)")
    ap.add_argument("--diff", metavar="NAME_OR_ADDR", help="show a diff for one function")
    ap.add_argument("--verbose", action="store_true", help="show diffs for every non-matching function")
    ap.add_argument("--data", action="store_true",
                    help="list data-segment items that no GLOBAL annotation names")
    ap.add_argument("--json", type=Path, help="write results as JSON")
    ap.add_argument("--progress", type=Path, default=ROOT / "docs" / "progress.json",
                    help="write a ByteMatch inventory (default: docs/progress.json); "
                         "pass an empty path to skip")
    ap.add_argument("--selftest", action="store_true",
                    help="compare the original against itself using the annotations as a map")
    args = ap.parse_args()

    orig_path, rebuilt_path, map_path = module_paths(args.module, args.orig, args.rebuilt, args.map)

    if args.diff or args.selftest or args.json or args.data or (
            args.progress and str(args.progress) in ("", "-")):
        # Single-module path used by --diff / --selftest / --data / --json.
        try:
            results, annotations, orig, data_by_module, data_total, linked = compare_one(
                args.module, orig_path, rebuilt_path, map_path, args.src, args.selftest)
        except FileNotFoundError as e:
            print(e, file=sys.stderr)
            return 2
        cmp = compare_one.last_cmp  # type: ignore[attr-defined]

        if args.diff:
            key = args.diff.upper()
            hits = [a for a in annotations if a.name.upper() == key or a.addr() == key.zfill(9)]
            if not hits:
                print(f"no annotation matches {args.diff}", file=sys.stderr)
                return 2
            for a in hits:
                r = cmp.compare(a)
                print(f"{a.addr()} {a.name}: {r.status} {r.score:.1%}\n{cmp.diff(a)}")
            return 0

        print(f"{'address':9}  {'function':32} {'orig':>5} {'new':>5} {'score':>7}  status")
        for r in results:
            print(f"{r.addr:9}  {r.name[:32]:32} {r.orig_size:5} {r.new_size:5} {r.score:7.1%}  {r.status}")
            if args.verbose and r.status == "differs":
                a = next(x for x in annotations if x.addr() == r.addr)
                print(cmp.diff(a) + "\n")

        code_bytes = sum(s.length for s in orig.segments.values() if s.is_code)
        exact = [r for r in results if r.status in ("exact", "equivalent")]
        matched = sum(r.orig_size for r in exact)
        print(f"\n{len(exact)}/{len(results)} annotated functions match; "
              f"{matched}/{code_bytes} code bytes ({matched / code_bytes:.2%}) of the original")
        if compare_one.last_data:  # type: ignore[attr-defined]
            line, notes, warnings = compare_one.last_data  # type: ignore[attr-defined]
            print(line)
            for w in warnings:
                print(f"  warning: {w}")
            if args.data:
                print("\nunannotated data:\n" + "\n".join(notes) if notes else "\nunannotated data: (none)")
        if args.json:
            args.json.write_text(json.dumps([asdict(r) for r in results], indent=2))
        return 0

    # Full run: compare every module and write the combined inventory.
    modules: dict[str, dict] = {}
    for name, _, _ in MODULES:
        o, r, m = module_paths(name,
                               args.orig if name == args.module else None,
                               args.rebuilt if name == args.module else None,
                               args.map if name == args.module else None)
        try:
            results, annotations, orig, data_by_module, data_total, linked = compare_one(
                name, o, r, m, args.src)
        except FileNotFoundError as e:
            print(e, file=sys.stderr)
            return 2
        cmp = compare_one.last_cmp  # type: ignore[attr-defined]
        print(f"=== {name} ===")
        print(f"{'address':9}  {'function':32} {'orig':>5} {'new':>5} {'score':>7}  status")
        for res in results:
            print(f"{res.addr:9}  {res.name[:32]:32} {res.orig_size:5} {res.new_size:5} "
                  f"{res.score:7.1%}  {res.status}")
            if args.verbose and res.status == "differs":
                a = next(x for x in annotations if x.addr() == res.addr)
                print(cmp.diff(a) + "\n")
        code_bytes = sum(s.length for s in orig.segments.values() if s.is_code)
        exact = [res for res in results if res.status in ("exact", "equivalent")]
        matched = sum(res.orig_size for res in exact)
        print(f"\n{len(exact)}/{len(results)} annotated functions match; "
              f"{matched}/{code_bytes} code bytes ({matched / code_bytes:.2%}) of the original")
        if compare_one.last_data:  # type: ignore[attr-defined]
            line, notes, warnings = compare_one.last_data  # type: ignore[attr-defined]
            print(line)
            for w in warnings:
                print(f"  warning: {w}")
        modules[name] = module_inventory(
            results, annotations, orig, data_by_module, data_total, linked)
        print()

    if args.progress and str(args.progress) not in ("", "-"):
        write_progress(args.progress, modules)
        print(f"wrote {args.progress.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
