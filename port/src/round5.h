/* Andromedroids. Matching source: src/round5.c (segment 31). */

#ifndef ROUND5_H
#define ROUND5_H

#include "pic.h"
#include "play.h"

typedef struct Round5 Round5;

Round5 *round5_new(const char *resources);
void round5_set_cursor(Round5 *round, int screen_x);
void round5_click(Round5 *round);
int round5_holding(const Round5 *round);
void round5_wave_info(const Round5 *round, int *wave, long *score, long *time_left);
void round5_add_score(Round5 *round, long n);
/* NextPlayWave and SetupRound5. This round has no time bonus. */
void round5_finish_wave(Round5 *round);
/* One WM_TIMER. The scene itself runs on every third 17ms tick. */
int round5_timer(Round5 *round);
int round5_pump(Round5 *round);
void round5_paint(const Round5 *round, Pic *field);
void round5_status(const Round5 *round, PlayHud *hud);
void round5_free(Round5 *round);

#endif
