/* Quasar Klutzes. Matching source: src/round3.c (segment 27). */

#ifndef ROUND3_H
#define ROUND3_H

#include "pic.h"
#include "play.h"

typedef struct Round3 Round3;

Round3 *round3_new(const char *resources);
void round3_set_cursor(Round3 *round, int screen_x);
void round3_click(Round3 *round);
int round3_holding(const Round3 *round);
void round3_wave_info(const Round3 *round, int *wave, long *score, long *time_left);
void round3_add_score(Round3 *round, long n);
void round3_set_time(Round3 *round, long time_left);
void round3_tick_time(Round3 *round);
/* NextPlayWave through SpawnRound3Klutz. now 0 keeps the clock from the clear. */
void round3_finish_wave(Round3 *round, uint32_t now);
/* One WM_TIMER. The scene itself runs on every third 17ms tick. */
int round3_timer(Round3 *round);
int round3_pump(Round3 *round);
void round3_paint(const Round3 *round, Pic *field);
void round3_status(const Round3 *round, PlayHud *hud);
void round3_free(Round3 *round);

#endif
