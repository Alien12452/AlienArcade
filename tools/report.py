#!/usr/bin/env python3
"""Build an objdiff progress report (v2) from docs/progress.json.

CI cannot run Borland or open the original EXE, so necmp.py writes a committed
inventory and this tool turns it into the JSON ByteMatch expects. Uses only
the standard library (safe to run in GitHub Actions without Capstone).

    python tools/report.py -o build/report.json
    python tools/report.py -i docs/progress.json -o build/report.json
"""
from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parent.parent
REPORT_VERSION = 2
LIBRARY_UNIT = "src/rtl.h"
CATEGORY_NAMES = {
    "game": "Game code",
    "library": "Runtime library",
}


def _pct(part: float, whole: float) -> float:
    """Empty total is 0%, not objdiff's 100%."""
    return 0.0 if whole == 0 else 100.0 * part / whole


def _empty_measures() -> dict:
    return {
        "fuzzy_match_percent": 0.0,
        "total_code": "0",
        "matched_code": "0",
        "matched_code_percent": 0.0,
        "total_data": "0",
        "matched_data": "0",
        "matched_data_percent": 0.0,
        "total_functions": 0,
        "matched_functions": 0,
        "matched_functions_percent": 0.0,
        "complete_code": "0",
        "complete_code_percent": 0.0,
        "complete_data": "0",
        "complete_data_percent": 0.0,
        "total_units": 0,
        "complete_units": 0,
    }


def _measures(total_code: int, matched_code: int, fuzzy_code: float,
              total_data: int, matched_data: int,
              total_functions: int, matched_functions: int,
              complete: bool, units: int = 1) -> dict:
    complete_code = total_code if complete else 0
    complete_data = total_data if complete else 0
    return {
        "fuzzy_match_percent": _pct(fuzzy_code, total_code),
        "total_code": str(total_code),
        "matched_code": str(matched_code),
        "matched_code_percent": _pct(matched_code, total_code),
        "total_data": str(total_data),
        "matched_data": str(matched_data),
        "matched_data_percent": _pct(matched_data, total_data),
        "total_functions": total_functions,
        "matched_functions": matched_functions,
        "matched_functions_percent": _pct(matched_functions, total_functions),
        "complete_code": str(complete_code),
        "complete_code_percent": _pct(complete_code, total_code),
        "complete_data": str(complete_data),
        "complete_data_percent": _pct(complete_data, total_data),
        "total_units": units,
        "complete_units": units if complete else 0,
    }


def _add_measures(total: dict, part: dict) -> None:
    def u(field: str) -> int:
        return int(total[field]) + int(part[field])

    total_code, matched_code = u("total_code"), u("matched_code")
    # Fuzzy percent is size-weighted, so sum it as bytes.
    part_fuzzy_bytes = int(part["total_code"]) * part["fuzzy_match_percent"] / 100.0
    total_fuzzy_bytes = int(total["total_code"]) * total["fuzzy_match_percent"] / 100.0
    fuzzy_bytes = total_fuzzy_bytes + part_fuzzy_bytes
    total_data, matched_data = u("total_data"), u("matched_data")
    total_functions = total["total_functions"] + part["total_functions"]
    matched_functions = total["matched_functions"] + part["matched_functions"]
    complete_code, complete_data = u("complete_code"), u("complete_data")
    total["fuzzy_match_percent"] = _pct(fuzzy_bytes, total_code)
    total["total_code"] = str(total_code)
    total["matched_code"] = str(matched_code)
    total["matched_code_percent"] = _pct(matched_code, total_code)
    total["total_data"] = str(total_data)
    total["matched_data"] = str(matched_data)
    total["matched_data_percent"] = _pct(matched_data, total_data)
    total["total_functions"] = total_functions
    total["matched_functions"] = matched_functions
    total["matched_functions_percent"] = _pct(matched_functions, total_functions)
    total["complete_code"] = str(complete_code)
    total["complete_code_percent"] = _pct(complete_code, total_code)
    total["complete_data"] = str(complete_data)
    total["complete_data_percent"] = _pct(complete_data, total_data)
    total["total_units"] += part["total_units"]
    total["complete_units"] += part["complete_units"]


def _parse_addr(addr: str) -> tuple[int, int]:
    seg, off = addr.split(":")
    return int(seg, 16), int(off, 16)


def _virtual_address(addr: str) -> str:
    seg, off = _parse_addr(addr)
    return str((seg << 16) | off)


def _module_unit(module: str, units: dict[str, dict]) -> str:
    """Map a TLINK module stem to a source-path unit name."""
    stem = module.upper()
    for path in units:
        if PurePosixPath(path).stem.upper() == stem:
            return path
    return LIBRARY_UNIT


def _gap_bytes(functions: list[dict], segments: dict[str, int]) -> dict[str, int]:
    """Unannotated bytes per unit: each gap goes to the following function's
    unit, or the previous one's when it sits at the end of a segment."""
    by_seg: dict[int, list[dict]] = defaultdict(list)
    for fn in functions:
        by_seg[fn["segment"]].append(fn)
    gaps: dict[str, int] = defaultdict(int)
    for key, length in segments.items():
        seg = int(key, 16)
        funcs = sorted(by_seg.get(seg, []), key=lambda f: _parse_addr(f["addr"])[1])
        if not funcs:
            gaps[LIBRARY_UNIT] += length
            continue
        prev_end = 0
        prev_file = funcs[0]["file"]
        for fn in funcs:
            off = _parse_addr(fn["addr"])[1]
            if off > prev_end:
                gaps[fn["file"]] += off - prev_end
            prev_end = off + fn["orig_size"]
            prev_file = fn["file"]
        if prev_end < length:
            gaps[prev_file] += length - prev_end
    return gaps


def build_report(inventory: dict) -> dict:
    functions = inventory["functions"]
    segments = inventory["segments"]
    data = inventory.get("data") or {"total": 0, "modules": {}}
    linked = bool(inventory.get("linked"))

    units: dict[str, dict] = {}
    for fn in functions:
        slot = units.setdefault(fn["file"], {
            "functions": [],
            "library": True,
            "gap": 0,
            "total_data": 0,
            "matched_data": 0,
        })
        slot["functions"].append(fn)
        if not fn.get("library"):
            slot["library"] = False

    for path, gap in _gap_bytes(functions, segments).items():
        units.setdefault(path, {
            "functions": [],
            "library": path == LIBRARY_UNIT,
            "gap": 0,
            "total_data": 0,
            "matched_data": 0,
        })
        units[path]["gap"] += gap

    modules = data.get("modules") or {}
    accounted = 0
    for mod, sizes in modules.items():
        path = _module_unit(mod, units)
        units.setdefault(path, {
            "functions": [],
            "library": path == LIBRARY_UNIT,
            "gap": 0,
            "total_data": 0,
            "matched_data": 0,
        })
        units[path]["total_data"] += sizes["total"]
        units[path]["matched_data"] += sizes["matched"]
        accounted += sizes["total"]
    remainder = max(0, int(data.get("total") or 0) - accounted)
    if remainder:
        units.setdefault(LIBRARY_UNIT, {
            "functions": [],
            "library": True,
            "gap": 0,
            "total_data": 0,
            "matched_data": 0,
        })
        units[LIBRARY_UNIT]["total_data"] += remainder

    report_units = []
    categories = {
        "game": {"id": "game", "name": CATEGORY_NAMES["game"], "measures": _empty_measures()},
        "library": {"id": "library", "name": CATEGORY_NAMES["library"], "measures": _empty_measures()},
    }
    total = _empty_measures()

    for path in sorted(units):
        slot = units[path]
        fns = sorted(slot["functions"], key=lambda f: _parse_addr(f["addr"]))
        matched_fns = [f for f in fns if f["status"] == "exact"]
        total_code = sum(f["orig_size"] for f in fns) + slot["gap"]
        matched_code = sum(f["orig_size"] for f in matched_fns)
        fuzzy_code = sum(f["orig_size"] * float(f["score"]) for f in fns)
        total_data = slot["total_data"]
        matched_data = slot["matched_data"]
        # "Complete" is ByteMatch's fully linked: only once the whole EXE is
        # identical (verify.py), never per unit from matched functions.
        complete = linked
        category = "library" if slot["library"] else "game"
        measures = _measures(
            total_code, matched_code, fuzzy_code,
            total_data, matched_data,
            len(fns), len(matched_fns),
            complete,
        )
        items = [{
            "name": f["name"],
            "size": str(f["orig_size"]),
            "fuzzy_match_percent": 100.0 * float(f["score"]),
            "metadata": {"virtual_address": _virtual_address(f["addr"])},
        } for f in fns]
        report_units.append({
            "name": path,
            "measures": measures,
            "functions": items,
            "metadata": {
                "complete": complete,
                "source_path": path,
                "progress_categories": [category],
            },
        })
        _add_measures(total, measures)
        _add_measures(categories[category]["measures"], measures)

    # Self-check: unit sums vs top-level, code total vs segment lengths.
    seg_total = sum(int(v) for v in segments.values())
    if int(total["total_code"]) != seg_total:
        raise SystemExit(
            f"total_code {total['total_code']} != sum of segment lengths {seg_total}")
    unit_code = sum(int(u["measures"]["total_code"]) for u in report_units)
    unit_matched = sum(int(u["measures"]["matched_code"]) for u in report_units)
    unit_data = sum(int(u["measures"]["total_data"]) for u in report_units)
    unit_mdata = sum(int(u["measures"]["matched_data"]) for u in report_units)
    if unit_code != int(total["total_code"]) or unit_matched != int(total["matched_code"]):
        raise SystemExit("unit code measures do not sum to the top-level totals")
    if unit_data != int(total["total_data"]) or unit_mdata != int(total["matched_data"]):
        raise SystemExit("unit data measures do not sum to the top-level totals")
    expected_data = int(data.get("total") or 0)
    if expected_data and int(total["total_data"]) != expected_data:
        raise SystemExit(
            f"total_data {total['total_data']} != inventory data.total {expected_data}")

    return {
        "measures": total,
        "units": report_units,
        "version": REPORT_VERSION,
        "categories": [categories["game"], categories["library"]],
    }


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("-i", "--inventory", type=Path, default=ROOT / "docs" / "progress.json",
                    help="committed inventory from necmp.py (default: docs/progress.json)")
    ap.add_argument("-o", "--output", type=Path, default=Path("build/report.json"),
                    help="objdiff report path (default: build/report.json)")
    args = ap.parse_args(argv)
    if not args.inventory.is_file():
        print(f"inventory not found: {args.inventory}", file=sys.stderr)
        return 2
    inventory = json.loads(args.inventory.read_text(encoding="utf-8"))
    report = build_report(inventory)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    m = report["measures"]
    print(
        f"{m['matched_functions']}/{m['total_functions']} functions, "
        f"{m['matched_code']}/{m['total_code']} code bytes "
        f"({m['matched_code_percent']:.2f}%), "
        f"{m['matched_data']}/{m['total_data']} data bytes "
        f"({m['matched_data_percent']:.2f}%)",
        file=sys.stderr,
    )
    print(f"wrote {args.output}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
