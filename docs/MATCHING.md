# Matching notes

What we have learned about getting Borland C++ 3.1 to produce the same bytes
as the original `AARCADE.EXE`. Every rule here was confirmed against the
original with `tools/necmp.py`; the examples are taken from this game.

Build flags are in the README (`bcc -c -mm -2 -p -WE -w-par`, no `-O`). Code
is compiled without optimization, so the shape of the C source shows up
directly in the machine code: two sources that do the same thing usually
compile differently, and only one of them is right.

When a function almost matches, run `necmp.py --diff NAME` and look at the
first real difference; labels that are shifted because of an earlier size
difference are noise.

## Source layout

### One file per code segment, functions in address order

TLINK is run with `/P-`, so every `.c` file becomes its own code segment, in
link order. Within a file, functions are emitted in source order, so they
must be written in the order of their original addresses.

Order matters for code too. A far call to a function in the same file is
compiled as `push cs; call near`, and Borland pads it with a `nop` only when
the callee comes *later* in the file:

```
0314  nop                 ; StartAnimTimer (02E8) calling StopAnimTimer (035D)
0315  push cs
0316  call 0005:035D
...
041C  push 0              ; RestoreAnimTimer (040B) calling StartAnimTimer (02E8)
041E  push cs
041F  call 0005:02E8      ; no nop: the callee is already known
```

A function that is called this way must be in the same file as its caller.

### Exported functions

Functions listed in `EXPORTS` get the full `-WE` prologue
(`mov ax,ds; nop; inc bp; push bp; mov bp,sp; push ds; mov ds,ax`). The saved
`DS` occupies `[bp-2]`, so their locals start at `[bp-4]`. Declare them
`CALLBACK _export` (or `FAR PASCAL _export`).

## Locals and registers

### Declaration order decides stack slots

The first local declared gets the highest address. In `InitGame`:

```c
int nPlanes, nBits;            /* nPlanes at [bp-2], nBits at [bp-4] */
nBits = GetDeviceCaps(hdc, BITSPIXEL);
nPlanes = GetDeviceCaps(hdc, PLANES);
```

```
mov word ptr [bp - 4], ax     ; nBits
...
mov word ptr [bp - 2], ax     ; nPlanes
```

Swapping the declaration swaps the two slots and the function no longer
matches.

### Block-scoped locals share slots

Locals declared in sibling `{ }` blocks reuse the same stack slots. When two
unrelated variables live at the same `[bp-n]`, the original declared them in
separate blocks. `InitGame` has `nPlanes`/`nBits` in the `else` of the colour
depth test and `dwFree`/`dwCompact` in a later bare block, so both pairs use
`[bp-2]..[bp-8]`. `ModalDlgProc` and `MainWndProc` do the same with their
per-message locals.

The frame size (`sub sp,N`) is the size of the largest set of blocks that are
live at the same time, which helps to work out the block structure.

Large locals (arrays, and structs bigger than a `RECT`) go below the small
ones, even below small locals of nested blocks and the `switch` temporary. In
the `WM_PAINT` block of `ParamTextProc`, `hpal` (in `if (g_fColor) { }`) and
`fmt` (in a bare block) share `[bp-0x18]`, directly under the block's
word-sized locals and above its `char sz[256]` and `PAINTSTRUCT ps`. In
`TrackUserButton` the switch temporary sits at `[bp-0x12]` above the
function-level `MSG msg`. Two unrelated words in one slot next to large
buffers therefore still mean sibling blocks.

### Register variables

Borland keeps up to two word-sized locals or parameters in `SI` and `DI`
without the `register` keyword. Parameters that are used often are copied
into them in the prologue (`mov si, word ptr [bp + 8]`). So far the choice
has always followed from the source as written; we have not had to force it.

If the wrong variable gets a register, look at the use counts first: one
local reused for two purposes adds up both sets of uses. In `ParamTextProc`
one `WORD w` for both the palette and the text alignment took `DI` from
`hdc`; two variables in sibling blocks fixed it. The `register` keyword
puts a local ahead of the parameters (an explicit `register HDC hdc` took
`SI` from `hwnd`, and marking both swapped them), so it is not a way to
reproduce the original allocation. The same goes for a pointer used in two
branches: one function-level `ptbl` in `SeekPicRes` took `DI` from the
`index` parameter, while a separate `ptbl` in each branch of the depth test
stays on the stack (both at `[bp-0xa]`).

How a value is computed also counts as uses. In `AddR5Count`,
`v = g_anR5Count[i] + n` goes through `AX` (`mov ax,[bx+g]; add ax,[bp+8];
mov si,ax`), and with `HDC hdc` declared first `hdc` took `DI` and the
parameter `i` took `SI`. `int v; HDC hdc;` with `v = g_anR5Count[i];
v += n;` gives the original's `mov si,[bx+g]; add si,[bp+8]`, the index in
`DI` and `hdc` on the stack, without `register`.

A `char *` used only at the end of a function still owns `SI` for the whole
function, so an `int` that needs `SI` earlier stays on the stack. Writing
both ranges as `_SI`, and the other live word as `register int`, splits
them: `_SI = 0x100; _SI -= g_r4Img[66].rc.right` is `mov si,0x100; sub si,
[g]` and `register int y; y = 0` is `xor di,di` (`SpawnRound4Fungus`). The
later walk `_SI += *(char *)(_SI + 1)` is `mov al,[si+1]; cbw; add si,ax`
(`8a 44 01 / 98 / 03 f0`). A plain `char *ip; ip += ip[1]` takes `SI` away
from the `int`. Two pointers plus a counter work the same way. In
`CloseSfx` a plain `int *p` took `SI` and `HGLOBAL *ph` took `DI`, leaving
the count on the stack. `_SI = (unsigned)g_anSndId` with
`id = *(int *)_SI; *(int *)_SI = 0; _SI += 2` is `mov ax,[si]; mov [si],0;
add si,2`, and `register int n` with `while (n--)` is `mov di,0x10` and
`mov ax,di; dec di`, so `ph` stays at `[bp-2]`.

`register` does decide between a local and a parameter of equal claim: in
`MoveRound5Player` the original keeps the local `x` in `SI` and the
`SPRITE *s` parameter in `DI`. Without the keyword they swap; `register int
x;` gives the original order.

Borland also keeps short-lived locals in `CX`/`DX` when no call intervenes.
In `TickRound5` the search loop's pointer and counter go to the stack as
function-level locals (`sub sp,4`), but declared inside the `if` block that
holds the loop they live in registers, the first-declared one in `DX`:

```c
int n;                        /* DX */
SPRITE **pp;                  /* CX */

pp = &g_apR5Stack[1][0];      /* mov cx, offset G_APR5STACK+6 */
n = 21;                       /* mov dx, 0x15 */
```

A pointer in `CX` has to be copied to `BX` before the load. `p = *pp++` loads
first and then adds (`mov bx,cx; mov di,[bx]; add cx,2`), and so do `p = *pp;
pp++` and `pp += 1`. The original increments first (`mov bx,cx; add cx,2;
mov di,[bx]`), which is `_BX = (unsigned)pp++; p = *(SPRITE **)_BX;`.

## Control flow

### Identical branch tails are merged

Even without `-O`, Borland merges code that is identical at the end of two
branches and jumps into the shared copy. This is how to tell `if`/`else`
from `?:`:

```c
if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)     /* CreateGamePalette */
	nColors = GetDeviceCaps(hdc, SIZEPALETTE);
else
	nColors = GetDeviceCaps(hdc, NUMCOLORS);
```

```
push di
push 0x68
jmp L00C1                     ; then-branch jumps into the else-branch's tail
push di
push 0x18
L00C1: lcall far GDI.GetDeviceCaps
cdq                           ; the store is shared too
```

A ternary would evaluate both calls separately and only share the store
after them. Conversely, in `MsgHookProc` the two values meet in a single
`mov [bp-8],ax`, which is a ternary.

The same merging produces shared calls across statements: in `InitGame` the
four `ShowMessage(g_hwndMain, n, MB_ICONSTOP); return FALSE;` blocks share
one `push 0x10; call ShowMessage; jmp` sequence.

A then-branch that is a block with its own locals keeps its jump to the end
of the `if` after the merge, so an unreachable `jmp` follows the jump into
the shared tail. In `TickScene` the `RECT rc` for the redraw call has to be
declared in the enclosing loop body, not in the then-block:

```
push 1
jmp L03E2                     ; then-branch: jump into the shared call
push di                       ; else-branch (no extra jmp before it)
...
L03E2: lcall far 000B:0000
```

The same applies to a `case` body that is a block with locals: `case
WM_PAINT: { ... } break;` gives `jmp end; jmp end`, while `{ ...; break; }`
gives one `jmp` (`PicItemProc` has the first form, `ParamTextProc` the
second). An inner `break` and the case's own `break` are the same pair of
jumps, with the branches above targeting the second (`MoveSprite` opcode 9).
An empty `else` makes the first of those jumps target the next instruction
(`eb 00`) instead of the exit. An empty `else if` retests and jumps to the
next instruction with `jne +0` (`75 00`) onto the exit `jmp`
(`HitRound1Snake`). A `goto` whose label is the next statement is the same
`eb 00`, and a conditional that ends just before it lands on that jump
rather than on the block's real exit. In `AdvanceSpeaker` the note test
is `je stop; eb 00` (`goto play; play:`), and both ways out of the wait
— `or si,si; jne` and the loop's `jne` — target a second `eb 00`
(`goto join; join:`) which then jumps over `StopSpeaker`. An empty
`else` on that wait puts only the loop exit on the `eb 00`; the
`or si,si; jne` skips it. The `fTimer = 1` is inside the `do`, because
the backward jump targets that `mov`: a rest stores 0, and the following
`-1` must still see 1. Ending the block on `s->fDirty = 1` and
writing `fCont = 1; break;` after the brace joins the shared fDirty tail and
then the shared fCont tail (opcodes 5–7).

A `for` loop whose initialiser is the same as its increment shares that
code too: the loop starts with a `jmp` to the increment instead of its own
copy. `DrawScene` continues down the sprite list with
`for (p = p->pPrev; p; p = p->pPrev)`; `while ((p = p->pPrev) != 0)` goes
through `AX` instead:

An early `return 0` written in front of a long body is emitted there
(`xor ax,ax; jmp` the epilogue). `if (nLen > 0) { ... return h; } return 0;`
puts that `return 0` after `return h`, and the test is `jg body; jmp
return0` because the body is too long for a conditional (`ReadScoreList`).
`return 0` then jumps back to the `jmp` that `return h` already emitted.
`if (n == 0) return;` in front of the rest of a short function is
`jne body; jmp epilogue`; wrapping that rest in `if (n) { ... }` is
`or cx,cx; je epilogue` (`InsertRank`). The same `je` reaches the epilogue
when the body is long but still inside 127 bytes: `if (a != 0 && b != 0)
{ ... }` in `ThrowRound1Pie` is `cmp [a],0; je epilogue`. A bare
`if (s == 0) return;` in front of a 117-byte body is still the long
`jne; jmp` (`SpawnRound2Nerdoid`); `if (s != 0) { ... }` is the short `je`.
The same choice decides a test that sits just inside the short limit.
`if (x >= 7) return;` in front of the rest is `jl body; jmp epilogue`
even when a direct `jge` would fit, and once that jump is long the later
returns in the same span go long too. Writing the rest as the body,
`if (x < 7 || y < 7) if (p != 0) if (w < n) { ... }`, is the direct
`jl body; jge epilogue` (`HitRound3Klutz`, displacement `7d 7d`):

```
cmp word ptr [si], 7
jl L00B4
cmp word ptr [di], 7
jge L0131                     ; both >= 7: epilogue
L00B4: ...                    ; xor; je L0131, then jge L0131
```

Two constant returns merge the same way: `if (n == 0) return 1; return 0;`
sends the first `jmp` straight to the epilogue and the second jumps back to
it. `return n == 0 ? 1 : 0;` keeps both jumps, the first landing on the
second (`DllPresent`):

```
mov ax, 1
jmp L0080
xor ax, ax
L0080: jmp L0082                 ; eb 00
```

`if (a <= 0 && b <= 0 && c <= 0 && (d != 0 || e != 0)) return 1; else
return 0;` sends every failed test at the else's `xor ax,ax`, which jumps
back to the `jmp` after `mov ax,1`. The `else` adds the second `jmp`
(`TickRound1`):

```
jg L0078                          ; a, b and c each
jne L0071
je L0078
L0071: mov ax, 1
jmp L007C
jmp L007C
L0078: xor ax, ax
jmp L0074                         ; back to the first jmp
```

`if (a == 0 && b == 0 || c != 0) return 1; else return 0;` puts `mov ax,1`
after the third test, and the else's `xor ax,ax` jumps back to the first
`jmp` (`TickRound2`):

```
cmp [a], 0
jne L0097
cmp [b], 0
je L009E
L0097: cmp [c], 0
je L00A5
L009E: mov ax, 1
jmp L00A9
jmp L00A9
L00A5: xor ax, ax
jmp L00A1
```

```
jmp L0240                     ; initialiser = jump to the increment
L01C7: ...                    ; body
L0240: mov si, word ptr [si + 0x92]
or si, si
je L024B
jmp L01C7
```

### Reading jump shapes

| Machine code | Source |
|---|---|
| `jcc else; ...; jmp end` | ordinary `if`/`else` |
| `je body; jmp end` in front of a short body | empty then-branch: `if (x) { } else { ... }` |
| `je body; jmp end` in front of a body over 127 bytes | ordinary `if`; the 286 has no long conditional jumps, so Borland inverts the test and adds a near `jmp` |
| two `jmp`s in a row after a `return` | `return x;` followed by an unreachable `break;`, or a then-branch ending in `return` followed by `else` |
| `jmp` to another `jmp` with the same target | Borland reuses an existing jump instead of emitting a new one; nothing to do in the source |
| `jmp` to the next instruction | the jump out of the last case or a `return` at the end of a function |
| `dec si; mov ax,si; cmp ax,0xffff; je out; jmp top` | `if (--c != -1) goto top;`, a 68k `dbra` loop ported with labels (`UnpackColumn1`) |

Unreachable statements give an "Unreachable code" warning. The original was
built with the same warnings; keep the statements.

Examples in this code:
- `MainWndProc`, `IDC_START`: `if (IsIconic(g_hwndMain)) { } else { ... }`.
- `return RunModalDialog(...);` ends with `eb 00` onto the epilogue
  (`RunNextDialog`). Dropping the `return` leaves that out and still returns
  the call's `AX` (`RunResetScores`); Borland warns that the function should
  return a value.
- `MainWndProc`, `WM_QUERYNEWPALETTE`: `return 0; break;`.
- `GetGamePalette`: `if (g_hPalette) return g_hPalette; else return ...;`
  gives `jmp exit; jmp exit`.
- `ModalDlgProc`: `if (lpfnHook) { ... return fResult; } else return FALSE;`.
- `CountIndStrings`: `if (!n) { } else n = (BYTE)sz[0];` gives
  `jne L0026; jmp L002E`.

### Combined conditions versus early returns

`if (a && b && (c || d)) { body }` and `if (a && b) { if (c && !d) return;
body }` look equivalent but compile differently: the second adds a
`je`/`jmp` pair. `StartAnimTimer` only matches with the combined condition.
`if (n != a && n != b) break;` is `je; je; jmp break` (`PlayWndProc`);
`if (n == a || n == b)` inverts the second compare.

### `switch`

- A few cases compile to a compare chain on a copy of the value in `AX`:
  `mov ax,[bp-0xa]; cmp ax,2; je ...; cmp ax,0x65; je ...; jmp default`.
  An `if (n == 2 || n == 101)` compares memory directly and does not match.
- More cases compile to an inline table in the code segment:
  `mov cx,N; mov bx,table; cmp ax,cs:[bx]; ... jmp cs:[bx+2N]`. The table
  sits right after the function. Ghidra does not recover these; read them
  from `nedump.py disasm`.
- A compare-chain switch copies its value to a temporary on the stack, which
  takes a slot of its own. A dense `switch (*ip)` covering every byte from 0
  to 15 does not: `mov al,[bx]; mov ah,0; mov bx,ax; cmp bx,0xf; jbe; jmp
  default; shl bx,1; jmp word ptr cs:[bx+table]`. The table is a list of near
  offsets placed immediately after the function (`MoveSprite`). Bodies still
  follow source order, so opcode 15's body sits before opcode 12's. An empty
  `default` that is last is `jmp` to the next instruction, and the
  out-of-range jump lands on that jmp.
- `switch (n - 1)` with cases 0 to 5 is the same dense table with the
  subtraction first: `mov bx,[n]; dec bx; cmp bx,5; ja default`
  (`OpenRound`). Out of range and every `break` land on whatever follows
  the switch.
- Case bodies are emitted in source order; the compare chain is sorted by
  value. `View1WndProc` puts the empty `WM_ERASEBKGND` case first, so its
  `jmp` to the shared exit sits immediately after the chain, then `WM_PAINT`
  and `WM_TIMER`. `break` out of every case and `return 1;` after the switch
  share one `xor dx,dx; mov ax,1`, which jumps to the `jmp` the default case
  already emitted. `return 1;` inside each case inlines that sequence in the
  first case instead.
- `if (n != 1) break; f(); break;` is `je f; jmp end; f; jmp end`
  (`PlayWndProc`). The `break` is the then-body, so it targets the switch
  exit past the `default`. An empty then jumps to the if's merge point
  instead, which is the next instruction and lands before the `default`.
- Tail merging keeps the first copy of a shared call and jumps back to it.
  An earlier case that has its own store jumps forward over a later case's
  store into one call only when it `goto`s a label on that later case
  (`PlayDlgProc`).
- `switch (n)` with cases -4, -3, -2, -1 subtracts the base:
  `mov bx,[bp+0xc]; sub bx,-4; cmp bx,3; ja default` (`MoveRound4Script`,
  `83 eb fc`). `switch (n + 4)` folds to `add bx,4`.
- Two `return 0` paths share one `xor ax,ax`. An unreachable `goto` between
  the inner return and the outer one gives each its own `xor`, and the
  compare jumps to the second: `xor ax,ax; jmp epilogue; jmp epilogue;
  xor ax,ax; jmp` (`TickRound4`, `33 c0 / eb 06 / eb 04 / 33 c0`). The
  source is `return cond ? 1 : 0; goto done;` then `return 0; done:;`.

## Expressions

- `__outportb__` and `__inportb__` from `dos.h` are inlined (`out dx,al` /
  `in al,dx`), not calls. The high byte of an `int` at `[bp-2]` is
  `[bp-1]`: `((unsigned char *)&nDiv)[1]` (`SetSpeakerPitch`).
- `a = b = 0;` stores `b` first, then `a`, through `AX` (`xor ax,ax` and
  two stores). A separate `b = 0; a = 0;` is `mov word ptr [r+n], 0` for
  each. `SetSpriteMove` needs the separate stores. A call result chained
  the same way stores `AX` into the first destination before the register
  copy: `s = g = f();` is `mov [g],ax; mov si,ax` (`AddRound1Player`).
- `if ((s = f()) != 0)` keeps the test on `AX` (`mov si,ax; or ax,ax`).
  `s = f(); if (s)` retests `SI`.
- `y = (i == 1) ? 0 : 6;` shares one store: `xor ax,ax; jmp; mov ax,6;
  mov [y],ax`. An if/else stores the immediate in each branch
  (`OpenView4`).
- `x = x | CS_DBLCLKS;` compiles to `mov ax,[x]; or ax,8; mov [x],ax`.
- A `long` used as an array index only uses its low word
  (`mov bx,[lo]; shl bx,2`), see `CreateGamePalette`.
- `(long)n << 16` stored in a long moves `n` to the high half and writes an
  immediate zero in the low half, with no `cwd`: `mov [high],ax; mov word ptr
  [low],0` (`MoveSprite`). Shifting a long by 16 does the same with that
  long's low word. Dividing the result by an int calls `LDIV@` with the
  sign-extended divisor pushed first, then `push` of the low word and `push 0`.
- `(int)((x + 0x8000L) >> 16) - i` held in `_CX` rounds a 16.16 value:
  `add dx,0x8000; adc ax,0; cwd; sub ax,[i]; mov cx,ax` (`MoveSprite`).
- `long * int` calls the far long-multiply helper `LXMUL@` with the left
  operand in `CX:BX`, so the operand order is visible: `nRate * 17L`.
- `PALETTERGB`/`RGB` from `windows.h` produce the odd `or ax,0` instructions;
  use the macros rather than writing the arithmetic out.
- Far `NULL` arguments are `push 0; push 0`. A handle widened through a
  near pointer gets `DS` instead of zero: `(LONG)(BYTE FAR *)(BYTE NEAR *)h`
  is `push ds; push h` (`ResultDlgProc`). `(LONG)(void FAR *)h` is
  `push 0; push h`, and `MAKELONG(h, _DS)` is an `or` of the two halves.
  A near array passed to a `FAR *` parameter is the same `push ds; push
  offset`, and `(int)` of that parameter reads only the offset
  (`mov ax,[bp+8]; mov [di],ax` in `AddNextSprite`).
- An `int` constant in -128..127 is always the short `push imm8` (`6a 7c`).
  A `push imm16` (`68 7c 00`) of such a value is a fixup: the operand is the
  offset of a DGROUP global, which the compiler cannot shorten because the
  linker fills it in. `OpenView1` passes `124` to `SetSpriteMove`, and
  `DS:0x7C` is the first of the `view.c` move scripts, so the source is
  `(int)g_cmdView1`. The other `OpenViewN`/`OpenLineView` ids are the same
  kind of pointer; they only matched before because they were above 127.
- A `char` tested with `!` is widened first (`mov al,[si+0x5c]; cbw; or ax,ax`);
  tested plainly it is compared in memory (`cmp byte ptr [si+0x5e],0`). In
  `TickScene`, `if (!s->fHold)` versus `if (s->fDead) continue;`. The same
  `!` on a stack array indexed by a stack `int` addresses through `BX`:
  `lea ax,[bp-6]; mov bx,[bp-2]; add bx,ax; mov al,[bx]; cbw; or ax,ax`
  (`while (!ab[nPick])` in `SpawnRound2Nerdoid`). `(int)ab[nPick] == 0` is the
  byte compare. With the index already in `SI`, `sz[n - 1]` folds to
  `[bp+si-0x129]`. `_BX = n; --_BX; sz[_BX]` is `mov bx,si; dec bx; lea ax,
  [sz]; add bx,ax; cmp [bx]` (`EraseScoreFile`).
- Unsigned `>=` whose body is longer than 127 bytes jumps over that body
  with `jae +3; jmp`, then `jne` into the body and the same pair on the low
  word (`if (GetTickCount() / 17 >= g_dwR6Next)` in `SpawnRound6`, and
  `TickRound5`). `if (tick < next) goto done` shares one trampoline instead
  (`ja` / `jb` / `jae` / `jmp`).
- `do { ...; if (dx >= limit) { aim = x; break; } } while (1);` is a direct
  `jl` onto the loop-back `jmp`. `if (dx < limit) continue` is `jge; jmp`.
- `do if (ab[n] != 0) break; while (n-- != 0);` is `je` onto the while
  test and a `jmp` past it (`SpawnRound2Nerdoid`). An empty then with `else break`
  inverts that and adds a third `jmp`.
- `do { n = (rand() & 0x7FFF) % 3; if (a[n * 2] == 0 || a[n * 2 + 1] == 0)
  break; } while (1);` is `je` to a `jmp` past the loop, then `jne` to a
  `jmp` back to the top (`SpawnRound3Klutz`). `while (a[n * 2] != 0 &&
  a[n * 2 + 1] != 0)` is a single `jne` back to the top and four bytes
  shorter:

```
cmp word ptr [bx], 0
je L0175
cmp word ptr [bx], 0
jne L0177
L0175: jmp L0179                 ; a side is free
L0177: jmp L0145                 ; both taken, pick again
```
- One test in that loop is the short form. `do { ...; if (PtInRect(&rc, pt)
  == 0) break; } while (1);` is `or ax,ax; jne retry; jmp break; retry:
  jmp top` (`SpawnRound4Fungus`, `0b c0 / 75 02 / eb 03 / e9`).
  `while (PtInRect(&rc, pt));` is a direct `je` to the break.
- `p = &p[i]` for a struct pointer adds through `DX` (`mov dx,si; add dx,ax;
  mov si,dx`). `p += i` is `add si,ax` (`DecImageBottoms`).
- An unsigned long compare against memory loads the high word into `AX`
  and the low word into `DX`, then `cmp ax,high; ja; jne; cmp dx,low`
  (`ScoreQualifies`). `<=` is that compare with the greater path jumping
  over a `return`.
- Subtracting a constant other than 1 from a byte is `add al,-n`, and
  `- 1` is `dec`. `sub al,4; dec al` is not a C subtraction (`InsertScore`
  uses `asm` for those two instructions). `_AX -= 4; --_AX` is the word
  form `sub ax,4; dec ax` (`GetScoreInfo`). A plain
  `n = (int)GetCurrentPosition(hdc) - 0x59` is `add ax,-0x59`. The
  `sub ax,imm16` form is `_AX = GetCurrentPosition(hdc); _AX -= 0x59;
  n = _AX` (`PaintNextScores`).
- `_CX = x; --_CX; x = _CX` is `mov cx,[x]; dec cx; mov [x],cx`.
  `_CX = _CX - 1` reloads through `AX`. Using `_CX` in the loop body also
  keeps a `while (n--)` count on the stack instead of in `DI`.
- A signed `int` used only as `--n >= 0` stays in `DX`, and the test uses
  the flags of `dec`: `mov dx,ax; dec dx; jge` (`CopyPStr`, `PStrDiff`).
  Once `SI` and `DI` are taken, the same countdown is `dec cx; jge`
  (`SplicePStr`). `while (--n >= 0)` jumps to that `dec` first; a
  `do`/`while` does not. `_DX` and `_CX` are unsigned, so `--_DX >= 0`
  retests (`dec dx; mov ax,dx; or ax,ax; jae`) and is warned as always true.
  The same signed `int` stays in `DX` across later statements when it is the
  only live word: `n = g_nPlayWave; if (n >= 10) ...; t[n]` is
  `mov dx,[g_nPlayWave]; cmp dx,0xa; jl` and then `mov bx,dx; shl bx,1`
  (`SetupRound1`). `_DX` would compare with `jae`.
- Values loaded into `DX`/`CX` and reused across several statements are
  Borland pseudo-registers (`_DX = s->dx; _CX = s->dy; if (_DX || _CX)`).
  They are `unsigned`, so `x += _DX` with an `int` `x` goes through `AX`;
  `x += (int)_DX` gives the original's `add word ptr [si+0xd],dx`.
  As a temporary in a condition, `if ((_DX = (unsigned)s->pNext) != 0)
  ((SPRITE *)_DX)->pPrev = ...` gives `mov ax,[si+0x94]; mov dx,ax;
  or ax,ax; je ...; mov bx,dx` (`UnlinkSprite`).
- Struct assignment calls `SCOPY@` with the size in `CX`, destination pushed
  first, both as far pointers (`push ds`/`push ss`).
- Long division calls `LDIV@`/`LUDIV@` with the divisor pushed first;
  `GetTickCount() / 17` is `push 0; push 0x11; call GetTickCount; push dx;
  push ax; call LUDIV@`.
- The order of operands in `|` is visible. `LoadPictImage` swaps the bytes of
  a word with `HIBYTE(w) | LOBYTE(w) << 8` from `windows.h`: `mov ax,es:[bx];
  shr ax,8; mov ah,0` then `mov dl,es:[bx]; mov dh,0; shl dx,8; or ax,dx`.
  Writing the masks out by hand evaluates the halves the other way round.
- Comparing an expression with 0 normally reuses the flags of its last
  `or`/`and`. The original sometimes tests again with `or ax,ax`, which comes
  from a real conversion at the top of the expression: `GETWORD` is
  `(int)(WORD)SWAPWORD(...)`, so `if (GETWORD(lp + 2) < 0)` gives
  `or ax,dx; or ax,ax; jge` (`DecodePicImage`, `SeekPicRes`). A plain
  `(int)` cast of an `int` is not enough.
- `if (a && b) stmt; else if (c)` sends the false side of either test to
  the `else`: `cmp ax,[c]; jge else; cmp [dy],0x14; jle else`
  (`MoveRound1Player`). A nested `if` jumps to the `jmp` that skips the
  `else` instead.
- An assignment used as the value being compared stays in `AX`:
  `if ((n = rc.right - rc.left) < 0x200)` is `mov [n],ax; cmp ax,0x200`
  (`LayoutMain`). A separate `n = ...; if (n < 0x200)` reloads `[n]`.
- Signed division by 2 is `cdq; sub ax,dx; sar ax,1`. Adding a negative
  constant is `add ax,-n`, so `(rc.bottom + rc.top + -0x156) / 2` is that
  sequence (`LayoutMain`).
- `LOBYTE` of a memory word becomes a byte load (`mov dl,es:[bx+2]`), and so
  do `(BYTE)` casts and `& 0xFF` with a cast. A word load followed by
  `mov dh,0` comes from `LOBYTE(HIWORD(dw))` on a `DWORD` in memory: the
  `>> 16` cannot be narrowed further. `SeekPicRes` reads a big-endian long
  with `SWAPLONG(*(DWORD FAR *)lp)` (`GETLONG` in `aarcade.h`); `x & 0xFF`
  gives `and dx,0xff` instead.
- `x & ~3` on a signed `int` is `and ax,0xfffc; add word ptr [g],ax`
  (`MoveRound4Ball`, `25 fc ff`). `x & 0xFFFC` is unsigned, so the add
  reloads through `DX`.
- `abs` from `stdlib.h` inlines: `cwd; xor ax,dx; sub ax,dx` (`BallRgnRound4`,
  `99 / 33 c2 / 2b c2`). A local `int abs(int);` calls the library.
- `~x` compiles to `not`; `x ^ 0xFF` (or `^ 0xFFFF` on an `int`) gives
  `xor al,0xff`. The picture unpackers use the `xor` form.
- For a far pointer, `p += n` only adds to the offset (`add [bp+0xa],ax`),
  while `p = p + n` loads, adds and stores both words
  (`mov ax,[bp+0xc]; mov dx,[bp+0xa]; add dx,[bp+6]; mov [bp+0xc],ax;
  mov [bp+0xa],dx`). For a `huge` pointer, `p -= n` and `p = p - n` also
  differ, in how the `__AHSHIFT` carry is computed.
- `x = t[i] << 8;` shifts `AX` before the store, while `x = t[i]; x <<= 8;`
  with `x` in `SI` gives `mov si,ax; shl si,8`.
- Library calls name the exact routine. `atoi` and `atol` are separate code
  in this runtime; `GetWindowNumber` calls `atol`, so the source is
  `(int)atol(sz + 1)`.

## Data

- The data segment (DGROUP) is byte aligned, so globals can sit at odd
  addresses (`g_lActivateTime` at `0x3B21`, `g_rgbGame` at `0x3C3D`).
- Duplicate string literals are *not* merged (no `-d`). One address used for
  the same string in several places is a named global array
  (`g_szUserItemClass`). Two `""` at `0x6A` and `0x6B` are two separate
  literals in `InitControls`.
- Each module's initialized data, with its string literals at the end, sits
  together in link order; the same holds for uninitialized data. Borland
  emits a module's BSS in reverse order of first appearance. An `extern` in
  `aarcade.h` is seen before the definition, so the header lists each
  module's symbols with the highest address first (`g_idLineA` above
  `g_rcLineB`). A symbol that is not in the header (`round6.c`, `intro.c`)
  follows its definition order, also highest address first. Each module's
  BSS starts on a word. `inifile.c` therefore also holds the bytes through
  `3C7F` (they are what `util.c` reads): ending that module at `3C31` would
  insert a pad and shift `g_fAltLayout`. The one pad that remains is the
  zero at `3C81`, before `next.c`. The resulting blocks show
  which source file a global belongs to: `0x5A..0x6B` is `init.c`
  (`g_fBlackWhite`, `g_szMainClass`, two `""`), `0x6C..0x7B` is `dialog.c`.
  `tools/build.py` links objects in original code-segment order.
  `mmstub.asm` is the exception: its code joins the startup `_TEXT`, and it
  is linked after `view.c` so its table stays at `0x2C4`. `globals.c` is
  linked last and now adds nothing to DGROUP.
  A definition lives in the file whose data block contains its address, which
  is not always the file that uses it (`g_szHiFile` is in `inifile.c`).
- `nedump.py xrefs 24:ADDR --size N` lists the code that uses a global.
- `build/LST/NAME.ASM` (`bcc -S`) shows a module's data exactly as emitted:
  each global as a label with its `db`/`dw` bytes, statics too, and the
  module's literal pool as `s@`. `necmp.py --data` lists the items in the
  data segment that no `GLOBAL` annotation names.
- A `char` array of odd length does not make the compiler align the next
  `int`. `g_szMMSystem` (13 bytes) is followed immediately by `g_nSndPad`,
  and the linker's word alignment of the next module supplies the zero at
  `096B`, so `speaker.c` starts at `096C`.
- A copy that asks for more bytes than the array (`_fmemcpy(..., g_r6Cmd, 0x80)`,
  `_fmemcpy(..., g_r4Cmd, 0x40)`) reads into the next module's data. The
  array stops at the module boundary; the following bytes belong to the
  next file.

## Runtime library

The runtime library (`CWM.LIB`) is linked unchanged, so its functions match
once they are named. They are listed in `src/rtl.h` as `LIBRARY`
annotations, and library data such as `_timezone` and `__ctype` as `GLOBAL`
annotations with an explicit symbol.

When a library function shows as `differs`, the diff names the cause:
- calls to an unannotated helper (`call 0001:0388`): annotate the helper,
  its name is in `build/AARCADE.MAP`;
- references to library data: annotate it if the map lists it. Private
  library variables (the `srand` seed, tables used by `tzset`, `dostounix`
  and `__ISDST`) are not in the map, so those functions keep showing a
  difference until the data layout matches.

## How the comparator decides

`necmp.py` reports `exact` when the bytes are identical and `equivalent` when
only link-dependent values differ. Two instructions count as the same when:

- their text is equal after naming: relocations by target, far calls by
  function, data operands by annotated global (`name+offset` only within the
  global's size; `[si+0x70]` and `[di+disp]` keep their numbers, since an
  SI/DI base is a struct field), `push`/`mov`/`add` immediates that fall
  inside an annotated
  global (`&g_img[n]` is `offset G_IMG+0xN`; a string pushed right after
  `push ds` keeps its text), and `DS`-relative pointers to strings by their
  contents (`offset "SwampGasDlg"`). A Pascal literal counts when its first
  byte is the length of the rest (`"\x06000000"` in `PadNumString`);
- their bytes are identical and neither has a relocation (a constant that
  happens to equal a global's address on one side);
- a `[bx+disp]` displacement compares as a number even when the immediate
  is relocated. The relocation is the function a sprite field points at
  (`AddRound4Bungee` stores `MoveRound4Bungee` at `[bx+0x76]`), and each layout
  names that field offset after a different global. The target stays in the
  comparison, so a different function still fails;
- an immediate or a displacement is named on one side only and the texts
  agree with it left as a number. `OpenRound5`'s `0x100` and the
  `[bx+0x6a]` field (`pMasks`) in `MovedRound5Part` fall inside a global in
  just one of the two layouts, and the other operand still differs in bytes.
  The same when both sides name it and the names differ: `mov [g], 0x5a` is
  `offset G_FBLACKWHITE` in the original and another global in the rebuild;
- a displacement just below a global names the same `G_X-0xN` on both sides.
  `g_apR5AsmBin[g_nR5Col - 1]` is `[bx + &g_apR5AsmBin[-1]]`. In the original
  that's `G_ANR5FLOOR+0x10`; in the rebuild, where `globals.c`'s uninitialized
  data comes out in reverse order, it's `G_R5SCENE+0x28`;
- they differ only in which segment holds a function that is still a stub.
- a near call or jump to an address with no public is the same target on
  both sides. The rebuilt text is prefixed (`new:0001:0614`); the prefix is
  ignored. `__scantol` calls `0001:0614`, which is not in the map.

A near jump out of the function is named `symbol+offset` when it lands a few
bytes past a known symbol, as the long-division entry points do
(`LUDIV@: mov cx,1; jmp LUMOD@+0x3`). Annotate the neighbouring entry
points so both sides get the same name.

Trailing zero bytes that the linker adds for alignment are ignored.

Known blind spot: a zero-filled buffer in initialized data reads as `""`,
so a pointer to it compares equal to a pointer to any empty string. Annotate
such buffers as globals.

## Picture DLLs (AACOLOR / AABW)

`AACOLOR.DLL` and `AABW.DLL` are bit-identical to the originals. User code is
only `src/datalib/libmain.c` (`LibMain` returns 1; no `UnlockData`). `__WEP`
comes from Borland `CWC.LIB`. Resources are bound with `BRC -30` the same way
as the EXE.

Link shape (packing on — no `/P-`; `DATA PRELOAD FIXED SINGLE`; `HEAPSIZE 4096`;
`WEP @1 RESIDENTNAME`):

| DLL | Startup | LibMain model | Runtime |
|---|---|---|---|
| `AABW.DLL` | `C0DS` (LockSegment / UnlockSegment) | `-ms` | `CWC` |
| `AACOLOR.DLL` | `C0DC` (no LockSegment) | `-mc` | `CWC` |

`CWC`'s `HEAP` routes `malloc` through `farmalloc` / `GlobalAlloc`, which pulls
`ASMMEM` / `WINMEM` / `FHEAP` and leaves `__WEP` just before `ASMMEM` — the
layout the originals have. Do not reorder EXTDEFs in the `C0D*.OBJ` copies:
FIXUPP records index those names, so swapping `__WEP` / `__cexit` rewires the
WEP thunk's near calls.

`AACOLOR.DLL` carries a trailing CodeView (`NB02`) overlay after the last
resource. Rebuild with `-v` will not reproduce it; `extract_res.py` saves it
as `build/res/AACOLOR/CODEVIEW.bin` and `build.py` appends that file once the
NE image matches. `AABW.DLL` has no overlay.

`tools/verify.py --dlls` requires both DLL hashes (and the EXE) to match
`orig/SHA256SUMS`. `METER.DLL` remains out of scope.

Both DLLs are annotated (`src/datalib/libmain.c`, `src/datalib/rtl.h`) and
included in `docs/progress.json` under `modules`, next to `AARCADE`.

## Progress inventory

`necmp.py` writes `docs/progress.json` for ByteMatch via `tools/report.py`.
The file has a top-level `linked` flag (all modules byte-identical) and a
`modules` map keyed by `AARCADE` / `AACOLOR` / `AABW`. Each module records
`functions`, code `segments`, `gaps` (uncovered stretches with a `zero` flag
for linker alignment padding), and `data` coverage. Report totals exclude
all-zero gaps from `total_code`; identical string-literal pools and
inter-contribution data padding count as matched data.
