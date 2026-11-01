/* Between waves: gate, instructions, and time-bonus UI.
   Matching source: src/between.c (segment 35).
   StepDlgProc / WaveDlgProc / RoundHelpDlgProc / CountTimeBonus /
   RunStepDialog / RunRoundHelp. Round-done is in scores.c
   (RunRoundDone / RoundDoneDlgProc). */

#ifndef BETWEEN_H
#define BETWEEN_H

#include <SDL3/SDL.h>

typedef struct Between Between;

Between *between_new(SDL_Renderer *renderer, const char *resources, int round);
void between_free(Between *ui);

/* Past the gate, including while a bonus dialog is up. */
int between_playing(const Between *ui);
/* A bonus dialog is on screen, counting or waiting for OK. */
int between_busy(const Between *ui);
/* Gate, help, or bonus — leave/quit confirm must not steal Esc/Ctrl. */
int between_modal(const Between *ui);

void between_motion(Between *ui, float x, float y);
void between_press(Between *ui);
int between_release(Between *ui);
/* Immediate click (tests / headless). Prefer press/release in the shell. */
void between_click(Between *ui);
/* Enter or Space. OK on the gate, the help, and a finished bonus. */
void between_ok(Between *ui);
/* Esc: OK on the gate and a finished bonus; ignored on help / while counting. */
void between_cancel(Between *ui);
/* The Instructions button. Only the gate has one. */
void between_instructions(Between *ui);

/* Headless play does not wait on the gate. */
void between_skip_gate(Between *ui);

/* Open the bonus dialog for the wave that just finished. */
void between_begin(Between *ui, int wave, long score, long time_left);
/* One slice of CountTimeBonus. *added is a score chunk to grant now.
   When *timed is set, the status clock shows *time_left. */
void between_tick(Between *ui, Uint64 now, long *added, int *timed, long *time_left);
/* 1 when OK closed the bonus. *supply is the +20 pies or comics. */
int between_take(Between *ui, int *supply);

/* The same bonus, granted in one go. Rounds 1-3 pay 5 a second, the rest 10. */
void between_grant(int round, long time_left, void *user, void (*add)(void *, long n));

void between_draw(SDL_Renderer *renderer, Between *ui);

#endif
