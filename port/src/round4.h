/* Fungus Patrol. Matching source: src/round4.c (segment 32). */

#ifndef ROUND4_H
#define ROUND4_H

#include "pic.h"
#include "play.h"

typedef struct Round4 Round4;

Round4 *round4_new(const char *resources);
void round4_set_cursor(Round4 *round, int screen_x, int screen_y);
void round4_set_button(Round4 *round, int down);
void round4_click(Round4 *round);
int round4_holding(const Round4 *round);
void round4_wave_info(const Round4 *round, int *wave, long *score, long *time_left);
void round4_add_score(Round4 *round, long n);
void round4_set_time(Round4 *round, long time_left);
void round4_tick_time(Round4 *round);
/* NextPlayWave, ArmRound4, SetupRound4. The bonus dialog adds the score. */
void round4_finish_wave(Round4 *round);
/* One WM_TIMER. The scene itself runs on every third 17ms tick. */
int round4_timer(Round4 *round);
int round4_pump(Round4 *round);
void round4_paint(const Round4 *round, Pic *field);
void round4_status(const Round4 *round, PlayHud *hud);
void round4_free(Round4 *round);

#endif
