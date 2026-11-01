#!/usr/bin/env python3
"""Extract resources from a local copy of the Alien Arcade binaries.

The pictures, sounds and dialogs are copyrighted and do not belong in this
repository. setup_orig.py expands your own copy of the game into orig/; this
script reads those files and writes a Borland .RC plus the files it includes.
The output directory is gitignored.

    .venv/bin/python tools/extract_res.py
    .venv/bin/python tools/extract_res.py orig/AARCADE.EXE --out build/res

AARCADE.RC is the executable: dialogs, strings, accelerators, the icon and
the WAVE sounds. The pictures are in the data libraries, AACOLOR.RC (colour)
and AABW.RC (black and white), which the game loads by name. AACOLOR's
trailing CodeView (NB02) overlay is saved as AACOLOR/CODEVIEW.bin so the
build can append it without reading orig/ again. tools/build.py compiles
and binds these scripts when they are present.
"""
from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BINS = ("AARCADE.EXE", "AACOLOR.DLL", "AABW.DLL", "METER.DLL")

# Integer resource types. Custom types are named through the name table.
RT_NAMES = {
    1: "CURSOR", 2: "BITMAP", 3: "ICON", 4: "MENU", 5: "DIALOG",
    6: "STRING", 7: "FONTDIR", 8: "FONT", 9: "ACCELERATOR", 10: "RCDATA",
    12: "GROUP_CURSOR", 14: "GROUP_ICON", 15: "NAMETABLE", 16: "VERSION",
}
CTL_CLASS = {
    0x80: "BUTTON", 0x81: "EDIT", 0x82: "STATIC",
    0x83: "LISTBOX", 0x84: "SCROLLBAR", 0x85: "COMBOBOX",
}
EXT = {"WAVE": "wav", "PPIC": "pic", "PPCT": "pct", "PICT": "mac"}


@dataclass
class Resource:
    type_id: int
    res_id: int
    flags: int
    data: bytes
    kind: str


def cstr(data: bytes, pos: int) -> tuple[str, int]:
    end = data.index(0, pos)
    return data[pos:end].decode("latin-1"), end + 1


def align4(pos: int) -> int:
    return (pos + 3) & ~3


def parse_resources(path: Path) -> list[Resource]:
    """NE resource table. Lengths in the table are in alignment units, so
    each record is padded; logical_size() drops that padding."""
    raw = path.read_bytes()
    if raw[:2] != b"MZ":
        raise SystemExit(f"{path}: not an MZ executable")
    ne = struct.unpack_from("<I", raw, 0x3C)[0]
    if raw[ne:ne + 2] != b"NE":
        raise SystemExit(f"{path}: not an NE executable")
    base = ne + struct.unpack_from("<H", raw, ne + 0x24)[0]
    shift = struct.unpack_from("<H", raw, base)[0]
    pos = base + 2
    found: list[tuple[int, int, int, bytes]] = []
    while True:
        type_id = struct.unpack_from("<H", raw, pos)[0]
        if type_id == 0:
            break
        count = struct.unpack_from("<H", raw, pos + 2)[0]
        pos += 8
        number = type_id & 0x7FFF if type_id & 0x8000 else None
        if number is None:
            raise SystemExit(f"{path}: resource type is a string, not an ordinal")
        for _ in range(count):
            off, length, flags, res_id, _, _ = struct.unpack_from("<HHHHHH", raw, pos)
            pos += 12
            start = off << shift
            found.append((number, res_id & 0x7FFF, flags,
                          raw[start:start + (length << shift)]))
    names = name_table(next((data for tid, _, _, data in found if tid == 15), b""))
    resources = []
    for type_id, res_id, flags, data in found:
        if type_id in (0x7F01, 0x7F02):
            kind = names.get((type_id, res_id))
            if kind is None:
                raise SystemExit(f"{path}: type {type_id:#x} id {res_id} has no name")
        else:
            kind = RT_NAMES.get(type_id, f"TYPE{type_id}")
        resources.append(Resource(type_id, res_id, flags, data, kind))
    return resources


def name_table(data: bytes) -> dict[tuple[int, int], str]:
    """RT_NAMETABLE entries: the string FindResource looks up, per resource."""
    names = {}
    pos = 0
    while pos + 2 <= len(data) and struct.unpack_from("<H", data, pos)[0]:
        size = struct.unpack_from("<H", data, pos)[0]
        type_id, res_id = struct.unpack_from("<HH", data, pos + 2)
        name = data[pos + 6:pos + size].split(b"\0", 1)[0].decode("latin-1")
        names[(type_id & 0x7FFF, res_id)] = name
        pos += size
    return names


def logical_size(res: Resource) -> int:
    data = res.data
    if res.kind == "WAVE":
        if data[:4] != b"RIFF":
            raise ValueError("not a RIFF")
        size = struct.unpack_from("<I", data, 4)[0] + 8
    elif res.kind in ("PPIC", "PPCT"):
        count = struct.unpack_from(">H", data, 0)[0]
        size = 2
        for _ in range(count):
            chunk = struct.unpack_from(">I", data, size)[0]
            if chunk < 4:
                raise ValueError("short picture chunk")
            size += chunk
    elif res.kind == "PICT":
        width, height = struct.unpack_from(">HH", data, 0)
        size = 4 + ((width + 15) // 16) * 2 * height
    elif res.kind == "DIALOG":
        size = dialog_end(data)
    elif res.kind == "STRING":
        size = 0
        for _ in range(16):
            size += 1 + data[size]
    elif res.kind == "ACCELERATOR":
        size = 0
        while True:
            flags = data[size]
            size += 5
            if flags & 0x80:
                break
    elif res.kind == "VERSION":
        size = struct.unpack_from("<H", data, 0)[0]
    elif res.kind in ("ICON", "BITMAP"):
        size = dib_size(data)
    elif res.kind == "CURSOR":
        size = 4 + dib_size(data[4:])
    elif res.kind in ("GROUP_ICON", "GROUP_CURSOR"):
        count = struct.unpack_from("<H", data, 4)[0]
        size = 6 + count * 14
    elif res.kind == "NAMETABLE":
        size = 0
        while size + 2 <= len(data) and struct.unpack_from("<H", data, size)[0]:
            size += struct.unpack_from("<H", data, size)[0]
    else:
        raise ValueError(f"unknown resource {res.kind}")
    if size <= 0 or size > len(data) or data[size:] != b"\x00" * (len(data) - size):
        raise ValueError(f"padding is not empty ({size} of {len(data)})")
    return size


def dib_size(data: bytes) -> int:
    if struct.unpack_from("<I", data, 0)[0] != 40:
        raise ValueError("not a BITMAPINFOHEADER")
    width, height, _planes, bits = struct.unpack_from("<iiHH", data, 4)
    image_height = height // 2
    colours = (1 << bits) * 4 if bits <= 8 else 0
    xor_row = ((width * bits + 31) // 32) * 4
    and_row = ((width + 31) // 32) * 4
    return 40 + colours + (xor_row + and_row) * image_height


def dialog_end(data: bytes) -> int:
    """Byte after the last control. Items are packed; there is no pad byte."""
    style = struct.unpack_from("<I", data, 0)[0]
    count = data[4]
    pos = 13

    def skip_name(pos: int) -> int:
        if data[pos] == 0xFF:
            return pos + 3
        if data[pos] == 0:
            return pos + 1
        _, pos = cstr(data, pos)
        return pos

    pos = skip_name(pos)
    pos = skip_name(pos)
    _, pos = cstr(data, pos)
    if style & 0x40:
        pos += 2
        _, pos = cstr(data, pos)
    for _ in range(count):
        pos += 14
        if data[pos] & 0x80:
            pos += 1
        else:
            _, pos = cstr(data, pos)
        if data[pos] == 0xFF:
            pos += 3
        else:
            _, pos = cstr(data, pos)
        pos += 1 + data[pos]
    return pos


def payload(res: Resource) -> bytes:
    return res.data[:logical_size(res)]


def rc_quote(text: str) -> str:
    out = ['"']
    for ch in text:
        code = ord(ch)
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\t":
            out.append("\\t")
        elif code < 32 or code > 126:
            out.append(f"\\{code:03o}")
        else:
            out.append(ch)
    out.append('"')
    return "".join(out)


def mem_flags(flags: int) -> str:
    parts = []
    parts.append("PRELOAD" if flags & 0x40 else "LOADONCALL")
    if flags & 0x10:
        parts.append("MOVEABLE")
    if flags & 0x1000:
        parts.append("DISCARDABLE")
    return " ".join(parts)


def dialog_lines(res: Resource) -> list[str]:
    data = payload(res)
    style = struct.unpack_from("<I", data, 0)[0]
    count = data[4]
    x, y, cx, cy = struct.unpack_from("<hhhh", data, 5)
    pos = 13

    def take_name(pos: int):
        if data[pos] == 0xFF:
            return struct.unpack_from("<H", data, pos + 1)[0], pos + 3
        if data[pos] == 0:
            return None, pos + 1
        text, pos = cstr(data, pos)
        return text, pos

    menu, pos = take_name(pos)
    cls, pos = take_name(pos)
    caption, pos = cstr(data, pos)
    font = None
    if style & 0x40:
        points = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        face, pos = cstr(data, pos)
        font = (points, face)
        style &= ~0x40
    lines = [f"{res.res_id} DIALOG {mem_flags(res.flags)} {x}, {y}, {cx}, {cy}",
             f"STYLE 0x{style:08X}"]
    if caption:
        lines.append(f"CAPTION {rc_quote(caption)}")
    if isinstance(cls, str):
        lines.append(f"CLASS {rc_quote(cls)}")
    if isinstance(menu, int):
        lines.append(f"MENU {menu}")
    elif isinstance(menu, str):
        lines.append(f"MENU {rc_quote(menu)}")
    if font:
        lines.append(f"FONT {font[0]}, {rc_quote(font[1])}")
    lines.append("BEGIN")
    for _ in range(count):
        ix, iy, icx, icy, ident, ctl_style = struct.unpack_from("<hhhhHI", data, pos)
        pos += 14
        if ident >= 0x8000:
            ident -= 0x10000
        if data[pos] & 0x80:
            cls_name = CTL_CLASS.get(data[pos], f"#{data[pos]}")
            pos += 1
        else:
            cls_name, pos = cstr(data, pos)
        if data[pos] == 0xFF:
            title = str(struct.unpack_from("<H", data, pos + 1)[0])
            pos += 3
        else:
            text, pos = cstr(data, pos)
            title = rc_quote(text)
        extra = data[pos]
        pos += 1 + extra
        # CONTROL defaults to WS_CHILD|WS_VISIBLE; clear the latter when the
        # original template omitted it (BRC has no flag to disable the default).
        style_txt = f"0x{ctl_style:08X}"
        if not ctl_style & 0x10000000:
            style_txt += " | NOT WS_VISIBLE"
        line = (f"    CONTROL {title}, {ident}, {rc_quote(cls_name)}, "
                f"{style_txt}, {ix}, {iy}, {icx}, {icy}")
        if extra:
            blob = data[pos - extra:pos].hex()
            line += f"  /* {extra} creation bytes: {blob} */"
        lines.append(line)
    lines.append("END")
    return lines


def string_lines(res: Resource) -> list[str]:
    data = payload(res)
    lines = ["STRINGTABLE", "BEGIN"]
    pos = 0
    base = (res.res_id - 1) * 16
    for index in range(16):
        length = data[pos]
        pos += 1
        text = data[pos:pos + length].decode("latin-1")
        pos += length
        if text:
            lines.append(f"    {base + index}, {rc_quote(text)}")
    lines.append("END")
    return lines


def accelerator_lines(res: Resource) -> list[str]:
    data = payload(res)
    lines = [f"{res.res_id} ACCELERATORS", "BEGIN"]
    pos = 0
    while pos < len(data):
        flags, event, command = struct.unpack_from("<BHH", data, pos)
        pos += 5
        if flags & 0x01 and 0x20 <= event <= 0x7E:
            key = rc_quote(chr(event))
        elif not flags & 0x01 and 32 <= event <= 126:
            key = rc_quote(chr(event))
        else:
            key = str(event)
        opts = ["VIRTKEY" if flags & 0x01 else "ASCII"]
        if flags & 0x04:
            opts.append("SHIFT")
        if flags & 0x08:
            opts.append("CONTROL")
        if flags & 0x10:
            opts.append("ALT")
        if flags & 0x02:
            opts.append("NOINVERT")
        extra = (", " + ", ".join(opts)) if opts else ""
        lines.append(f"    {key}, {command}{extra}")
        if flags & 0x80:
            break
    lines.append("END")
    return lines


def version_lines(res: Resource) -> list[str]:
    data = payload(res)

    def block(start: int) -> tuple[str, bytes, list]:
        length, value_len = struct.unpack_from("<HH", data, start)
        key, pos = cstr(data, start + 4)
        pos = align4(pos)
        value = data[pos:pos + value_len]
        pos = align4(pos + value_len)
        end = start + length
        children = []
        while True:
            pos = align4(pos)
            if pos + 4 > end:
                break
            child_len = struct.unpack_from("<H", data, pos)[0]
            if child_len < 4 or pos + child_len > end:
                break
            children.append(block(pos))
            pos += child_len
        return key, value, children

    def text_of(value: bytes) -> str | None:
        # Keep a trailing NUL: Microsoft-style VERSION strings include it in
        # wValueLength, and BRC only emits that form when the RC has ``\0``.
        if value.endswith(b"\0"):
            raw, suffix = value[:-1], "\0"
        else:
            raw, suffix = value, ""
        if any(byte < 32 or byte > 126 for byte in raw):
            return None
        return raw.decode("latin-1") + suffix

    def emit(node, indent: str) -> list[str]:
        key, value, children = node
        if children:
            lines = [f"{indent}BLOCK {rc_quote(key)}", f"{indent}BEGIN"]
            for child in children:
                lines.extend(emit(child, indent + "    "))
            lines.append(f"{indent}END")
            return lines
        text = text_of(value)
        if text is None:
            words = [f"0x{word:X}" for word, in struct.iter_unpack("<H", value)]
            return [f"{indent}VALUE {rc_quote(key)}, " + ", ".join(words)]
        return [f"{indent}VALUE {rc_quote(key)}, {rc_quote(text)}"]

    key, value, children = block(0)
    if key != "VS_VERSION_INFO" or len(value) < 52:
        raise ValueError("not a version resource")
    sig, _struc, file_ms, file_ls, prod_ms, prod_ls, mask, flags, os, ftype = \
        struct.unpack_from("<IIIIIIIIII", value, 0)
    if sig != 0xFEEF04BD:
        raise ValueError("bad version signature")

    def quad(ms: int, ls: int) -> str:
        return f"{ms >> 16}, {ms & 0xFFFF}, {ls >> 16}, {ls & 0xFFFF}"

    lines = [f"{res.res_id} VERSIONINFO",
             f"FILEVERSION {quad(file_ms, file_ls)}",
             f"PRODUCTVERSION {quad(prod_ms, prod_ls)}",
             f"FILEFLAGSMASK 0x{mask:X}",
             f"FILEFLAGS 0x{flags:X}",
             f"FILEOS 0x{os:X}",
             f"FILETYPE 0x{ftype:X}",
             "BEGIN"]
    for child in children:
        lines.extend(emit(child, "    "))
    lines.append("END")
    return lines


def icon_file(group: Resource, images: dict[int, bytes], cursor: bool) -> bytes:
    data = payload(group)
    count = struct.unpack_from("<H", data, 4)[0]
    header = bytearray(struct.pack("<HHH", 0, 2 if cursor else 1, count))
    blobs = []
    offset = 6 + count * 16
    for index in range(count):
        width, height, colours, _reserved, planes, bits, _nbytes, res_id = \
            struct.unpack_from("<BBBBHHIH", data, 6 + index * 14)
        blob = images[res_id]
        if cursor:
            planes, bits = struct.unpack_from("<HH", blob, 0)
            blob = blob[4:]
        header += struct.pack("<BBBBHHII", width, height, colours, 0,
                              planes, bits, len(blob), offset)
        blobs.append(blob)
        offset += len(blob)
    return bytes(header) + b"".join(blobs)


def write_module(module: str, resources: list[Resource], out: Path) -> None:
    by_id = {}
    for res in resources:
        by_id.setdefault(res.kind, {})[res.res_id] = payload(res)
    lines = [
        "/* Generated by tools/extract_res.py from your copy of the original",
        "   binaries. These resources are copyrighted; do not commit them.",
        f"   Source module: {module} */",
        "",
    ]
    for res in resources:
        if res.kind == "DIALOG":
            lines.extend(dialog_lines(res))
            lines.append("")
        elif res.kind == "STRING":
            lines.extend(string_lines(res))
            lines.append("")
        elif res.kind == "ACCELERATOR":
            lines.extend(accelerator_lines(res))
            lines.append("")
        elif res.kind == "VERSION":
            lines.extend(version_lines(res))
            lines.append("")
    for res in resources:
        if res.kind == "GROUP_ICON":
            blob = icon_file(res, by_id["ICON"], cursor=False)
            rel = Path(module) / "ICON" / f"{res.res_id}.ico"
            write_bin(out / rel, blob)
            lines.append(f"{res.res_id} ICON {mem_flags(res.flags)} {rc_path(rel)}")
        elif res.kind == "GROUP_CURSOR":
            blob = icon_file(res, by_id["CURSOR"], cursor=True)
            rel = Path(module) / "CURSOR" / f"{res.res_id}.cur"
            write_bin(out / rel, blob)
            lines.append(f"{res.res_id} CURSOR {mem_flags(res.flags)} {rc_path(rel)}")
    lines.append("")
    for res in resources:
        ext = EXT.get(res.kind)
        if ext is None:
            continue
        rel = Path(module) / res.kind / f"{res.res_id}.{ext}"
        write_bin(out / rel, payload(res))
        lines.append(f"{res.res_id} {res.kind} {mem_flags(res.flags)} {rc_path(rel)}")
    lines.append("")
    text = "\r\n".join(lines)
    (out / f"{Path(module).stem}.RC").write_bytes(text.encode("latin-1"))


def rc_path(path: Path) -> str:
    return rc_quote(str(path).replace("/", "\\"))


def write_bin(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def last_resource_end(data: bytes) -> int:
    """File offset just past the last resource payload (alignment included)."""
    ne = struct.unpack_from("<H", data, 0x3C)[0]
    res_table = ne + struct.unpack_from("<H", data, ne + 0x24)[0]
    shift = struct.unpack_from("<H", data, res_table)[0]
    p = res_table + 2
    last = 0
    while True:
        rtype = struct.unpack_from("<H", data, p)[0]
        if rtype == 0:
            break
        count = struct.unpack_from("<H", data, p + 2)[0]
        p += 8
        for _ in range(count):
            off, length = struct.unpack_from("<HH", data, p)[:2]
            p += 12
            end = (off << shift) + (length << shift)
            if end > last:
                last = end
    return last


def write_codeview_overlay(source: Path, module: str, out: Path) -> Path | None:
    """Save a trailing NB02 CodeView blob next to the module's resources.

    Only AACOLOR.DLL has one; others return None. build.py appends this file
    after binding so the rebuilt DLL hashes without touching orig/ again.
    """
    raw = source.read_bytes()
    if len(raw) < 8 or raw[-8:-4] != b"NB02":
        return None
    overlay = raw[last_resource_end(raw):]
    if not overlay:
        return None
    rel = Path(module) / "CODEVIEW.bin"
    write_bin(out / rel, overlay)
    return rel


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument(
        "binaries",
        nargs="*",
        type=Path,
        help="NE files to read (default: AARCADE.EXE, AACOLOR.DLL, AABW.DLL, "
             "METER.DLL under orig/)",
    )
    ap.add_argument(
        "--out",
        type=Path,
        default=ROOT / "build" / "res",
        help="directory for the .RC files and their data (default: build/res)",
    )
    args = ap.parse_args()
    binaries = args.binaries
    if not binaries:
        binaries = [ROOT / "orig" / name for name in DEFAULT_BINS]
        missing = [path for path in binaries if not path.exists()]
        if missing:
            names = ", ".join(path.name for path in missing)
            raise SystemExit(f"missing {names}; run tools/setup_orig.py with your own copy of the game")
    counts = []
    for path in binaries:
        resources = parse_resources(path)
        for res in resources:
            try:
                logical_size(res)
            except (ValueError, IndexError) as exc:
                raise SystemExit(f"{path.name} {res.kind} {res.res_id}: {exc}") from exc
        write_module(path.stem, resources, args.out)
        note = f"{path.stem}.RC ({len(resources)} resources)"
        cv = write_codeview_overlay(path, path.stem, args.out)
        if cv is not None:
            note += f" + {cv}"
        counts.append(note)
    print(f"wrote {args.out}/ " + ", ".join(counts))
    return 0


if __name__ == "__main__":
    sys.exit(main())
