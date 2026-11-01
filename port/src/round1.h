/* Stink Snakes. Matching source: src/round1.c (segment 30). */

#ifndef ROUND1_H
#define ROUND1_H

#include "pic.h"
#include "play.h"

typedef struct Round1 Round1;

Round1 *round1_new(const char *resources);
void round1_set_cursor(Round1 *round, int screen_y);
void round1_click(Round1 *round);
int round1_holding(const Round1 *round);
void round1_wave_info(const Round1 *round, int *wave, long *score, long *time_left);
void round1_add_score(Round1 *round, long n);
void round1_set_time(Round1 *round, long time_left);
/* PauseTimeLeft / TickTimeLeft: apply elapsed wall time before freezing. */
void round1_tick_time(Round1 *round);
void round1_add_supply(Round1 *round, int n);
/* NextPlayWave, ArmRound1, SetupRound1. The bonus dialog adds the score and the pies. */
void round1_finish_wave(Round1 *round);
/* One timer fire. scene_steps is how many times TickScene runs inside it.
   Returns 1 when the pies are gone and snakes are still out. */
int round1_timer(Round1 *round, int scene_steps);
/* The real 17ms timer. Returns the same loss flag, or 0 if it is not time. */
int round1_pump(Round1 *round);
void round1_paint(const Round1 *round, Pic *field);
void round1_status(const Round1 *round, PlayHud *hud);
void round1_free(Round1 *round);

#endif
