# Alien Arcade decompilation

A matching decompilation of *Alien Arcade* (Robert Cavey and Sean Callahan,
1994, KidSoft), a 16-bit Windows 3.x game. The C in `src/` is shaped so
Borland C++ 3.1 rebuilds the original `AARCADE.EXE` byte-for-byte. A separate
SDL3 reimplementation lives under `port/`.

Common decomp tools only understand 32-bit PE files, so this project has its
own NE (New Executable) toolchain in `tools/`.

## Status

Matching for `AARCADE.EXE`, `AACOLOR.DLL`, and `AABW.DLL` is complete.
`docs/progress.json` reports `linked: true` and every annotated function as
`exact`. `tools/verify.py` (with `--dlls`) confirms the rebuilt files match
the hashes in `orig/SHA256SUMS`. `METER.DLL` shipped with the game is out of
scope (the game does not load it).

A Macintosh release of *Alien Arcade* also exists. Looking into that build
(symbols, shared code with the Windows version, assets) is a future todo;
this repository covers only the Windows 3.x NE binaries for now.

## Legal

The MIT [`LICENSE`](LICENSE) covers this repository’s source, tools,
documentation, and `port/` code only. The original game binaries, installer
packs, and assets remain copyrighted and are **not** redistributed here—only
SHA-256 hashes in `orig/SHA256SUMS`. See [`NOTICE`](NOTICE) for the original
game and for Wine-derived Win16 ordinal tables under `tools/winapi16/`.

Do not publish `orig/`, `bin/`, extracted `build/res/`, or a rebuilt
`AARCADE.EXE` with resources as if they were free redistributables.

## Layout

| Path | Contents |
|---|---|
| `bin/`, `AlienArcade.bin/.cue` | Original install files (not committed). |
| `orig/` | Expanded originals; only `SHA256SUMS` is committed. |
| `src/` | Matching source with `// FUNCTION:` / `// GLOBAL:` annotations. |
| `docs/progress.json` | Match inventory written by `necmp.py` for ByteMatch. |
| `docs/MATCHING.md` | Borland codegen notes for keeping a byte match. |
| `docs/GLOSSARY.md` | Game vocabulary used for function and global names. |
| `port/` | SDL3 reimplementation (does not compile `src/`). |
| `src/aarcade.def` | Module definition for the original NE header and exports. |
| `tools/` | NE parser, explorer, comparator, build and Ghidra sync. |
| `build/` | Rebuilt `AARCADE.EXE` and map (not committed). |
| `bin/BCPP31/` | Borland C++ 3.1 (not committed). |
| `bin/ghidra/` | GhidraMCP plugin and bridge (not committed). |
| `ghidra/` | Ghidra project with `AARCADE.EXE` imported (not committed). |

## Prerequisites

**Toolchain (exploration / compare / report):**

- Python 3 and a venv with `requirements.txt` (Capstone).

**Original game (required for rebuild, verify, resources, and the port UI):**

- Your own copy of the game. Place these files in `bin/`:
  `aarcade.ex_`, `aabw.dl_`, `aacolor.dl_`, and `meter.dll`
  (as on the install media; the `.ex_` / `.dl_` files are SZDD-compressed).

**Matching rebuild (optional; Linux is the supported path):**

- Borland C++ 3.1 under `bin/BCPP31/` (not freely redistributable; you need
  your own copy).
- [DOSBox-X](https://dosbox-x.com/) Flatpak: `com.dosbox_x.DOSBox-X`.
- The rebuilt EXE is for comparison only; it is not a redistributable game
  build.
- PRs for building on other paths are very much welcome!

## Setup

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python tools/setup_orig.py      # expands bin/ packs into orig/, checks hashes
```

## What we know about the original

| Fact | Evidence |
|---|---|
| Borland C++ 3.0/3.1 | "Borland C++ - Copyright 1991 Borland Intl." in the EXE; linker version 5.10 |
| Medium memory model (`-mm`) | Far code (`retf`), but RTL calls like `memset`/`time` get 16-bit data pointers |
| 286 code (`-2`) | `push imm`, `leave` |
| Pascal calling convention by default (`-p`) | Game functions pop their own arguments; RTL calls are cleaned up by the caller |
| Smart exports only (`-WE`) | Full `mov ax,ds; nop; inc bp; push bp...` prologue only on exported functions |
| No jump optimization | Jumps to jumps, `jmp` to the next instruction |
| One source file per code segment, no segment packing (`tlink /P-`) | 35 code segments; segment 1 is Borland's startup (`c0w`) and runtime library |

Segment 2 is the Windows entry (`WinMain` / `MainWndProc`); later segments
cover init, the sprite engine, Mac-ported helpers, and packed picture
decoding. `tools/nedump.py info` and [`docs/MATCHING.md`](docs/MATCHING.md)
have the detail. Read MATCHING.md before “tidying” odd-looking source; most
oddities exist so Borland emits the original bytes.

## Workflow

1. **Explore** the original:
   ```sh
   .venv/bin/python tools/nedump.py info
   .venv/bin/python tools/nedump.py funcs 3        # functions in segment 3
   .venv/bin/python tools/nedump.py disasm 3:00D9  # one function, symbolized
   .venv/bin/python tools/nedump.py xrefs 2:0179   # who references an address
   .venv/bin/python tools/nedump.py xrefs 24:38B8 --size 0x100   # code using a global
   ```
   Import ordinals are shown by name (`USER.RegisterClass`), annotated
   functions and globals by their source names.

2. **Write C** in `src/`, one file per original segment, annotated with the
   original address (segment:offset, hex):
   ```c
   // FUNCTION: AARCADE 0003:00D9
   BOOL FAR PASCAL InitApplication(HINSTANCE hInstance)

   // GLOBAL: AARCADE 0024:237C
   HWND g_hwndMain;
   ```
   A third token overrides the linker symbol when it cannot be guessed
   (`// FUNCTION: AARCADE 0003:00D9 @Foo$qv`). Borland RTL from `CWM.LIB` is
   listed in `src/rtl.h` as `// LIBRARY:` / `// GLOBAL:` so the comparator can
   name those calls. During matching, unfinished callees used `FUN_SSSS_OOOO`
   placeholders; those are gone from `src/` now that the EXE matches.

3. **Build**:
   ```sh
   .venv/bin/python tools/build.py
   ```
   Runs Borland C++ 3.1 from `bin/BCPP31` in the DOSBox-X Flatpak (no window)
   and prints compiler/linker output. The result is for comparison only.

4. **Compare**:
   ```sh
   .venv/bin/python tools/necmp.py                    # table + progress
   .venv/bin/python tools/necmp.py --diff InitApplication
   .venv/bin/python tools/necmp.py --selftest         # original vs. itself; checks the tooling
   ```
   Status per function:
   - `exact`: identical bytes, apart from relocated fields and calls whose
     target moved but resolves to the same function.
   - `equivalent`: identical instructions, but some data offsets differ
     because the data segment layout does not match yet.
   - `differs`: see `--diff`; tweak the source and rebuild.
   - `not in map`: the function's symbol is missing from the map file
     (`static` functions are not listed; keep functions non-static while matching).

   After the code summary, a data line gives the share of the data segment
   that `GLOBAL` annotations name, the bytes in string-literal pools, and how
   many bytes are identical to the original. `--data` lists the items no
   annotation names. Item sizes come from the compiler's assembly listings,
   so a global's size is exact. Library data has no listing; its sizes run
   to the next public and are marked as such.

   A full run also writes `docs/progress.json`. Commit that file when match
   status changes so CI can publish a ByteMatch report without Borland or
   the original EXE (`python tools/report.py -o build/report.json`).

5. **Verify the whole file**:
   ```sh
   .venv/bin/python tools/verify.py              # clean build, then compare
   .venv/bin/python tools/verify.py --no-build   # compare the current build
   ```
   Hashes `build/AARCADE.EXE` against `orig/SHA256SUMS` and exits 0 only when
   every byte matches. Otherwise it lists the differences by NE structure:
   DOS stub, header fields, tables, segment bytes and relocations, and
   resources. A matching build uses Borland C++ 3.1 with `STUB
   'D:\\BIN\\WINSTUB.EXE'`, and Borland's `BRC -30` to compile and bind
   resources (the Windows 3.0 nametable form: `WAVE` as `#32513` via type
   `#15`). Controls that omit `WS_VISIBLE` in the original get
   `| NOT WS_VISIBLE` in the generated `.RC` so BRC does not force them
   visible. `VERSIONINFO` string values keep their trailing NULs the same
   way. `AACOLOR.DLL` / `AABW.DLL` are rebuilt bit-for-bit as well (LibMain +
   Borland C0D/CWC; see `docs/MATCHING.md`); pass `--dlls` to make
   `verify.py` require their hashes too. Other binders are selectable with
   `--rc-compile` / `--rc-bind` / `--rc-tree`.

## Progress report

[ByteMatch](https://bytematch.dev) charts matching progress from an objdiff
report. This project cannot run objdiff (16-bit OMF / NE), so `necmp.py`
writes a committed inventory and `tools/report.py` turns it into the same
JSON. GitHub Actions uploads it as the `v1.0_report` artifact on every push
to the default branch.

The inventory covers `AARCADE.EXE` and the picture DLLs `AACOLOR` / `AABW`
(under a `modules` map). Word-alignment padding between modules is recorded
in each module's `gaps` list and left out of the code total. String-literal
pools and linker data padding count as matched data when their bytes are
identical; runtime-library data is annotated with `// GLOBAL:` in
`src/rtl.h` and `src/datalib/rtl.h`. ByteMatch's "fully linked" measures
follow the top-level `linked` flag: true only when all three rebuilt files
are identical to the originals (`verify.py --dlls`). AACOLOR's trailing
CodeView overlay is debug data appended after the build and is not measured.

```sh
.venv/bin/python tools/necmp.py              # refresh docs/progress.json
python tools/report.py -o build/report.json  # objdiff report (stdlib only)
```

## Build details

`tools/build.py` mounts the project as `C:` and `bin/BCPP31` as `D:`, then
writes and runs `build/BUILD.BAT`:

```bat
bcc -c -mm -2 -p -WE -w-par C:\SRC\MAIN.C
bcc -S -nLST -mm -2 -p -WE -w-par C:\SRC\MAIN.C
tlink /Twe /m /s /c /n /P- D:\LIB\C0WM MAIN ..., AARCADE, AARCADE, D:\LIB\IMPORT D:\LIB\MATHWM D:\LIB\CWM, AARCADE.DEF
```

The second `bcc` writes an assembly listing to `build/LST/` for the data
coverage figure, and `/s` adds each module's contribution to the map. Neither
changes the executable.

Borland C++ 3.1 is not freely redistributable; you need your own copy.

## Ghidra

Ghidra (Flatpak, 12.1.3) is useful for reading larger functions. A local
project under `ghidra/` (not committed) can import and analyze `AARCADE.EXE`.

Ghidra loads NE segment *n* at segment `0x1000 + (n - 1) * 8` with unchanged
offsets: annotation `0002:0000` is Ghidra address `1008:0000`, and the data
segment `0024` is `1118`.

```sh
.venv/bin/python tools/ghidra_sync.py   # push annotation names into the project (Ghidra must be closed)
```

Optional: GhidraMCP can expose the running CodeBrowser to an editor MCP
client. Install the plugin from `bin/ghidra/`, open the program, enable
GhidraMCPPlugin (listens on `127.0.0.1:8080`), and use `.cursor/mcp.json` if
you work in Cursor.

## Resources

Pictures, sounds and dialogs are copyrighted, so they are not in this
repository. With your own copy of the game expanded into `orig/`:

```sh
.venv/bin/python tools/extract_res.py
```

That writes `build/res/`, which git ignores. `AARCADE.RC` is the executable
(dialogs, strings, accelerators, the icon and the WAVE sounds). The pictures
are in `AACOLOR.RC` and `AABW.RC`, the two libraries the game loads by name.
Replace a `.wav`, `.pic`, `.pct` or `.mac` and build again to swap an asset.

```sh
.venv/bin/python tools/build.py
```

The build compiles those scripts and binds them. `build/AARCADE.EXE` receives
the dialogs and sounds. `build/AACOLOR.DLL` and `build/AABW.DLL` are picture
libraries written next to the executable (where the game looks), linked from
`src/datalib/libmain.c` plus Borland startup/RTL so they match the originals
byte-for-byte; AACOLOR also gets its trailing CodeView overlay from
`build/res/AACOLOR/CODEVIEW.bin` (extracted alongside the pictures). Without
`build/res/` the executable is linked with no resources,
which is all `necmp.py` needs. `METER.RC` is extracted too, but the game does
not load that library and its code is not decompiled.

## Port

`port/` is a separate SDL3 program. Matching sources stay in `src/` as the
specification for behavior; `tools/build.py` does not compile anything under
`port/`. All six rounds are playable when assets are present.

When changing the port, treat `src/` as law: find the matching function first,
prefer a close transcription, and mark clear correspondences with
`/* orig: Name (SSSS:OOOO) */` (see `port/src/between.c` for the pattern).
Those comments are cross-refs only—not the matching `// FUNCTION:` annotations
used by `necmp.py`.

Needs CMake, FreeType, and an SDL3 install (or CMake will fetch SDL 3.4.18).
Extract resources first (`tools/extract_res.py`) so `build/res/` exists for
the full UI.

```sh
cmake -S port -B port/build
cmake --build port/build
./port/build/alienarcade
```
