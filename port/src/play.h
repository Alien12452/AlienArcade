/* One round of play: border, field, status, gate/bonus via between.c.
   Matching shell: RunPlay / PlayWndProc (000D:03AE / 000D:0443),
   SetRound (001A:0000) and Open/Tick/Paint/CloseRoundN, DrawStatusBar
   (001D:054F). All six rounds run. */

#ifndef PLAY_H
#define PLAY_H

#include <SDL3/SDL.h>

typedef struct Play Play;

/* DrawStatusBar (001D:054F) fields. Rounds fill this; play_draw places the pieces. */
typedef struct PlayHud {
    /* 0: supply + value; 1: title label only (round 4); 2: parts + droids (round 5) */
    int kind;
    const char *left_label;
    int left_x;
    char left_value[16];
    char wave[8];
    char mid_value[16];
    long score;
} PlayHud;

Play *play_load(SDL_Renderer *renderer, const char *resources, int round);
/* screen_y is the pointer in the 640x480 view, which is what GetMessagePos
   reports when that desktop is at the origin. */
void play_pointer(Play *play, float screen_x, float screen_y);
/* Left or right button, polled the way GetAsyncKeyState is. */
void play_button(Play *play, int down);
void play_click(Play *play);
void play_press(Play *play);
void play_release(Play *play);
/* Enter/Space/Esc on gate/help/bonus. I opens instructions from the gate. */
void play_key(Play *play, SDL_Keycode key);
/* Gate, help, or bonus dialog is up. */
int play_modal(const Play *play);
/* Dialog 200. Refused while the gate, the help, or a bonus is up. */
int play_pause(Play *play);
int play_paused(const Play *play);
void play_resume(Play *play);
/* ResumeTimeLeft's clock stamp, without clearing a pause. */
void play_catchup(Play *play);
/* One frame of the 17ms timer. Returns 1 when the round is over. */
int play_tick(Play *play);
/* One timer fire with a single scene step, ignoring the clock. */
int play_step(Play *play);
void play_draw(SDL_Renderer *renderer, Play *play);
/* Cabinet frame only (after the play window would have been destroyed). */
void play_draw_border(SDL_Renderer *renderer, const Play *play);
long play_score(const Play *play);
int play_round(const Play *play);
void play_free(Play *play);

#endif
