/* Quasar Rain. Matching source: src/round6.c (segment 33). */

#ifndef ROUND6_H
#define ROUND6_H

#include "pic.h"
#include "play.h"

typedef struct Round6 Round6;

Round6 *round6_new(const char *resources);
void round6_set_cursor(Round6 *round, int screen_x);
int round6_holding(const Round6 *round);
void round6_wave_info(const Round6 *round, int *wave, long *score, long *time_left);
void round6_add_score(Round6 *round, long n);
void round6_set_time(Round6 *round, long time_left);
void round6_tick_time(Round6 *round);
/* NextPlayWave through the spawn inside the clear. now 0 keeps that clock. */
void round6_finish_wave(Round6 *round, uint32_t now);
/* One WM_TIMER. The scene itself runs on every third 17ms tick. */
int round6_timer(Round6 *round);
int round6_pump(Round6 *round);
void round6_paint(const Round6 *round, Pic *field);
void round6_status(const Round6 *round, PlayHud *hud);
void round6_free(Round6 *round);

#endif
