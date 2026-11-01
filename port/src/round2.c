#include "round2.h"

#include "scene.h"
#include "sound.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

/* Matching source: src/round2.c (segment 28): Nerdoids.
   OpenRound2 (001C:0AF6)        round2_new
   ClickRound2 (001C:0B9D)       round2_click → fire
   TickRound2 (001C:0BA8)        round2_timer / round2_pump / once
   PaintRound2 (001C:0C54)       round2_paint
   CloseRound2 (001C:0C63)       round2_free
   BackRound2 (001C:0ADE)        field clear in play_draw
   ThrowRound2Comic (001C:095A)        fire
   SetupRound2 (001C:0460)       setup
   SpawnRound2Nerdoid (001C:058F)       spawn
   ArmRound2 (001C:0AC6)         arm
   LayRound2 (001C:09F0)         lay
   ReleaseRound2 (001C:073F)     release
   SpawnRound2Fall (001C:0169)   spawn_fall
   DropRound2Comic (001C:09B5)    drop_comic
   MakeRound2Comic (001C:08E9)    make_comic
   AddRound2Player (001C:07FC)   player setup in round2_new
   MoveRound2Player (001C:078A)  move_player
   MoveRound2Fall (001C:0101)    move_fall
   TickRound2Fall (001C:011E)    tick_fall
   HitRound2Fall (001C:0136)     hit_fall
   MoveRound2Nerdoid (001C:02B2)   move_nerdoid
   TickRound2Nerdoid (001C:035A)   tick_nerdoid
   HitRound2Nerdoid (001C:0398)    hit_nerdoid
   MoveRound2Comic (001C:0879)    move_comic
   TickRound2Comic (001C:089C)    tick_comic
   HitRound2Comic (001C:08B6)     hit_comic
   AddPlayAmmo / AddPlayScore    add_comics / add_score
     (001D:0144 / 001D:04B4)
   SetTimeLeft / FormatTime      round2_set_time / format_time
     (001D:0446 / 001D:0737)
   DrawStatusBar (001D:054F)     round2_status */
#include <time.h>

#define FIELD_W 0x200
#define CLIP_H 0x146
#define BANK 29

static const int k_left[16] = {
    5, 6, 7, 8, 12, 12, 12, 12, 10, 10, 10, 10, 14, 16, 20, 5
};
static const int k_max[16] = {
    2, 3, 3, 3, 4, 4, 5, 6, 3, 4, 5, 6, 4, 5, 6, 1
};
static const int k_bonus[16] = {
    30, 35, 40, 45, 30, 35, 40, 45, 35, 40, 45, 50, 40, 50, 55, 160
};
static const int k_lanes[16] = {
    5, 4, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 2, 2, 2, 4
};
static const int k_pts[16] = {
    10, 10, 10, 10, 12, 12, 12, 12, 15, 15, 15, 15, 18, 18, 18, 75
};
static const int k_vy[16] = {
    5, 5, 6, 6, 7, 8, 8, 9, 5, 6, 6, 6, 4, 5, 6, 0
};
static const int k_dx[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 5, 3, 4, 5, 3, 5, 5, 0
};
static const int k_shots[16] = {
    1, 1, 2, 2, 3, 3, 4, 4, 2, 2, 3, 3, 3, 4, 4, 0
};
static const int k_bomb_vy[16] = {
    10, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17
};
static const int k_speed[16] = {
    8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15
};

/* g_r2CmdL. SetupRound2 overwrites the long step with 4 and the bonus. */
static const uint8_t k_cmd_l[42] = {
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00,
    0x06, 0x02, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x00, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
    0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x0A, 0x00
};
static const uint8_t k_cmd_r[42] = {
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00,
    0x06, 0x02, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x00, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
    0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x0A, 0x00
};
static const uint8_t k_exit_l[14] = {
    0x05, 0x04, 0x03, 0x00, 0x04, 0x08, 0x01, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x0A, 0x00
};
static const uint8_t k_exit_r[14] = {
    0x05, 0x04, 0x03, 0x00, 0x04, 0x08, 0x01, 0x00, 0xF1, 0xFF, 0x00, 0x00, 0x0A, 0x00
};

struct Round2 {
    Scene *scene;
    Pic *bank[BANK];
    Sprite *player;
    uint8_t script_l[42];
    uint8_t script_r[42];
    int comics;
    int wave;
    int hold;
    int shots;
    int active;
    int gone;
    int left;
    int won;
    int dx_max;
    int shot_max;
    int bomb_vy;
    int max_out;
    int vy;
    int lanes;
    int pts;
    int speed;
    int bomb;
    int cursor;
    long score;
    long time_left;
    time_t time_last;
    int over;
    int clock_started;
    uint32_t clock;
    uint32_t next_tick;
    int scene_started;
    uint32_t scene_mark;
    uint32_t step_clock;
};

static void put_i16(uint8_t *p, int v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static uint8_t *skip_cmds(uint8_t *p, int n)
{
    while (n--)
        p += p[1];
    return p;
}

/* orig: AddPlayScore (001D:04B4) */
static void add_score(Round2 *r, long n)
{
    r->score += n;
    if (r->score < 0)
        r->score = 0;
}

/* orig: AddPlayAmmo (001D:0144) */
static void add_comics(Round2 *r, int n)
{
    r->comics += n;
    if (r->comics < 0)
        r->comics = 0;
}

static void move_comic(Sprite *s);
static void tick_comic(Sprite *s);
static void hit_comic(Sprite *s, Sprite *other, int first);
static Sprite *make_comic(Round2 *r);
static void drop_comic(Round2 *r, Sprite *parent);

/* orig: ArmRound2 (001C:0AC6) */
static void arm(Round2 *r)
{
    r->time_left = (long)r->wave * 10 + 0x2D;
    r->time_last = time(NULL);
}

/* orig: SetupRound2 (001C:0460) */
static void setup(Round2 *r)
{
    int n = r->wave;
    int bonus;
    uint8_t *lp;

    r->active = 0;
    r->gone = 0;
    if (n >= 16)
        n = (r->wave & 3) + 12;
    r->left = k_left[n];
    r->max_out = k_max[n];
    r->vy = k_vy[n];
    r->dx_max = k_dx[n];
    r->shot_max = k_shots[n];
    r->bomb_vy = k_bomb_vy[n];
    r->lanes = k_lanes[n];
    r->pts = k_pts[n];
    r->speed = k_speed[n];
    if (r->wave >= 16 && n != 15) {
        bonus = k_bonus[n] + ((r->wave - 16) / 4 + 1) * 10;
        if (bonus > 100)
            bonus = 100;
    } else
        bonus = k_bonus[n];
    SDL_memcpy(r->script_l, k_cmd_l, sizeof(r->script_l));
    SDL_memcpy(r->script_r, k_cmd_r, sizeof(r->script_r));
    lp = skip_cmds(r->script_l, 6);
    put_i16(lp + 2, 4);
    put_i16(lp + 4, bonus);
    lp = skip_cmds(r->script_r, 6);
    put_i16(lp + 2, 4);
    put_i16(lp + 4, -bonus);
}

/* orig: MoveRound2Player (001C:078A) */
static void move_player(Sprite *s)
{
    Round2 *r = s->user;
    int cursor = r->cursor;
    int limit = FIELD_W - r->bank[18]->w;

    if (cursor < 0)
        cursor = 0;
    else if (limit < cursor)
        cursor = limit;
    r->cursor = cursor;
    s->dx = cursor - s->left;
    if (s->left < cursor && s->dx > r->speed)
        s->dx = r->speed;
    else if (s->left > cursor && s->dx < -r->speed)
        s->dx = -r->speed;
}

/* orig: MoveRound2Fall (001C:0101) */
static void move_fall(Sprite *s)
{
    if (s->top >= CLIP_H) {
        s->visible = 0;
        s->dead = 1;
    }
}

/* orig: TickRound2Fall (001C:011E) */
static void tick_fall(Sprite *s)
{
    Round2 *r = s->user;

    if (s->dead)
        r->shots--;
}

/* orig: HitRound2Fall (001C:0136) */
static void hit_fall(Sprite *s, Sprite *other, int first)
{
    Round2 *r = s->user;

    (void)first;
    if (other->group != 1)
        return;
    s->visible = 0;
    s->dead = 1;
    add_comics(r, -5);
    sound_play(0x1770, 1);
}

/* orig: SpawnRound2Fall (001C:0169) */
static void spawn_fall(Round2 *r, Sprite *parent)
{
    Sprite *s;
    int n;
    int sw;
    int x;
    int y;

    if (r->shots >= r->shot_max)
        return;
    if (parent->group != 7 && parent->group != 5)
        return;
    if (!r->player || !r->bank[16] || !r->bank[17])
        return;
    n = rand() & 1;
    sw = r->bank[16 + n]->w;
    y = parent->bottom;
    x = parent->left + (parent->right - parent->left - sw) / 2;
    s = scene_insert(r->scene, r->scene->first, r->bank[16 + n], NULL, x, y, 0);
    if (!s)
        return;
    s->user = r;
    s->group = n + 3;
    s->dy = r->vy;
    if (r->dx_max != 0 && r->vy != 0) {
        int denom = (r->player->y - s->y + r->bank[18]->h / 2) / r->vy;

        s->dx = -(s->x - r->player->x + r->bank[18]->w / 2);
        if (denom != 0)
            s->dx /= denom;
        else
            s->dx = 0;
        if (s->dx > r->dx_max)
            s->dx = r->dx_max;
        else if (s->dx < -r->dx_max)
            s->dx = -r->dx_max;
    }
    s->hit_mask = 0x4000;
    s->on_moved = move_fall;
    s->on_hit = hit_fall;
    s->on_tick = tick_fall;
    r->shots++;
    sound_play(0x1B5A, 0);
}

/* orig: MakeRound2Comic (001C:08E9) */
static Sprite *make_comic(Round2 *r)
{
    Sprite *s;

    if (!r->bank[19])
        return NULL;
    s = scene_add(r->scene, r->bank[19], NULL, 0, 0, 0);
    if (!s)
        return NULL;
    s->user = r;
    s->group = 2;
    s->dy = -r->bomb_vy;
    s->hit_mask = 0x8000;
    s->on_moved = move_comic;
    s->on_hit = hit_comic;
    s->on_tick = tick_comic;
    r->bomb = 1;
    return s;
}

/* orig: MoveRound2Comic (001C:0879) */
static void move_comic(Sprite *s)
{
    if (s->bottom <= 0 || s->top >= CLIP_H) {
        s->visible = 0;
        s->dead = 1;
    }
}

/* orig: TickRound2Comic (001C:089C) */
static void tick_comic(Sprite *s)
{
    Round2 *r = s->user;

    if (s->dead)
        r->bomb = 0;
}

/* orig: HitRound2Comic (001C:08B6) */
static void hit_comic(Sprite *s, Sprite *other, int first)
{
    Round2 *r = s->user;

    (void)first;
    s->visible = 0;
    s->dead = 1;
    if (other->group == 1) {
        add_comics(r, 1);
        sound_play(0x1B5C, 0);
    }
}

/* orig: DropRound2Comic (001C:09B5) */
static void drop_comic(Round2 *r, Sprite *parent)
{
    Sprite *s = make_comic(r);

    if (!s || !r->bank[0])
        return;
    s->dy = -s->dy;
    s->y = parent->bottom + 1;
    s->x = parent->left + r->bank[0]->w / 2;
    sprite_sync(s);
}

/* orig: HitRound2Nerdoid (001C:0398) */
static void hit_nerdoid(Sprite *s, Sprite *other, int first)
{
    Round2 *r = s->user;

    (void)first;
    if (other->group != 2)
        return;
    if (s->group == 5 || s->group == 7) {
        r->gone++;
        s->group++;
        s->images = &r->bank[(s->group - 5) << 2];
        sprite_sync(s);
        add_score(r, r->pts);
        sound_play(0x1B59, 0);
    } else {
        r->gone--;
        s->group--;
        s->images = &r->bank[(s->group - 5) << 2];
        sprite_sync(s);
        sound_play(0x1B5B, 0);
        drop_comic(r, s);
    }
}

/* orig: MoveRound2Nerdoid (001C:02B2) */
static void move_nerdoid(Sprite *s)
{
    Round2 *r = s->user;

    if (s->group <= 6 && s->left > FIELD_W) {
        if (s->group == 6) {
            s->visible = 0;
            s->dead = 1;
        } else if (s->top == 0xEA) {
            r->won = 1;
            sound_play(0x232D, 1);
        } else {
            s->group += 2;
            s->y += 0x2C;
            sprite_set_script(s, r->script_r);
            s->images = &r->bank[8];
            s->frame = 0;
            s->frames = 4;
            sprite_sync(s);
        }
    } else if (s->group >= 7 && s->right < 0) {
        if (s->group == 8) {
            s->visible = 0;
            s->dead = 1;
        } else {
            s->group -= 2;
            s->y += 0x2C;
            sprite_set_script(s, r->script_l);
            s->images = &r->bank[0];
            s->frame = 0;
            s->frames = 4;
            sprite_sync(s);
        }
    }
}

/* orig: TickRound2Nerdoid (001C:035A) */
static void tick_nerdoid(Sprite *s)
{
    Round2 *r = s->user;

    if (s->dead) {
        r->active--;
        r->left--;
        r->gone--;
    } else if (s->left >= 0 && s->right <= FIELD_W) {
        if ((rand() & 7) == 0)
            spawn_fall(r, s);
    }
}

/* orig: SpawnRound2Nerdoid (001C:058F) */
static void spawn(Round2 *r)
{
    unsigned char lane[4];
    int span;
    int n;
    int pick;
    Sprite *s;

    if (!r->bank[0] || !r->bank[3] || !r->bank[8])
        return;
    if (r->active - r->gone >= r->max_out)
        return;
    if (r->left <= r->active || r->comics == 0)
        return;
    lane[0] = lane[1] = lane[2] = lane[3] = 0xFF;
    span = r->bank[3]->w * 3;
    for (s = r->scene->first; s; s = s->next) {
        int top;
        int slot;

        if (s->group < 5 || s->group > 8)
            continue;
        top = s->top;
        if (s->right <= span) {
            if (top <= 0x92) {
                slot = ((top - 0x3A) / 0x2C + 1) & ~1;
                if (slot >= 0 && slot < 4)
                    lane[slot] = 0;
            }
        } else if (s->left >= FIELD_W - span && top <= 0xBE) {
            slot = (top - 0x3A) / 0x2C | 1;
            if (slot >= 0 && slot < 4)
                lane[slot] = 0;
        }
    }
    if ((lane[0] | lane[1] | lane[2] | lane[3]) == 0)
        return;
    n = 5 - r->lanes;
    do {
        if (lane[n] != 0)
            break;
    } while (n-- != 0);
    if (n < 0)
        return;
    do
        pick = (rand() & 0x7FFF) % (6 - r->lanes);
    while (!lane[pick]);
    if (pick & 1)
        s = scene_add(r->scene, r->bank[8], NULL, FIELD_W, (pick & ~1) * 0x2C + 0x66, 0);
    else
        s = scene_add(r->scene, r->bank[0], NULL, -0x1E, (pick & ~1) * 0x2C + 0x3A, 0);
    if (!s)
        return;
    s->user = r;
    s->frames = 4;
    if (pick & 1) {
        sprite_set_script(s, r->script_r);
        s->images = &r->bank[8];
        s->group = 7;
    } else {
        sprite_set_script(s, r->script_l);
        s->images = &r->bank[0];
        s->group = 5;
    }
    s->frame = 0;
    sprite_sync(s);
    s->hit_mask = 0x8000;
    s->on_move = move_nerdoid;
    s->on_tick = tick_nerdoid;
    s->on_hit = hit_nerdoid;
    r->active++;
}

/* orig: ReleaseRound2 (001C:073F) */
static void release(Round2 *r)
{
    Sprite *s;

    if (r->left != r->gone)
        return;
    for (s = r->scene->first; s; s = s->next) {
        if (s->group != 6 && s->group != 8)
            continue;
        s->hit_mask = 0;
        if (s->group == 8)
            sprite_set_script(s, k_exit_r);
        else
            sprite_set_script(s, k_exit_l);
    }
}

/* orig: LayRound2 (001C:09F0) */
static void lay(Round2 *r)
{
    int y;
    int i;

    y = 0;
    while (y < 0x108) {
        if (r->bank[20])
            scene_add(r->scene, r->bank[20], NULL, 0, y + 0x20, 0);
        y += 0x2C;
    }
    for (i = 0; i <= 2; i++) {
        if (r->bank[22 + i])
            scene_add(r->scene, r->bank[22 + i], NULL, 4, i * 0x58 + 0x28, 0);
        if (r->bank[26 + i]) {
            int rx = 0x1CF;
            int ry = i * 0x58 + 0x54;

            if (i == 2) {
                rx = 0x1A1;
                ry -= 0x2C;
            }
            scene_add(r->scene, r->bank[26 + i], NULL, rx, ry, 0);
        }
    }
}

/* orig: ThrowRound2Comic (001C:095A) */
static void throw_comic(Round2 *r)
{
    Sprite *s;

    if (r->bomb != 0 || r->comics == 0 || !r->player || !r->bank[19] || !r->bank[18])
        return;
    s = make_comic(r);
    if (!s)
        return;
    s->y = r->player->top - r->bank[19]->h - 1;
    s->x = r->player->left + (r->bank[18]->w - r->bank[19]->w) / 2;
    sprite_sync(s);
    add_comics(r, -1);
    sound_play(0x1B58, 0);
}

int round2_holding(const Round2 *round)
{
    return round && round->hold;
}

void round2_wave_info(const Round2 *round, int *wave, long *score, long *time_left)
{
    if (!round)
        return;
    if (wave)
        *wave = round->wave + 1;
    if (score)
        *score = round->score;
    if (time_left)
        *time_left = round->time_left;
}

void round2_add_score(Round2 *round, long n)
{
    if (round)
        add_score(round, n);
}

/* orig: SetTimeLeft (001D:0446) */
void round2_set_time(Round2 *round, long time_left)
{
    if (!round)
        return;
    round->time_left = time_left;
    if (round->time_left < 0)
        round->time_left = 0;
    round->time_last = time(NULL);
}

/* orig: TickTimeLeft (001D:03AC) / PauseTimeLeft (001D:0425) */
void round2_tick_time(Round2 *round)
{
    time_t now;

    if (!round)
        return;
    now = time(NULL);
    round->time_left -= (long)(now - round->time_last);
    if (round->time_left < 0)
        round->time_left = 0;
    round->time_last = now;
}

void round2_add_supply(Round2 *round, int n)
{
    if (round)
        add_comics(round, n);
}

/* orig: NextPlayWave + ArmRound2 + SetupRound2 after RunStepDialog */
void round2_finish_wave(Round2 *round)
{
    if (!round)
        return;
    round->hold = 0;
    round->wave++;
    if (round->wave > 98)
        round->wave = 98;
    arm(round);
    setup(round);
    spawn(round);
}

/* orig: TickRound2 lose / hold / TickScene catch-up (001C:0BA8) */
static int once(Round2 *r, uint32_t now)
{
    time_t wall;
    int steps;

    if (r->over)
        return 1;
    if (r->hold)
        return 0;
    release(r);
    if (r->bomb == 0 && r->active == 0 && r->shots == 0 && r->left == 0) {
        r->hold = 1;
        return 0;
    }
    if (now >= r->next_tick) {
        steps = 1;
        if (r->scene_started) {
            steps = (int)(now - r->scene_mark) / 3;
            if (steps < 1)
                steps = 1;
            else if (steps > 3)
                steps = 3;
        }
        r->scene_started = 1;
        r->scene_mark = now;
        spawn(r);
        r->next_tick = now + 3;
        scene_run(r->scene, steps);
    }
    wall = time(NULL);
    r->time_left -= (long)(wall - r->time_last);
    if (r->time_left < 0)
        r->time_left = 0;
    r->time_last = wall;
    if ((r->comics == 0 && r->bomb == 0) || r->won)
        r->over = 1;
    return r->over;
}

static int load_bank(const char *resources, Pic **bank)
{
    char path[1024];
    size_t nbytes = 0;
    void *file;
    int i;

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/7000.pic", resources);
    file = SDL_LoadFile(path, &nbytes);
    if (!file)
        return 0;
    for (i = 0; i < BANK; i++) {
        bank[i] = pic_decode_ppic(file, nbytes, i);
        /* DecImageBottoms trims one row off the nerdoid frames. */
        if (i <= 15 && bank[i] && bank[i]->h > 1)
            bank[i]->h--;
    }
    SDL_free(file);
    return bank[18] != NULL;
}

/* orig: OpenRound2 (001C:0AF6) */
Round2 *round2_new(const char *resources)
{
    Round2 *r = SDL_calloc(1, sizeof(*r));

    if (!r)
        return NULL;
    srand((unsigned)time(NULL));
    r->comics = 20;
    if (!load_bank(resources, r->bank)) {
        round2_free(r);
        return NULL;
    }
    r->scene = scene_new(3);
    if (!r->scene) {
        round2_free(r);
        return NULL;
    }
    r->player = scene_add(r->scene, r->bank[18], NULL,
                          (FIELD_W - r->bank[18]->w) / 2, 0x144 - r->bank[18]->h, 0);
    if (!r->player) {
        round2_free(r);
        return NULL;
    }
    r->player->user = r;
    r->player->group = 1;
    r->player->hit_mask = 0xC000;
    r->player->on_move = move_player;
    r->cursor = r->player->left;
    sound_preload(0x1770, 1);
    sound_preload(0x232D, 1);
    sound_preload(0x1B58, 5);
    lay(r);
    arm(r);
    setup(r);
    return r;
}

/* orig: TickRound2 LOWORD(GetMessagePos) read (001C:0BA8) */
void round2_set_cursor(Round2 *round, int screen_x)
{
    if (round && !round->over)
        round->cursor = screen_x;
}

/* orig: ClickRound2 (001C:0B9D) */
void round2_click(Round2 *round)
{
    if (round && !round->over)
        throw_comic(round);
}

/* orig: TickRound2 (001C:0BA8) */
int round2_timer(Round2 *round)
{
    if (!round)
        return 0;
    round->step_clock++;
    return once(round, round->step_clock);
}

/* orig: TickScene timing prelude (0004:00D9) via GetTicks/17 */
int round2_pump(Round2 *round)
{
    uint32_t now;

    if (!round || round->over)
        return round && round->over;
    now = (uint32_t)(SDL_GetTicks() / 17);
    if (!round->clock_started) {
        round->clock_started = 1;
        round->clock = now;
        return once(round, now);
    }
    if ((int)(now - round->clock) <= 0)
        return 0;
    round->clock = now;
    return once(round, now);
}

/* orig: PaintRound2 (001C:0C54) */
void round2_paint(const Round2 *round, Pic *field)
{
    if (round)
        scene_paint(round->scene, field, FIELD_W, CLIP_H);
}

/* orig: FormatTime (001D:0737) */
static void format_time(long t, char *dst)
{
    int n;

    if (t < 0)
        t = 0;
    n = (int)(t / 600);
    t -= (long)n * 600;
    if (n > 9)
        n = 9;
    dst[0] = (char)('0' + n);
    n = (int)(t / 60);
    t -= (long)n * 60;
    dst[1] = (char)('0' + n);
    dst[2] = ':';
    n = (int)(t / 10);
    t -= (long)n * 10;
    dst[3] = (char)('0' + n);
    dst[4] = (char)('0' + (int)t);
    dst[5] = '\0';
}

/* orig: DrawStatusBar round 2 columns (001D:054F) */
void round2_status(const Round2 *round, PlayHud *hud)
{
    if (!hud)
        return;
    SDL_zerop(hud);
    if (!round)
        return;
    hud->kind = 0;
    hud->left_label = "Comics:";
    hud->left_x = 0x20;
    SDL_snprintf(hud->left_value, sizeof(hud->left_value), "%03d", round->comics);
    SDL_snprintf(hud->wave, sizeof(hud->wave), "%02d", round->wave + 1);
    format_time(round->time_left, hud->mid_value);
    hud->score = round->score;
}

/* orig: CloseRound2 (001C:0C63) */
void round2_free(Round2 *round)
{
    int i;

    if (!round)
        return;
    scene_free(round->scene);
    for (i = 0; i < BANK; i++)
        pic_free(round->bank[i]);
    SDL_free(round);
}
