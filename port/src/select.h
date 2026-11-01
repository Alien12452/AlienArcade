/* Game-select screen. Matching:
   LayoutMain / PaintMain (000E:0000 / 000E:0214)  select_paint_frame / place
   PlayDlgProc (000D:0105) / g_anPlayCtl           cards and hit testing
   PlayBorderProc (000D:0000)                     timer / card frames
   OpenViews / TickViews / CloseViews
     (0008:1451 / 0008:14BA / 0008:1501)           animated card pictures
   LineWndProc (0008:11DA)                        peach strips
   Coordinates are g_anPlayCtl pixels, not dialog-unit resource coords. */

#ifndef SELECT_H
#define SELECT_H

#include <SDL3/SDL.h>

#include "pic.h"

typedef struct Select Select;

Select *select_load(SDL_Renderer *renderer, const char *resources);
/* TickViews + PlayBorderProc WM_TIMER. Returns 1 after 30 seconds. */
int select_tick(Select *screen);
void select_draw(SDL_Renderer *renderer, Select *screen);
/* PlayDlgProc results: 0 ignore, 1..6 pick, -1 idle (scores), 101 quit. */
int select_key(Select *screen, SDL_Keycode key, SDL_Keymod mod);
void select_press(Select *screen, float x, float y);
int select_release(Select *screen, float x, float y);
/* LayoutMain's border. One of the four sets, chosen once per launch. */
void select_paint_frame(Pic *canvas, const char *resources);
/* CenterWindow. The moonscape set adds (47, 36). */
void select_place(int w, int h, int *x, int *y);
void select_free(Select *screen);

#endif
