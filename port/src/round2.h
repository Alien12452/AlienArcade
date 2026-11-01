/* Nerdoids. Matching source: src/round2.c (segment 28). */

#ifndef ROUND2_H
#define ROUND2_H

#include "pic.h"
#include "play.h"

typedef struct Round2 Round2;

Round2 *round2_new(const char *resources);
void round2_set_cursor(Round2 *round, int screen_x);
void round2_click(Round2 *round);
int round2_holding(const Round2 *round);
void round2_wave_info(const Round2 *round, int *wave, long *score, long *time_left);
void round2_add_score(Round2 *round, long n);
void round2_set_time(Round2 *round, long time_left);
void round2_tick_time(Round2 *round);
void round2_add_supply(Round2 *round, int n);
/* NextPlayWave through the spawn that follows the bonus dialog. */
void round2_finish_wave(Round2 *round);
/* One WM_TIMER. The scene itself runs on every third 17ms tick. */
int round2_timer(Round2 *round);
int round2_pump(Round2 *round);
void round2_paint(const Round2 *round, Pic *field);
void round2_status(const Round2 *round, PlayHud *hud);
void round2_free(Round2 *round);

#endif
