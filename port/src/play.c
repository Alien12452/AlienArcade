#include "play.h"

#include "between.h"
#include "pic.h"
#include "round1.h"
#include "round2.h"
#include "round3.h"
#include "round4.h"
#include "round5.h"
#include "round6.h"
#include "select.h"
#include "text.h"

#include <SDL3/SDL.h>

/* Matching: src/play.c (seg 13), src/round.c (seg 26), status.c, between.c.
   RunPlay / PlayWndProc (000D:03AE / 000D:0443)   play_load / tick / draw
   SetRound / OpenRound (001A:0000 / 001A:000F)    round choice + open
   DrawStatusBar / FormatTime (001D:054F / 0737)   play_draw status line
   Pause path (Control-P)                          play_pause / resume
   Gate / bonus                                    between_* (see between.c) */

#define VIEW_W 640
#define VIEW_H 480
#define FIELD_W 0x200
#define FIELD_H 0x156
#define SCENE_H 0x146

struct Play {
    SDL_Texture *picture;
    SDL_Texture *field;
    Pic *field_pic;
    Round1 *round1;
    Round2 *round2;
    Round3 *round3;
    Round4 *round4;
    Round5 *round5;
    Round6 *round6;
    Between *ui;
    int nround;
    float mx, my;
    uint8_t bg_r, bg_g, bg_b;
    PlayHud hud;
    int field_x;
    int field_y;
    int paused;
    /* port: Win3.1-ish anim timer — GetTicks()/17 units; due every 3 (~51 ms). */
    int timer_started;
    uint32_t timer_tick;
};

/* Brushes from InitGame / BackRoundN. Status via DrawStatusBar / PlayHud. */
static const struct {
    uint8_t r, g, b;
} k_round[6] = {
    {102, 204, 51},
    {153, 255, 255},
    {255, 204, 153},
    {192, 192, 192},
    {255, 255, 102},
    {255, 204, 153},
};

/* orig: DrawStatusBar (001D:054F) — baseline y 0x153, yellow g_rgbGame[3] */
static void draw_hud(SDL_Renderer *renderer, int field_x, int field_y, const PlayHud *hud)
{
    float y = (float)(field_y + 0x153 - text_ascent(9, 1));
    char score[16];

    if (!hud)
        return;
    if (hud->left_label) {
        text_draw(renderer, (float)(field_x + hud->left_x), y, hud->left_label, 9, 1, 255, 255, 102);
        /* kind 1 is title-only (Fungus Patrol); no supply count. */
        if (hud->kind != 1)
            text_draw(renderer, (float)(field_x + (hud->kind == 2 ? 0x4B : 0x56)), y,
                      hud->left_value, 9, 1, 255, 255, 102);
    }
    text_draw(renderer, (float)(field_x + 0xA3), y, "Wave:", 9, 1, 255, 255, 102);
    text_draw(renderer, (float)(field_x + 0xCF), y, hud->wave, 9, 1, 255, 255, 102);
    if (hud->kind == 2) {
        text_draw(renderer, (float)(field_x + 0x101), y, "Droids left:", 9, 1, 255, 255, 102);
        text_draw(renderer, (float)(field_x + 0x150), y, hud->mid_value, 9, 1, 255, 255, 102);
    } else {
        text_draw(renderer, (float)(field_x + 0x10A), y, "Time:", 9, 1, 255, 255, 102);
        text_draw(renderer, (float)(field_x + 0x131), y, hud->mid_value, 9, 1, 255, 255, 102);
    }
    text_draw(renderer, (float)(field_x + 0x17D), y, "Score:", 9, 1, 255, 255, 102);
    SDL_snprintf(score, sizeof(score), "%06ld", hud->score < 0 ? 0L : hud->score);
    text_draw(renderer, (float)(field_x + 0x1A9), y, score, 9, 1, 255, 255, 102);
}

static SDL_Texture *texture_from_pic(SDL_Renderer *renderer, Pic *pic)
{
    SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STATIC, pic->w, pic->h);
    if (!tex)
        return NULL;
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    if (!SDL_UpdateTexture(tex, NULL, pic->rgba, pic->w * 4)) {
        SDL_DestroyTexture(tex);
        return NULL;
    }
    return tex;
}

static int holding(const Play *play)
{
    return round1_holding(play->round1) || round2_holding(play->round2) ||
           round3_holding(play->round3) || round4_holding(play->round4) ||
           round5_holding(play->round5) || round6_holding(play->round6);
}

static int gated(const Play *play)
{
    return play->ui && !between_playing(play->ui);
}

static void wave_info(const Play *play, int *wave, long *score, long *time_left)
{
    if (play->round1)
        round1_wave_info(play->round1, wave, score, time_left);
    if (play->round2)
        round2_wave_info(play->round2, wave, score, time_left);
    if (play->round3)
        round3_wave_info(play->round3, wave, score, time_left);
    if (play->round4)
        round4_wave_info(play->round4, wave, score, time_left);
    if (play->round5)
        round5_wave_info(play->round5, wave, score, time_left);
    if (play->round6)
        round6_wave_info(play->round6, wave, score, time_left);
}

static void add_score(Play *play, long n)
{
    if (play->round1)
        round1_add_score(play->round1, n);
    if (play->round2)
        round2_add_score(play->round2, n);
    if (play->round3)
        round3_add_score(play->round3, n);
    if (play->round4)
        round4_add_score(play->round4, n);
    if (play->round5)
        round5_add_score(play->round5, n);
    if (play->round6)
        round6_add_score(play->round6, n);
}

static void grant_add(void *user, long n)
{
    add_score(user, n);
}

static void set_time(Play *play, long time_left)
{
    if (play->round1)
        round1_set_time(play->round1, time_left);
    if (play->round2)
        round2_set_time(play->round2, time_left);
    if (play->round3)
        round3_set_time(play->round3, time_left);
    if (play->round4)
        round4_set_time(play->round4, time_left);
    if (play->round6)
        round6_set_time(play->round6, time_left);
}

static void add_supply(Play *play, int n)
{
    if (n == 0)
        return;
    if (play->round1)
        round1_add_supply(play->round1, n);
    if (play->round2)
        round2_add_supply(play->round2, n);
}

static void finish_wave(Play *play, uint32_t now)
{
    if (play->round1)
        round1_finish_wave(play->round1);
    if (play->round2)
        round2_finish_wave(play->round2);
    if (play->round3)
        round3_finish_wave(play->round3, now);
    if (play->round4)
        round4_finish_wave(play->round4);
    if (play->round5)
        round5_finish_wave(play->round5);
    if (play->round6)
        round6_finish_wave(play->round6, now);
}

static int pump(Play *play)
{
    if (play->round6)
        return round6_pump(play->round6);
    if (play->round5)
        return round5_pump(play->round5);
    if (play->round4)
        return round4_pump(play->round4);
    if (play->round3)
        return round3_pump(play->round3);
    if (play->round2)
        return round2_pump(play->round2);
    if (play->round1)
        return round1_pump(play->round1);
    return 0;
}

static int step_timer(Play *play)
{
    if (play->round6)
        return round6_timer(play->round6);
    if (play->round5)
        return round5_timer(play->round5);
    if (play->round4)
        return round4_timer(play->round4);
    if (play->round3)
        return round3_timer(play->round3);
    if (play->round2)
        return round2_timer(play->round2);
    if (play->round1)
        return round1_timer(play->round1, 1);
    return 0;
}

static void commit_bonus(Play *play)
{
    long time_left = 0;

    wave_info(play, NULL, NULL, &time_left);
    between_grant(play->nround, time_left, play, grant_add);
    if (play->nround == 1 || play->nround == 2)
        add_supply(play, 20);
    /* Match GetTickCount/17 after RunStepDialog — not the wave-clear stamp. */
    finish_wave(play, (uint32_t)(SDL_GetTicks() / 17));
}

/* orig: RunPlay (000D:03AE) / OpenRound (001A:000F) */
Play *play_load(SDL_Renderer *renderer, const char *resources, int round)
{
    Play *play;
    Pic *canvas;
    Pic *field;
    const int which = round - 1;

    if (round < 1 || round > 6)
        return NULL;
    play = SDL_calloc(1, sizeof(*play));
    if (!play)
        return NULL;
    select_place(FIELD_W, FIELD_H, &play->field_x, &play->field_y);
    play->bg_r = k_round[which].r;
    play->bg_g = k_round[which].g;
    play->bg_b = k_round[which].b;

    canvas = pic_canvas(VIEW_W, VIEW_H, 0, 0, 0);
    field = pic_canvas(FIELD_W, FIELD_H, 0, 0, 0);
    if (!canvas || !field) {
        pic_free(canvas);
        pic_free(field);
        SDL_free(play);
        return NULL;
    }
    select_paint_frame(canvas, resources);
    pic_fill(field, 0, 0, FIELD_W, SCENE_H,
             k_round[which].r, k_round[which].g, k_round[which].b);
    pic_fill(field, 0, SCENE_H, FIELD_W, FIELD_H - SCENE_H, 0, 0, 0);
    pic_blit(canvas, play->field_x, play->field_y, field, field->w, field->h);
    play->picture = texture_from_pic(renderer, canvas);
    pic_free(canvas);
    if (!play->picture) {
        pic_free(field);
        SDL_free(play);
        return NULL;
    }
    if (round == 1)
        play->round1 = round1_new(resources);
    else if (round == 2)
        play->round2 = round2_new(resources);
    else if (round == 3)
        play->round3 = round3_new(resources);
    else if (round == 4)
        play->round4 = round4_new(resources);
    else if (round == 5)
        play->round5 = round5_new(resources);
    else if (round == 6)
        play->round6 = round6_new(resources);
    play->field_pic = field;
    play->field = (play->round1 || play->round2 || play->round3 || play->round4 ||
                   play->round5 || play->round6)
                      ? texture_from_pic(renderer, field) : NULL;
    if ((!play->round1 && !play->round2 && !play->round3 && !play->round4 &&
         !play->round5 && !play->round6) || !play->field) {
        play_free(play);
        return NULL;
    }
    play->nround = round;
    play->ui = between_new(renderer, resources, round);
    return play;
}

void play_pointer(Play *play, float screen_x, float screen_y)
{
    if (!play)
        return;
    play->mx = screen_x;
    play->my = screen_y;
    between_motion(play->ui, screen_x, screen_y);
    if (play->round1)
        round1_set_cursor(play->round1, (int)screen_y);
    if (play->round2)
        round2_set_cursor(play->round2, (int)screen_x);
    if (play->round3)
        round3_set_cursor(play->round3, (int)screen_x);
    if (play->round4)
        round4_set_cursor(play->round4, (int)screen_x, (int)screen_y);
    if (play->round5)
        round5_set_cursor(play->round5, (int)screen_x);
    if (play->round6)
        round6_set_cursor(play->round6, (int)screen_x);
}

void play_button(Play *play, int down)
{
    if (!play)
        return;
    if (gated(play) || (play->ui && between_busy(play->ui)))
        return;
    if (play->round4)
        round4_set_button(play->round4, down);
}

void play_click(Play *play)
{
    if (!play)
        return;
    if (gated(play) || (play->ui && between_busy(play->ui))) {
        between_click(play->ui);
        return;
    }
    if (play->round1)
        round1_click(play->round1);
    if (play->round2)
        round2_click(play->round2);
    if (play->round3)
        round3_click(play->round3);
    if (play->round4)
        round4_click(play->round4);
    if (play->round5)
        round5_click(play->round5);
}

void play_press(Play *play)
{
    if (!play)
        return;
    if (gated(play) || (play->ui && between_busy(play->ui))) {
        between_press(play->ui);
        return;
    }
    play_click(play);
}

void play_release(Play *play)
{
    if (!play || !play->ui)
        return;
    if (gated(play) || between_busy(play->ui))
        between_release(play->ui);
}

int play_modal(const Play *play)
{
    return play && play->ui && between_modal(play->ui);
}

void play_key(Play *play, SDL_Keycode key)
{
    if (!play || !play->ui)
        return;
    if (key == SDLK_ESCAPE)
        between_cancel(play->ui);
    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
        between_ok(play->ui);
    else if (key == SDLK_I)
        between_instructions(play->ui);
}

/* orig: pause handling in PlayWndProc (000D:0443) / PauseTimeLeft (001D:0425) */
int play_pause(Play *play)
{
    if (!play || play->paused)
        return 0;
    if (gated(play) || (play->ui && between_busy(play->ui)))
        return 0;
    /* TickTimeLeft before freezing so a pending second is not dropped. */
    if (play->round1)
        round1_tick_time(play->round1);
    if (play->round2)
        round2_tick_time(play->round2);
    if (play->round3)
        round3_tick_time(play->round3);
    if (play->round4)
        round4_tick_time(play->round4);
    if (play->round6)
        round6_tick_time(play->round6);
    play->paused = 1;
    return 1;
}

int play_paused(const Play *play)
{
    return play && play->paused;
}

/* orig: ResumeTimeLeft (001D:0430) + RestoreAnimTimer after RunModalDialog */
void play_catchup(Play *play)
{
    long time_left = 0;

    if (!play)
        return;
    wave_info(play, NULL, NULL, &time_left);
    set_time(play, time_left);
    /* Do not dump dialog duration into the next anim step. */
    play->timer_tick = (uint32_t)(SDL_GetTicks() / 17);
    play->timer_started = 1;
}

void play_resume(Play *play)
{
    if (!play || !play->paused)
        return;
    play->paused = 0;
    play_catchup(play);
}

/*
 * port: StartAnimTimer asked for 17 ms, but Win3.1 usually delivered ~55 ms.
 * Gate pumps to every 3 × 17 ms so TickRound runs ~20 Hz instead of ~59 Hz.
 * scene_due / TickScene math is unchanged once a pump runs. See port/DIVERGENCES.md.
 */
static int play_anim_due(Play *play)
{
    uint32_t now = (uint32_t)(SDL_GetTicks() / 17);

    if (!play->timer_started) {
        play->timer_started = 1;
        play->timer_tick = now;
        return 1;
    }
    if ((int)(now - play->timer_tick) < 3)
        return 0;
    play->timer_tick = now;
    return 1;
}

/* orig: PlayWndProc timer + TickRoundN */
int play_tick(Play *play)
{
    int over = 0;
    Uint64 now;

    if (!play)
        return 0;
    if (play->paused)
        return 0;
    if (gated(play))
        return 0;
    if (!holding(play)) {
        if (!play_anim_due(play))
            return 0;
        over = pump(play);
    }
    if (!holding(play))
        return over;
    now = SDL_GetTicks();
    if (play->ui) {
        int supply = 0;

        /* OK is clicked before this tick, so take it before opening another. */
        if (between_take(play->ui, &supply)) {
            add_supply(play, supply);
            finish_wave(play, (uint32_t)(now / 17));
            play->timer_tick = (uint32_t)(now / 17);
            play->timer_started = 1;
            return pump(play);
        }
    }
    if (play->ui && !between_busy(play->ui)) {
        int wave = 1;
        long score = 0;
        long time_left = 0;

        wave_info(play, &wave, &score, &time_left);
        between_begin(play->ui, wave, score, time_left);
    }
    if (play->ui && between_busy(play->ui)) {
        long added = 0;
        int timed = 0;
        long time_left = 0;

        between_tick(play->ui, now, &added, &timed, &time_left);
        if (added)
            add_score(play, added);
        if (timed)
            set_time(play, time_left);
        return 0;
    }
    commit_bonus(play);
    play->timer_tick = (uint32_t)(SDL_GetTicks() / 17);
    play->timer_started = 1;
    return pump(play);
}

int play_step(Play *play)
{
    if (!play)
        return 0;
    between_skip_gate(play->ui);
    if (play->ui && between_busy(play->ui))
        return 0;
    if (holding(play))
        commit_bonus(play);
    return step_timer(play);
}

/* orig: LayoutMain frame after play window destroyed (name / board) */
void play_draw_border(SDL_Renderer *renderer, const Play *play)
{
    SDL_FRect dst;

    if (!play || !play->picture)
        return;
    dst.x = 0;
    dst.y = 0;
    dst.w = VIEW_W;
    dst.h = VIEW_H;
    SDL_RenderTexture(renderer, play->picture, NULL, &dst);
}

/* orig: PaintRoundN + DrawStatusBar (001D:054F) + border */
void play_draw(SDL_Renderer *renderer, Play *play)
{
    SDL_FRect dst;

    dst.x = 0;
    dst.y = 0;
    dst.w = VIEW_W;
    dst.h = VIEW_H;
    if ((play->round1 || play->round2 || play->round3 || play->round4 ||
         play->round5 || play->round6) &&
        play->field_pic && play->field) {
        pic_fill(play->field_pic, 0, 0, FIELD_W, SCENE_H, play->bg_r, play->bg_g, play->bg_b);
        pic_fill(play->field_pic, 0, SCENE_H, FIELD_W, FIELD_H - SCENE_H, 0, 0, 0);
        if (play->round1)
            round1_paint(play->round1, play->field_pic);
        if (play->round2)
            round2_paint(play->round2, play->field_pic);
        if (play->round3)
            round3_paint(play->round3, play->field_pic);
        if (play->round4)
            round4_paint(play->round4, play->field_pic);
        if (play->round5)
            round5_paint(play->round5, play->field_pic);
        if (play->round6)
            round6_paint(play->round6, play->field_pic);
        SDL_UpdateTexture(play->field, NULL, play->field_pic->rgba, FIELD_W * 4);
        if (play->round1)
            round1_status(play->round1, &play->hud);
        if (play->round2)
            round2_status(play->round2, &play->hud);
        if (play->round3)
            round3_status(play->round3, &play->hud);
        if (play->round4)
            round4_status(play->round4, &play->hud);
        if (play->round5)
            round5_status(play->round5, &play->hud);
        if (play->round6)
            round6_status(play->round6, &play->hud);
    }
    SDL_RenderTexture(renderer, play->picture, NULL, &dst);
    if (play->field) {
        SDL_FRect at;

        at.x = (float)play->field_x;
        at.y = (float)play->field_y;
        at.w = FIELD_W;
        at.h = FIELD_H;
        SDL_RenderTexture(renderer, play->field, NULL, &at);
        draw_hud(renderer, play->field_x, play->field_y, &play->hud);
    }
    between_draw(renderer, play->ui);
    if (play->paused) {
        SDL_FRect box;

        box.x = (float)play->field_x;
        box.y = (float)play->field_y;
        box.w = FIELD_W;
        box.h = FIELD_H;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &box);
        /* ResultItemProc draws string 150 at (0xD0, 0xAA), on the baseline. */
        text_draw(renderer,
                  (float)play->field_x + 0xD0,
                  (float)play->field_y + 0xAA - text_ascent(9, 1),
                  "Game Paused...", 9, 1, 255, 255, 255);
    }
}

long play_score(const Play *play)
{
    long score = 0;

    if (play)
        wave_info(play, NULL, &score, NULL);
    return score;
}

int play_round(const Play *play)
{
    return play ? play->nround : 0;
}

void play_free(Play *play)
{
    if (!play)
        return;
    between_free(play->ui);
    round1_free(play->round1);
    round2_free(play->round2);
    round3_free(play->round3);
    round4_free(play->round4);
    round5_free(play->round5);
    round6_free(play->round6);
    pic_free(play->field_pic);
    SDL_DestroyTexture(play->field);
    SDL_DestroyTexture(play->picture);
    SDL_free(play);
}
