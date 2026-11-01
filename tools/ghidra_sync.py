#!/usr/bin/env python3
"""Push names from the source annotations into the Ghidra project.

Writes build/ghidra_annotations.tsv from ``// FUNCTION:`` / ``// GLOBAL:`` /
``// LIBRARY:`` comments under src/, then runs Ghidra headless (Flatpak
``org.ghidra_sre.Ghidra``) with ApplyAnnotations.java. The project must not be
open in the Ghidra GUI at the same time; with the GUI open, rename through
GhidraMCP instead.

Ghidra loads NE segment n at segment ``0x1000 + (n - 1) * 8`` with unchanged
offsets, so annotation ``0002:0000`` is Ghidra address ``1008:0000``.

    .venv/bin/python tools/ghidra_sync.py
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from annotations import parse_tree

ROOT = Path(__file__).resolve().parent.parent
GHIDRA = ["flatpak", "run", "--command=/app/lib/ghidra/support/analyzeHeadless", "org.ghidra_sre.Ghidra"]


def ghidra_addr(seg: int, off: int) -> str:
    """Map an NE segment:offset annotation to a Ghidra segmented address."""
    return f"{0x1000 + (seg - 1) * 8:04x}:{off:04x}"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--project-dir", type=Path, default=ROOT / "ghidra",
                    help="directory that holds the .gpr (default: ghidra/)")
    ap.add_argument("--project", default="AlienArcade",
                    help="Ghidra project name (default: AlienArcade)")
    ap.add_argument("--program", default="AARCADE.EXE",
                    help="program inside the project to rename (default: AARCADE.EXE)")
    ap.add_argument("--module", default="AARCADE",
                    help="annotation module filter (default: AARCADE)")
    args = ap.parse_args()

    tsv = ROOT / "build" / "ghidra_annotations.tsv"
    tsv.parent.mkdir(exist_ok=True)
    rows = [f"{a.kind}\t{ghidra_addr(a.seg, a.off)}\t{a.name}"
            for a in parse_tree(ROOT / "src", args.module)]
    tsv.write_text("\n".join(rows) + "\n")

    cmd = GHIDRA + [str(args.project_dir), args.project, "-process", args.program, "-noanalysis",
                    "-scriptPath", str(ROOT / "tools" / "ghidra_scripts"),
                    "-postScript", "ApplyAnnotations.java", str(tsv)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    for line in proc.stdout.splitlines():
        if "ApplyAnnotations.java>" in line or "ERROR" in line or "locked" in line.lower():
            print(line.strip())
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
