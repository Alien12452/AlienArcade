#include "round1.h"

#include "scene.h"
#include "sound.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

/* Matching source: src/round1.c (segment 30): Stink Snakes.
   OpenRound1 (001E:06A3)        round1_new
   ClickRound1 (001E:073A)       round1_click → fire
   TickRound1 (001E:0745)        round1_timer / round1_pump / lost
   PaintRound1 (001E:07C4)       round1_paint
   CloseRound1 (001E:07D3)       round1_free
   BackRound1 (001E:0673)        field clear in play_draw
   ThrowRound1Pie (001E:022C)        fire
   SetupRound1 (001E:02C1)       setup
   SpawnRound1Snake (001E:053E)       spawn
   ArmRound1 (001E:068B)         arm
   AddRound1Player (001E:016B)   player setup in round1_new
   SetRound1Move (001E:006B)     sprite_set_script callers
   SetRound1Frame (001E:00BB)    frame/image setup
   MoveRound1Player (001E:0104)  move_player
   MoveRound1Pie (001E:01B5)    move_pie
   TickRound1Pie (001E:01E2)    tick_pie
   HitRound1Pie (001E:01FA)     hit_pie
   MoveRound1Snake (001E:03A4)   move_snake
   TickRound1Snake (001E:03D8)   tick_snake
   HookRound1Happy (001E:03F9)        hook_happy
   HitRound1Snake (001E:0462)    hit_snake
   AddPlayAmmo / AddPlayScore    add_pies / add_score
     (001D:0144 / 001D:04B4)
   SetTimeLeft / FormatTime      round1_set_time / format_time
     (001D:0446 / 001D:0737)
   DrawStatusBar (001D:054F)     round1_status */
#include <time.h>

#define FIELD_W 0x200
#define CLIP_H 0x146
#define BANK 26

/* Level rows, ten entries. Past wave 10, SetupRound1 folds the index. */
static const int k_vx[10] = { 20, 15, 10, 10, 5, 15, 10, 5, 5, 5 };
static const int k_vy[10] = { 40, 40, 40, 45, 45, 45, 50, 50, 50, 55 };
static const int k_span[10] = { 0, 0, 0, 0, 0, 15, 15, 20, 20, 25 };
static const int k_left[10] = { 10, 10, 10, 15, 15, 15, 20, 20, 20, 25 };
static const int k_max[10] = { 5, 5, 5, 10, 10, 10, 10, 10, 15, 25 };
static const int k_speed[10] = { -10, -10, -10, -10, -15, -15, -15, -20, -20, -20 };

/* g_r1Cmd. SetupRound1 writes the two long swoops over the 0x63 steps. */
static const uint8_t k_cmd[122] = {
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x09, 0x04,
    0x01, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x04, 0x08,
    0x01, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x04, 0x08, 0x63, 0x00, 0x9D, 0xFF, 0x63, 0x00, 0x07, 0x02,
    0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
    0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x01, 0x00, 0x06, 0x02,
    0x04, 0x08, 0x01, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xF9, 0xFF,
    0x00, 0x00, 0x04, 0x08, 0x63, 0x00, 0x9D, 0xFF, 0x9D, 0xFF, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
    0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x0A, 0x00
};

/* g_r1Cmd2 then g_r1Tail. The splat script reads past the first array. */
static const uint8_t k_cmd2[32] = {
    0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00,
    0x0A, 0x00, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x00, 0x00
};

struct Round1 {
    Scene *scene;
    Pic *bank[BANK];
    Sprite *player;
    uint8_t script[122];
    uint8_t script2[32];
    int pies;
    int wave;
    int hold;
    int shots;
    int active;
    int gone;
    int left;
    int span;
    int speed;
    int max_snakes;
    int cursor;
    long score;
    long time_left;
    time_t time_last;
    int over;
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
static void add_score(Round1 *r, long n)
{
    r->score += n;
    if (r->score < 0)
        r->score = 0;
}

/* orig: AddPlayAmmo (001D:0144) */
static void add_pies(Round1 *r, int n)
{
    r->pies += n;
    if (r->pies < 0)
        r->pies = 0;
}

/* orig: ArmRound1 (001E:068B) */
static void arm(Round1 *r)
{
    r->time_left = (long)r->wave * 5 + 0x1E;
    r->time_last = time(NULL);
}

/* orig: SetupRound1 (001E:02C1) */
static void setup(Round1 *r)
{
    int n = r->wave;
    uint8_t *lp;

    r->active = 0;
    r->gone = 0;
    if (n >= 10)
        n = (n & 3) + 6;
    r->span = k_span[n];
    r->left = k_left[n];
    r->max_snakes = k_max[n];
    r->speed = k_speed[n];
    SDL_memcpy(r->script, k_cmd, sizeof(r->script));
    lp = skip_cmds(r->script, 8);
    put_i16(lp + 2, k_vx[n]);
    put_i16(lp + 4, -k_vy[n]);
    put_i16(lp + 6, r->span);
    lp = skip_cmds(lp, 14);
    put_i16(lp + 2, k_vx[n]);
    put_i16(lp + 4, -k_vy[n]);
    put_i16(lp + 6, -r->span);
}

/* orig: MoveRound1Player (001E:0104) */
static void move_player(Sprite *s)
{
    Round1 *r = s->user;
    int cursor = r->cursor;
    int limit = 0x141 - r->bank[0]->h;

    if (cursor < 5)
        cursor = 5;
    else if (limit < cursor)
        cursor = limit;
    r->cursor = cursor;
    s->dy = cursor - s->top;
    if (s->top < cursor && s->dy > 0x14)
        s->dy = 0x14;
    else if (s->top > cursor && s->dy < -0x14)
        s->dy = -0x14;
}

/* orig: MoveRound1Pie (001E:01B5) */
static void move_pie(Sprite *s)
{
    if (s->left >= FIELD_W)
        s->dx = -s->dx;
    else if (s->right <= 0) {
        s->visible = 0;
        s->dead = 1;
    }
}

/* orig: TickRound1Pie (001E:01E2) */
static void tick_pie(Sprite *s)
{
    Round1 *r = s->user;

    if (s->dead)
        r->shots--;
}

/* orig: HitRound1Pie (001E:01FA) */
static void hit_pie(Sprite *s, Sprite *other, int first)
{
    Round1 *r = s->user;

    (void)first;
    s->dead = 1;
    s->visible = 0;
    if (other == r->player) {
        add_pies(r, 1);
        sound_play(0x1773, 0);
    }
}

/* orig: HookRound1Happy (001E:03F9) */
static int hook_happy(int n, Sprite *s, MoveState *pm, int ready)
{
    Round1 *r = s->user;

    (void)n;
    (void)pm;
    (void)ready;
    s->move_ex = NULL;
    s->group = 5;
    s->dx = r->speed;
    s->anim_rate = 2;
    s->frame = 0;
    s->images = &r->bank[9];
    s->masks = &r->bank[22];
    sprite_sync(s);
    return 0;
}

/* orig: MoveRound1Snake (001E:03A4) */
static void move_snake(Sprite *s)
{
    Round1 *r = s->user;

    if (s->right <= 0) {
        s->visible = 0;
        s->dead = 1;
        if (s->group == 3) {
            add_pies(r, -5);
            sound_play(0x1770, 1);
        }
    }
}

/* orig: TickRound1Snake (001E:03D8) */
static void tick_snake(Sprite *s)
{
    Round1 *r = s->user;

    if (!s->dead)
        return;
    r->active--;
    if (s->group != 3)
        r->gone--;
}

/* orig: HitRound1Snake (001E:0462) */
static void hit_snake(Sprite *s, Sprite *other, int first)
{
    Round1 *r = s->user;

    (void)first;
    if (other->group == 1) {
        if (s->group == 5) {
            s->visible = 0;
            s->dead = 1;
            add_score(r, 10);
            sound_play(0x1774, 0);
        }
        return;
    }
    if (other->group != 2)
        return;
    add_score(r, 1);
    r->gone++;
    r->left--;
    sprite_set_script(s, r->script2);
    if (s->move)
        s->move->hook = hook_happy;
    s->hit_mask = 0x20000000u;
    s->group = 4;
    s->frame = 0;
    s->images = &r->bank[6];
    s->masks = &r->bank[19];
    sprite_sync(s);
    sound_play(0x1772, 0);
}

/* orig: SpawnRound1Snake (001E:053E) */
static void spawn(Round1 *r)
{
    int half;
    int tries;
    int blocked;
    int y;
    Sprite *s;

    if (!r->bank[2])
        return;
    if (r->active != 0 && (rand() & 0x7FFF) % 16 != 0)
        return;
    if (r->active >= r->max_snakes || r->left <= r->active || r->pies == 0)
        return;
    half = r->bank[2]->h;
    if (0x13C - half - r->span <= 0)
        return;
    blocked = 1;
    tries = 0x15;
    y = 5;
    while (blocked && tries-- != 0) {
        blocked = 0;
        y = (rand() & 0x7FFF) % (0x13C - half - r->span) + 5;
        for (s = r->scene->first; s; s = s->next) {
            if (s->group == 3 &&
                s->right >= FIELD_W - r->bank[2]->w + -11 &&
                s->top - half < y && s->bottom + half > y) {
                blocked = 1;
                break;
            }
        }
    }
    if (blocked)
        return;
    s = scene_add(r->scene, r->bank[2], r->bank[15], 0x1F6, y, -1);
    if (!s)
        return;
    s->user = r;
    s->images = &r->bank[2];
    s->masks = &r->bank[15];
    s->frame = 0;
    s->frames = 4;
    s->anim_mode = 1;
    sprite_sync(s);
    sprite_set_script(s, r->script);
    s->group = 3;
    s->hit_mask = 0xC0000000u;
    s->on_moved = move_snake;
    s->on_hit = hit_snake;
    s->on_tick = tick_snake;
    r->active++;
}

/* orig: ThrowRound1Pie (001E:022C) */
static void throw_pie(Round1 *r)
{
    Sprite *s;

    if (r->shots >= 5 || r->pies == 0 || r->active == 0 || !r->player)
        return;
    s = scene_add(r->scene, r->bank[1], r->bank[14],
                  r->player->right + 1, r->player->top + 0xC, -1);
    if (!s)
        return;
    s->user = r;
    s->group = 2;
    s->dx = 0x14;
    s->hit_mask = 0x40000000u;
    s->on_moved = move_pie;
    s->on_hit = hit_pie;
    s->on_tick = tick_pie;
    r->shots++;
    add_pies(r, -1);
    sound_play(0x1771, 0);
}

/* orig: TickRound1 lose test (001E:0745) */
static int lost(const Round1 *r)
{
    return r->pies <= 0 && r->shots <= 0 && r->gone <= 0 &&
           (r->left != 0 || r->active != 0);
}

int round1_holding(const Round1 *round)
{
    return round && round->hold;
}

void round1_wave_info(const Round1 *round, int *wave, long *score, long *time_left)
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

void round1_add_score(Round1 *round, long n)
{
    if (round)
        add_score(round, n);
}

/* orig: SetTimeLeft (001D:0446) */
void round1_set_time(Round1 *round, long time_left)
{
    if (!round)
        return;
    round->time_left = time_left;
    if (round->time_left < 0)
        round->time_left = 0;
    round->time_last = time(NULL);
}

/* orig: TickTimeLeft (001D:03AC) / PauseTimeLeft (001D:0425) */
void round1_tick_time(Round1 *round)
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

void round1_add_supply(Round1 *round, int n)
{
    if (round)
        add_pies(round, n);
}

/* orig: NextPlayWave + ArmRound1 + SetupRound1 after RunStepDialog */
void round1_finish_wave(Round1 *round)
{
    if (!round)
        return;
    round->hold = 0;
    round->wave++;
    if (round->wave > 98)
        round->wave = 98;
    arm(round);
    setup(round);
}

static Pic **load_bank(const char *resources, Pic **bank)
{
    char path[1024];
    size_t nbytes = 0;
    void *file;
    int i;

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/6000.pic", resources);
    file = SDL_LoadFile(path, &nbytes);
    if (!file)
        return NULL;
    for (i = 0; i < BANK; i++)
        bank[i] = pic_decode_ppic(file, nbytes, i);
    SDL_free(file);
    return bank;
}

/* orig: OpenRound1 (001E:06A3) */
Round1 *round1_new(const char *resources)
{
    Round1 *r = SDL_calloc(1, sizeof(*r));

    if (!r)
        return NULL;
    srand((unsigned)time(NULL));
    SDL_memcpy(r->script2, k_cmd2, sizeof(r->script2));
    r->pies = 20;
    r->cursor = 0xA3;
    if (!load_bank(resources, r->bank) || !r->bank[0] || !r->bank[13]) {
        round1_free(r);
        return NULL;
    }
    r->scene = scene_new(2);
    if (!r->scene) {
        round1_free(r);
        return NULL;
    }
    r->player = scene_add(r->scene, r->bank[0], r->bank[13], 5, 0xA3, -1);
    if (!r->player) {
        round1_free(r);
        return NULL;
    }
    r->player->user = r;
    r->player->group = 1;
    r->player->hit_mask = 0xE0000000u;
    r->player->on_move = move_player;
    sound_preload(0x1770, 5);
    arm(r);
    setup(r);
    return r;
}

/* orig: TickRound1 HIWORD(GetMessagePos) read (001E:0745) */
void round1_set_cursor(Round1 *round, int screen_y)
{
    if (round && !round->over)
        round->cursor = screen_y;
}

/* orig: ClickRound1 (001E:073A) */
void round1_click(Round1 *round)
{
    if (round && !round->over)
        throw_pie(round);
}

/* orig: TickRound1 (001E:0745) */
int round1_timer(Round1 *round, int scene_steps)
{
    time_t now;

    if (!round || round->over || scene_steps < 1)
        return round && round->over;
    if (round->hold)
        return 0;
    spawn(round);
    if (round->shots <= 0 && round->left <= 0 && round->active <= 0 && round->gone <= 0) {
        round->hold = 1;
        return 0;
    }
    scene_run(round->scene, scene_steps);
    now = time(NULL);
    round->time_left -= (long)(now - round->time_last);
    if (round->time_left < 0)
        round->time_left = 0;
    round->time_last = now;
    if (lost(round))
        round->over = 1;
    return round->over;
}

/* orig: TickScene timing prelude (0004:00D9) via scene_due */
int round1_pump(Round1 *round)
{
    int steps;

    if (!round || round->over)
        return round && round->over;
    steps = scene_due(round->scene);
    if (steps < 1)
        return 0;
    return round1_timer(round, steps);
}

/* orig: PaintRound1 (001E:07C4) */
void round1_paint(const Round1 *round, Pic *field)
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

/* orig: DrawStatusBar round 1 columns (001D:054F) */
void round1_status(const Round1 *round, PlayHud *hud)
{
    if (!hud)
        return;
    SDL_zerop(hud);
    if (!round)
        return;
    hud->kind = 0;
    hud->left_label = "Pies:";
    hud->left_x = 0x33;
    SDL_snprintf(hud->left_value, sizeof(hud->left_value), "%03d", round->pies);
    SDL_snprintf(hud->wave, sizeof(hud->wave), "%02d", round->wave + 1);
    format_time(round->time_left, hud->mid_value);
    hud->score = round->score;
}

/* orig: CloseRound1 (001E:07D3) */
void round1_free(Round1 *round)
{
    int i;

    if (!round)
        return;
    scene_free(round->scene);
    for (i = 0; i < BANK; i++)
        pic_free(round->bank[i]);
    SDL_free(round);
}
