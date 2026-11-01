"""Parse source annotations tying decompiled code to the original binary.

Used by nedump, necmp, build (stubs), and ghidra_sync. Comment forms:

    // FUNCTION: AARCADE 0005:00AD
    LRESULT FAR PASCAL ModalDlgProc(HWND hDlg, ...)

The address is segment:offset in the original file (NE segment numbers,
1-based). An optional third token gives the linker symbol when it cannot be
guessed from the function name, e.g. a C++ mangled name:

    // FUNCTION: AARCADE 0010:0E09 @Swamp@Update$qv

Borland runtime library functions are linked from CWM.LIB rather than
compiled from src/; LIBRARY annotations name them by linker symbol:

    // LIBRARY: AARCADE 0001:08C8 _time

Assembly sources use ``;`` comments and always give the symbol:

    ; FUNCTION: AARCADE 0001:01E7 WAVEOUTOPEN

Call ``parse_tree(src_dir, module)`` to collect annotations under a tree.
"""
from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

ANNOT_RE = re.compile(
    r"(?://|;)\s*(FUNCTION|STUB|GLOBAL|LIBRARY):\s*(\w+)\s+([0-9A-Fa-f]{1,4}):([0-9A-Fa-f]{1,4})(?:\s+(\S+))?")
NAME_RE = re.compile(r"([A-Za-z_~][\w:~]*)\s*\(")
SOURCE_GLOBS = ("*.c", "*.cpp", "*.h", "*.asm")


@dataclass
class Annotation:
    kind: str  # FUNCTION, STUB, GLOBAL or LIBRARY
    module: str
    seg: int
    off: int
    name: str
    symbol: str | None
    file: Path
    line: int

    def addr(self) -> str:
        return f"{self.seg:04X}:{self.off:04X}"


def _function_name(lines: list[str], start: int) -> str:
    text = ""
    for ln in lines[start:start + 8]:
        if ANNOT_RE.search(ln):
            continue
        text += " " + ln
        if "{" in ln or ";" in ln:
            break
    m = NAME_RE.search(text)
    return m.group(1) if m else "?"


def parse_file(path: Path) -> list[Annotation]:
    """Parse every FUNCTION / STUB / GLOBAL / LIBRARY annotation in one file."""
    lines = path.read_text(encoding="latin-1").splitlines()
    out = []
    for i, ln in enumerate(lines):
        m = ANNOT_RE.search(ln)
        if not m:
            continue
        kind, module, seg, off, symbol = m.groups()
        if kind == "LIBRARY" or (symbol and (kind == "GLOBAL" or path.suffix.lower() == ".asm")):
            name = symbol or "?"
        elif kind == "GLOBAL":
            nm = re.search(r"([A-Za-z_]\w*)\s*(\[|=|;)", " ".join(lines[i + 1:i + 3]))
            name = nm.group(1) if nm else "?"
        else:
            name = _function_name(lines, i + 1)
        out.append(Annotation(kind, module.upper(), int(seg, 16), int(off, 16), name,
                              symbol, path, i + 1))
    return out


def parse_tree(root: Path, module: str | None = None) -> list[Annotation]:
    """Collect annotations under ``root`` (*.c / *.cpp / *.h / *.asm).

    When ``module`` is set, only annotations whose first token matches
    (e.g. ``AARCADE``) are kept.
    """
    out = []
    for pattern in SOURCE_GLOBS:
        for p in sorted(root.rglob(pattern)):
            out.extend(parse_file(p))
    if module:
        out = [a for a in out if a.module == module.upper()]
    return out


def linker_candidates(name: str) -> list[str]:
    """Borland C/C++ public names a function called `name` may link as."""
    if "::" in name:
        cls, meth = name.rsplit("::", 1)
        return ["@" + cls.replace("::", "@") + "@" + meth + "$"]
    return ["_" + name, name, name.upper(), "@" + name + "$"]
