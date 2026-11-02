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
DLL_LIBRARY_UNIT = "src/datalib/rtl.h"
CATEGORY_NAMES = {
    "game": "Game code",
    "library": "Runtime library",
    "dlls": "Picture DLLs",
}
DLL_MODULES = frozenset({"AACOLOR", "AABW"})


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


def _library_unit(module_name: str | None) -> str:
    return DLL_LIBRARY_UNIT if module_name in DLL_MODULES else LIBRARY_UNIT


def _unit_path(file_path: str, module_name: str | None) -> str:
    """EXE units keep their src/ paths; DLL units are `MODULE/basename`."""
    if module_name in DLL_MODULES:
        base = PurePosixPath(file_path).name
        return f"{module_name}/{base}"
    return file_path


def _module_unit(module: str, units: dict[str, dict], module_name: str | None) -> str:
    """Map a TLINK module stem to a source-path unit name."""
    stem = module.upper()
    for path in units:
        base = PurePosixPath(path).stem.upper()
        if base == stem:
            return path
    return _unit_path(_library_unit(module_name), module_name)


def _gap_owner(functions: list[dict], segments: dict[str, int],
               addr: str, size: int, library_unit: str) -> str:
    """Unit that owns an uncovered stretch: the following function, or the
    previous one at the end of a segment."""
    seg, off = _parse_addr(addr)
    funcs = sorted(
        (f for f in functions if f["segment"] == seg),
        key=lambda f: _parse_addr(f["addr"])[1],
    )
    if not funcs:
        return library_unit
    for fn in funcs:
        fn_off = _parse_addr(fn["addr"])[1]
        if fn_off >= off + size:
            return fn["file"]
    return funcs[-1]["file"]


def _gap_bytes(functions: list[dict], segments: dict[str, int],
               gap_list: list[dict] | None,
               library_unit: str = LIBRARY_UNIT) -> tuple[dict[str, int], int]:
    """Unmatched non-padding gap bytes per unit, and the padding byte count.

    When `gap_list` is present (from necmp), all-zero gaps are padding and are
    left out of total_code. Older inventories without the list fall back to
    counting every uncovered stretch as unmatched."""
    gaps: dict[str, int] = defaultdict(int)
    padding = 0
    if gap_list is not None:
        for g in gap_list:
            size = int(g["size"])
            if g.get("zero"):
                padding += size
                continue
            gaps[_gap_owner(functions, segments, g["addr"], size, library_unit)] += size
        return gaps, padding

    by_seg: dict[int, list[dict]] = defaultdict(list)
    for fn in functions:
        by_seg[fn["segment"]].append(fn)
    for key, length in segments.items():
        seg = int(key, 16)
        funcs = sorted(by_seg.get(seg, []), key=lambda f: _parse_addr(f["addr"])[1])
        if not funcs:
            gaps[library_unit] += length
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
    return gaps, 0


def _is_library_unit(path: str, lib_unit: str) -> bool:
    return path == lib_unit or path == LIBRARY_UNIT or path.endswith("/" + DLL_LIBRARY_UNIT)


def _collect_units(mod_inv: dict, module_name: str | None,
                   linked: bool) -> tuple[list[dict], int, int]:
    """Build report units for one binary module. Returns units, padding bytes,
    and expected data total for self-checks."""
    # Prefix DLL source paths so AACOLOR and AABW do not collide.
    functions = [
        {**fn, "file": _unit_path(fn["file"], module_name)}
        for fn in mod_inv["functions"]
    ]
    segments = mod_inv["segments"]
    data = mod_inv.get("data") or {"total": 0, "modules": {}}
    gap_list = mod_inv.get("gaps")
    lib_unit = _unit_path(_library_unit(module_name), module_name)

    units: dict[str, dict] = {}
    for fn in functions:
        path = fn["file"]
        slot = units.setdefault(path, {
            "functions": [],
            "library": True,
            "gap": 0,
            "total_data": 0,
            "matched_data": 0,
        })
        slot["functions"].append(fn)
        if not fn.get("library"):
            slot["library"] = False

    # Gap owners use un-prefixed paths from the inventory; re-prefix for DLLs.
    raw_funcs = mod_inv["functions"]
    gap_by_unit, padding = _gap_bytes(
        raw_funcs, segments, gap_list, _library_unit(module_name))
    for path, gap in gap_by_unit.items():
        path = _unit_path(path, module_name)
        units.setdefault(path, {
            "functions": [],
            "library": _is_library_unit(path, lib_unit),
            "gap": 0,
            "total_data": 0,
            "matched_data": 0,
        })
        units[path]["gap"] += gap

    accounted = 0
    for mod, sizes in (data.get("modules") or {}).items():
        path = _module_unit(mod, units, module_name)
        units.setdefault(path, {
            "functions": [],
            "library": _is_library_unit(path, lib_unit),
            "gap": 0,
            "total_data": 0,
            "matched_data": 0,
        })
        units[path]["total_data"] += sizes["total"]
        units[path]["matched_data"] += sizes["matched"]
        accounted += sizes["total"]
    remainder = max(0, int(data.get("total") or 0) - accounted)
    if remainder:
        print(
            f"warning: {remainder} data bytes not claimed by any module item"
            + (f" ({module_name})" if module_name else ""),
            file=sys.stderr,
        )
        units.setdefault(lib_unit, {
            "functions": [],
            "library": True,
            "gap": 0,
            "total_data": 0,
            "matched_data": 0,
        })
        units[lib_unit]["total_data"] += remainder

    report_units = []
    module_code = 0
    for path in sorted(units):
        slot = units[path]
        fns = sorted(slot["functions"], key=lambda f: _parse_addr(f["addr"]))
        matched_fns = [f for f in fns if f["status"] == "exact"]
        total_code = sum(f["orig_size"] for f in fns) + slot["gap"]
        matched_code = sum(f["orig_size"] for f in matched_fns)
        fuzzy_code = sum(f["orig_size"] * float(f["score"]) for f in fns)
        complete = linked
        category = "library" if slot["library"] else "game"
        cats = [category]
        if module_name in DLL_MODULES:
            cats.append("dlls")
        measures = _measures(
            total_code, matched_code, fuzzy_code,
            slot["total_data"], slot["matched_data"],
            len(fns), len(matched_fns),
            complete,
        )
        items = [{
            "name": f["name"],
            "size": str(f["orig_size"]),
            "fuzzy_match_percent": 100.0 * float(f["score"]),
            "metadata": {"virtual_address": _virtual_address(f["addr"])},
        } for f in fns]
        # DLL unit names are MODULE/basename; point source_path at the real file.
        if module_name in DLL_MODULES:
            source = DLL_LIBRARY_UNIT if path.endswith("/rtl.h") else "src/datalib/libmain.c"
        else:
            source = path
        meta = {
            "complete": complete,
            "source_path": source,
            "progress_categories": cats,
        }
        if module_name:
            meta["module_name"] = module_name
        report_units.append({
            "name": path,
            "measures": measures,
            "functions": items,
            "metadata": meta,
        })
        module_code += total_code

    seg_total = sum(int(v) for v in segments.values())
    fn_bytes = sum(f["orig_size"] for f in functions)
    other_gaps = sum(gap_by_unit.values())
    label = module_name or "AARCADE"
    if gap_list is not None:
        if fn_bytes + padding + other_gaps != seg_total:
            raise SystemExit(
                f"{label}: functions {fn_bytes} + padding {padding} + gaps {other_gaps} "
                f"!= segment lengths {seg_total}")
        if module_code != fn_bytes + other_gaps:
            raise SystemExit(
                f"{label}: unit code {module_code} != functions {fn_bytes} "
                f"+ non-zero gaps {other_gaps}")
    elif module_code != seg_total:
        raise SystemExit(
            f"{label}: unit code {module_code} != sum of segment lengths {seg_total}")
    return report_units, padding, int(data.get("total") or 0)


def build_report(inventory: dict) -> dict:
    # New multi-module inventory, or a flat single-module one.
    if "modules" in inventory:
        mod_map = inventory["modules"]
        linked = bool(inventory.get("linked"))
    else:
        mod_map = {"AARCADE": inventory}
        linked = bool(inventory.get("linked"))

    categories = {
        "game": {"id": "game", "name": CATEGORY_NAMES["game"], "measures": _empty_measures()},
        "library": {"id": "library", "name": CATEGORY_NAMES["library"], "measures": _empty_measures()},
        "dlls": {"id": "dlls", "name": CATEGORY_NAMES["dlls"], "measures": _empty_measures()},
    }
    total = _empty_measures()
    report_units: list[dict] = []
    expected_data_sum = 0

    for name in sorted(mod_map):
        mod_inv = mod_map[name]
        module_name = name if (name in DLL_MODULES or len(mod_map) > 1) else None
        units, _padding, expected_data = _collect_units(mod_inv, module_name, linked)
        expected_data_sum += expected_data
        for u in units:
            report_units.append(u)
            _add_measures(total, u["measures"])
            for cat in u["metadata"]["progress_categories"]:
                if cat in categories:
                    _add_measures(categories[cat]["measures"], u["measures"])

    report_units.sort(key=lambda u: u["name"])

    unit_code = sum(int(u["measures"]["total_code"]) for u in report_units)
    unit_matched = sum(int(u["measures"]["matched_code"]) for u in report_units)
    unit_data = sum(int(u["measures"]["total_data"]) for u in report_units)
    unit_mdata = sum(int(u["measures"]["matched_data"]) for u in report_units)
    if unit_code != int(total["total_code"]) or unit_matched != int(total["matched_code"]):
        raise SystemExit("unit code measures do not sum to the top-level totals")
    if unit_data != int(total["total_data"]) or unit_mdata != int(total["matched_data"]):
        raise SystemExit("unit data measures do not sum to the top-level totals")
    if expected_data_sum and int(total["total_data"]) != expected_data_sum:
        raise SystemExit(
            f"total_data {total['total_data']} != inventory data.total {expected_data_sum}")

    cats = [categories["game"], categories["library"]]
    if int(categories["dlls"]["measures"]["total_code"]) or int(categories["dlls"]["measures"]["total_data"]):
        cats.append(categories["dlls"])
    return {
        "measures": total,
        "units": report_units,
        "version": REPORT_VERSION,
        "categories": cats,
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
