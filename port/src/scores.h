/* High scores and name dialog. Matching:
   src/score.c (segment 0x16), src/slist.c (seg 7), RunNameDialog (000D:062C),
   PaintNextScores (0013:01B4), RunNextDialog (0012:0372),
   RunRoundDone / RoundDoneDlgProc (0023:08E9 / 0023:0902),
   clear.c confirm dialogs.
   Lists are AARCADE.HI, same shape as the original ini sections. */

#ifndef SCORES_H
#define SCORES_H

#include <SDL3/SDL.h>

typedef struct Scores Scores;

Scores *scores_open(SDL_Renderer *renderer, const char *resources);
void scores_close(Scores *scores);

/* A round has just ended. Loads nothing new; the lists were read at open. */
void scores_present(Scores *scores, int round, long score);
/* Show the lists for a round. Keeps ranks; plays 0x2331 if daily_rank is set. */
void scores_show(Scores *scores, int round);
/* ClearRanks — after attract board advances (post-idle LoadRoundScores). */
void scores_clear_ranks(Scores *scores);
int scores_needs_name(const Scores *scores);

/* The name field. The first edit replaces the previous name (EM_SETSEL). */
void scores_type(Scores *scores, const char *text);
void scores_backspace(Scores *scores);
/* 1 when the typed name is empty, which records nothing. */
int scores_name_blank(const Scores *scores);
/* Insert into whichever lists the score made, and write the file. */
void scores_commit(Scores *scores);
/* 1 when this game took a place on today's list. */
int scores_daily_place(const Scores *scores);

int scores_hit_over(const Scores *scores, float x, float y);
int scores_hit_name_ok(const Scores *scores, float x, float y);
/* 1 when (x,y) is inside the 0x202 x 0x158 board frame. */
int scores_hit_board(float x, float y);

void scores_draw_over(SDL_Renderer *renderer, const Scores *scores);
void scores_draw_name(SDL_Renderer *renderer, const Scores *scores);
void scores_draw_board(SDL_Renderer *renderer, Scores *scores);

/* R on the board clears this round. Shift+R erases the whole file.
   The confirm dialog returns 3 for Yes and 1 for No. */
void scores_reset_round(Scores *scores);
void scores_erase_all(Scores *scores);
void scores_draw_confirm(SDL_Renderer *renderer, int all);
int scores_hit_confirm(float x, float y, int all);

#endif
