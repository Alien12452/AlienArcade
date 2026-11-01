/* Opening animation (dialog 130). Matching source: src/intro.c (segment 34).
   RunStartDialog plays it before the game list. Pictures 4001/4002 and the
   move scripts live in that file. */

#ifndef INTRO_H
#define INTRO_H

#include <SDL3/SDL.h>

typedef struct Intro Intro;

Intro *intro_new(SDL_Renderer *renderer, const char *resources);
void intro_free(Intro *in);
/* One timer tick. Returns 1 when the last cue has fired. */
int intro_tick(Intro *in);
void intro_draw(SDL_Renderer *renderer, Intro *in);
/* StartDlgProc WM_COMMAND results: 0 ignore, 1 dismiss, 2/101 quit. */
int intro_key(Intro *in, SDL_Keycode key, SDL_Keymod mod);
void intro_press(Intro *in, float x, float y);
int intro_release(Intro *in, float x, float y);

#endif
