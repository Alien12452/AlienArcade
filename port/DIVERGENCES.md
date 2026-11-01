# Port divergences from matching `src/`

The matching decompilation in `src/` is the behaviour specification for game
logic. This SDL3 port aims to match that, but some differences are intentional:
host substitution, playability, or small hardenings. `docs/MATCHING.md` is only
about Borland codegen for the original EXE — not this list.

When you add another intentional mismatch, put a bullet here and a one-line
`/* port: ... */` at the call site when it helps.

## Host / platform

- **SDL3** replaces Win16: window, renderer, audio stream, mouse/keyboard, no
  HWND / GDI / WaveOut / `SetTimer` callback.
- High-score file is `AARCADE.HI` under the SDL pref path
  (`alienarcade/AlienArcade/`), not the Windows directory.
- Lists for all six rounds stay in memory; the original loaded one round at a
  time via the ini API.

## Timing

- **Play anim gate (~51 ms).** `StartAnimTimer` asked for 17 ms, but Win 3.1
  usually delivered closer to the ~55 ms system tick, so `TickRound` ran ~18 Hz.
  Polling every frame with precise `SDL_GetTicks()/17` made the port ~59 Hz
  (unplayably fast). `play_tick` only pumps a round when three `/17` units have
  elapsed (~51 ms). `scene_due` / TickScene catch-up math is unchanged once a
  pump runs. See `play_anim_due` in `play.c`.
- Intro, select, scores strips, and between help panes advance on the same
  ~51 ms cadence (`SubclassAnimItem` rate 3). Strip tick helpers accumulate
  elapsed time so a heavy frame cannot permanently slow the marquee, and
  restamp after a long hide (>500 ms) so post-game board open does not dump
  catch-up at max steps.
- Round wall-clock (`time()` / `Time left`) is unchanged.

## Original quirks kept

- Select screen: clicking a game **icon** does nothing; only the name / hit
  targets start a round (matches original).
- Clear-scores confirm: Enter means **No** (default button was No).
- Hit-test nesting and related TickScene shapes match `src/scene.c`.

## Port hardenings

- Typed high-score names drop `=` and `;` so they cannot break the `.HI` value
  format (original allowed them and could corrupt the file).
- Abort → scores board loads that round’s real lists. The original only set
  `g_nRound` on abort, so the board could show stale lists from the last load.

## Presentation

- Full-frame repaint each frame instead of dirty rectangles (same visible
  result for gameplay).
- Dialogs use the game font and simple SDL chrome, not the system dialog look.
- Colour banks only; the mono / `g_fColor == 0` branches are not ported.
- No software game cursor; the OS cursor is used.
- Text uses Liberation Sans via FreeType at 9/12 px em height (SDL debug font
  if no TTF is found), not Win16 raster `Helv`. Glyph widths and ascent differ
  slightly, so centring / wrap can shift by a pixel or two. No kerning;
  glyphs outside ASCII 32–126 render as `?`.

## Pictures / PPIC

- Colour banks only: mono (`UnpackColumn1`), 8-bpp (`UnpackColumn8`), and
  embedded-CLUT skip paths are not ported (`init` caps depth at 4; no shipped
  colour image uses those branches).
- Palette is always `g_rgbCustom`. The original used `g_rgbVga` when
  `g_nColorDepth <= 4`.
- BitBlt ROPs run on RGB bytes rather than device palette indices. Equivalent
  here because mask art is palette 0/15 (white/black are bitwise inverses).
- Decode returns `NULL` on malformed or overrunning input instead of walking
  past the buffer. Callers only pass in-range indices (no `SeekPicRes` clamp).

## Sound

- One voice, like `PlaySfx` / `PlaySoundEx`; the third `PlaySoundEx` argument is
  ignored (as with `OpenWaveOut` in the matching sources).
- `sound_preload` mirrors `PreloadSounds` where rounds call it (skipped when
  muted, like `!g_fSoundOn`).
- Ctrl+0..9 volume: step 0 mutes; 1–9 scale PCM with an audible port ladder
  (~12%→100%). The original `SelectVolume` only toggled `g_fSoundOn` (0 off,
  1–9 full) and never applied `g_anVolume` to samples. Chords still match
  accel table 1 (Ctrl only, live play with no modal).
- No `AARCADE.INI` `SoundOn` / `useMMIO` or `/NS` switch; always WAVE via SDL
  (no PC-speaker path). Clip cache is 48 slots vs orig 16.
