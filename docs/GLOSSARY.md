# Alien Arcade glossary

Names for matching `src/` and the SDL `port/` follow the game's own wording:
instruction dialogs, the string table, and on-screen labels in
`build/res/AARCADE.RC`. Prefer those nouns over generic arcade terms
(enemy, bomb, missile, shot).

Borland export names (`MainWndProc`, …), RTL symbols in `src/rtl.h`, and
`mmstub.asm` symbols are real linker names and are not renamed.

Identifiers stay under 32 significant characters.

## Shared HUD / play state

| Game text | Identifier stem | Notes |
|---|---|---|
| Pies / Comics / Bananas / Saves | `PlayAmmo` (`g_nPlayAmmo`) | Left stock; round-specific label |
| Score | `PlayScore` (`g_lPlayScore`) | Six-digit score |
| Wave | `PlayWave` (`g_nPlayWave`) | Shown as `n+1`; caps at 98 |
| Time | `TimeLeft` | Already correct |
| Droids left | `R5Droids` | Round 5 only |
| Parts `NN-NN-NN` | `R5Parts` / `R5Count` | Heads, arms, legs supply |

## Rounds

| # | Title | Player / tool | Ammo / stock | Main foe | Other |
|---|---|---|---|---|---|
| 1 | Stink Snakes | player | Pie | Snake | Happy face (after hit) |
| 2 | Nerdoids | player | Comic | Nerdoid | Fall (= calculator or pocket protector) |
| 3 | Quasar Klutzes | player ship | Banana | Klutz | Robot Dog; gas Cloud; Bolt (unnamed in text — kept) |
| 4 | Fungus Patrol | scraper / tool | — | Fungus (+ egg score branch) | Bungee cord |
| 5 | Andromedroids | player | Part | — | Mercurian Metal Bug; assembly bin; recycle |
| 6 | Quasar Rain | Mom-E-Tron | Saves | Klutz | Banana peel; Safe-T-Net; bed |

## Round-local stems

### Round 1 — Stink Snakes
`Pie`, `Snake`, `Happy` (hook after splat). Inventory remains `PlayAmmo`.

### Round 2 — Nerdoids
`Comic`, `Nerdoid`, `Fall` (shared path for calculator / pocket protector).

### Round 3 — Quasar Klutzes
`Banana`, `Klutz`, `RobotDog` / `Dog`, `Cloud` (gas cloud).
`Bolt` has no resource name; left as `Bolt`.

### Round 4 — Fungus Patrol
`Fungus`, `Bungee`. Player tool stays `Ball` until art proves a noun
(port comments say “scraper”; not in dialogs).

### Round 5 — Andromedroids
`Part`, `Bug`, `AsmBin` / `Bin`, `Droids`, `Recycled`.

### Round 6 — Quasar Rain
`MomETron` / `MomE`, `Klutz`, `Peel`, `Miss` (Safe-T-Net miss counter).

## Deliberately not renamed

- Per-round spawn cadence fields named `*Wave*` (`g_anR3Wave`, …) — not the HUD wave.
- Layout / picture “frame” (`g_arcFrame`, sprite `.frame`).
- Opaque bookkeeping (`g_nR1Gone`, `g_nR4Count`, `g_lSavedCount`) without a clear game noun.
- Engine APIs (`SPRITE`, `SCENE`, image helpers) that are not game lore.
