#include "round6.h"

#include "scene.h"
#include "sound.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

/* Matching source: src/round6.c (segment 33): Quasar Rain.
   OpenRound6 (0021:099B)        round6_new
   ClickRound6 (0021:0A6E)       (bumper is cursor-driven; click unused)
   TickRound6 (0021:0A75)        round6_timer / round6_pump / once
   PaintRound6 (0021:0B53)       round6_paint
   CloseRound6 (0021:0B62)       round6_free
   BackRound6 (0021:0983)        field clear in play_draw
   SetupRound6 (0021:08C5)       setup
   ArmRound6 (0021:095E)         arm
   LayRound6 (0021:0891)         lay
   SpawnRound6 (0021:0677)       spawn
   BounceRound6Klutz (0021:0443)      bounce_klutz
   MoveRound6MomETron (0021:02DF)  move_mom_e_tron
   AddRound6MomETron (0021:032A)   player setup in round6_new
   HitTestRound6MomE (0021:0286)     hit_test_mom_e
   HitRound6Klutz (0021:0626)    hit_klutz
   HitRound6Peel (0021:0648)     hit_peel
   MovedRound6 (0021:05FE)       moved_faller
   HookRound6 (0021:01FA)        script_hook
   ShrinkRound6Bottom (0021:096E) bank h-- in round6_new
   ClearRound6Marks (0021:01BF)  marks init in round6_new
   AllocRound6Mark (0021:01DA)   alloc_mark
   Find/FreeRound6Slot           find_slot / free_slot
     (0021:03D3 / 0021:03FE)
   SetRound6Frame (0021:0156)    set_frame
   AddPlayAmmo / AddPlayScore    saves / add_score
     (001D:0144 / 001D:04B4)
   SetTimeLeft / FormatTime      round6_set_time / format_time
     (001D:0446 / 001D:0737)
   DrawStatusBar (001D:054F)     round6_status */
#include <time.h>

#define FIELD_W 0x200
#define CLIP_H 0x146
#define BANK 26
#define SLOTS 16

static const int k_count[10] = { 16, 16, 16, 16, 18, 20, 22, 24, 26, 28 };
static const int k_mod[10] = { 14, 12, 11, 10, 12, 12, 12, 12, 12, 10 };
static const int k_spread[10] = { 40, 40, 40, 50, 50, 50, 50, 60, 60, 60 };
static const int k_range[10] = { 200, 200, 200, 100, 200, 300, 400, 500, 500, 500 };
static const int k_dy[10] = { 8, 10, 13, 16, 20, 24, 24, 26, 28, 30 };
static const int k_delay[10] = { 60, 56, 52, 30, 28, 26, 24, 22, 20, 18 };

/* g_r6Cmd. BounceRound6Klutz patches the 0x63 moves; the tail is the hop into bed. */
static const uint8_t k_cmd[82] = {
    0x00, 0x02, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00,
    0x63, 0x00, 0x00, 0x00, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x06, 0x02, 0x04, 0x08,
    0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x06, 0x02, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00,
    0x04, 0x08, 0x04, 0x00, 0xEC, 0xFF, 0x18, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x51, 0x46, 0x09, 0x04,
    0x04, 0x00, 0x04, 0x08, 0x0C, 0x00, 0xC4, 0xFF, 0x00, 0x00, 0x01, 0x02, 0x0C, 0x04, 0xFD, 0xFF,
    0x0E, 0x02
};

static const uint8_t k_idle[8] = { 0x00, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0A, 0x00 };

struct Round6;

typedef struct Slot {
    struct Round6 *round;
    Sprite *sprite;
    uint8_t script[128];
    int *mark;
} Slot;

struct Round6 {
    Pic *bank[BANK];
    Scene *scene;
    Sprite *player;
    Slot slots[SLOTS];
    int marks[SLOTS];
    int cursor;
    int saved;
    int level;
    int mod;
    int spread;
    int range;
    int dy;
    int delay;
    int left;
    int active;
    int aim;
    int gone;
    int saves;
    int wave;
    int hold;
    uint32_t hold_now;
    long score;
    long time_left;
    time_t time_last;
    int over;
    int clock_started;
    int scene_started;
    uint32_t clock;
    uint32_t step_clock;
    uint32_t gate;
    uint32_t next_spawn;
    uint32_t scene_mark;
};

static int script_hook(int n, Sprite *s, MoveState *pm, int ready);

static void put_i16(uint8_t *p, int v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static uint8_t *skip_cmd(uint8_t *ip)
{
    return ip + ip[1];
}

static int rnd_mod(int n)
{
    if (n <= 0)
        return 0;
    return (int)((unsigned)rand() % (unsigned)n);
}

/* orig: AddPlayScore (001D:04B4) — grants a save at each 5000 boundary */
static void add_score(Round6 *r, long n)
{
    long wrapped = r->score % 5000;

    if (wrapped + n >= 5000) {
        r->saves++;
        sound_play(0x4653, 1);
    }
    r->score += n;
    if (r->score < 0)
        r->score = 0;
}

/* orig: AllocRound6Mark (0021:01DA) */
static int *alloc_mark(Round6 *r)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (r->marks[i] < 0) {
            r->marks[i] = 0;
            return &r->marks[i];
        }
    }
    return NULL;
}

/* orig: FindRound6Slot (0021:03D3) */
static Slot *find_slot(Round6 *r)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (r->slots[i].sprite == NULL)
            return &r->slots[i];
    }
    return NULL;
}

static Slot *slot_for(Round6 *r, Sprite *s)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (r->slots[i].sprite == s)
            return &r->slots[i];
    }
    return NULL;
}

/* orig: FreeRound6Slot (0021:03FE) */
static void free_slot(Round6 *r, Sprite *s)
{
    Slot *slot = slot_for(r, s);

    if (slot)
        slot->sprite = NULL;
}

/* orig: SetRound6Frame (0021:0156) */
static void set_frame(Sprite *s, Pic **images, Pic **masks, int frame, int frames)
{
    s->images = images;
    s->masks = masks;
    s->frame = frame;
    s->frames = frames;
    sprite_sync(s);
}

/* orig: HitTestRound6MomE (0021:0286) — bumper top is a slope; klutz above falls past */
static int hit_test_mom_e(Sprite *s, Sprite *other, int first)
{
    int right;
    int bottom;

    if (!first)
        return 0;
    right = other->right;
    bottom = other->bottom;
    if (s->left < right && s->right > right && s->top < bottom &&
        s->top + s->right - right > bottom)
        return 0;
    return 1;
}

/* orig: MoveRound6MomETron (0021:02DF) */
static void move_mom_e_tron(Sprite *s)
{
    Round6 *r = s->user;

    s->dx = (r->cursor - r->saved) << 2;
    r->saved = r->cursor;
    if (s->dx + s->right > FIELD_W)
        s->dx = FIELD_W - s->right;
    else if (s->dx + s->left < 0x43)
        s->dx = 0x43 - s->left;
}

/* orig: MovedRound6 (0021:05FE) */
static void moved_faller(Sprite *s)
{
    Round6 *r = s->user;

    if (s->y <= CLIP_H)
        return;
    s->dead = 1;
    if (s->group == 4)
        return;
    r->active--;
    r->gone = 1;
}

/* orig: BounceRound6Klutz (0021:0443) */
static void bounce_klutz(Round6 *r, Sprite *s)
{
    Slot *slot;
    int n_wide;
    int x;
    int y;
    int row;
    int col;
    uint8_t *ip;

    sound_play(0x4650, 0);
    n_wide = s->group == 2 ? 0x0E : 0x19;
    x = s->x - n_wide - 36;
    row = 3 - (s->x - 67) / 0x6F;
    y = s->y - (row * 0x48 - 3);
    col = x >> 6;
    if (s->group == 2)
        y -= 0x0E;
    else
        y -= 0x16;
    s->hit_mask = 0;
    if (s->group == 2)
        set_frame(s, &r->bank[0], &r->bank[10], 0, 10);
    else
        set_frame(s, &r->bank[5], &r->bank[15], 0, 10);
    slot = find_slot(r);
    if (!slot)
        return;
    SDL_memcpy(slot->script, k_cmd, sizeof(k_cmd));
    slot->sprite = s;
    slot->mark = alloc_mark(r);
    ip = skip_cmd(slot->script);
    put_i16(ip + 2, col);
    put_i16(ip + 4, -(x >> 2));
    put_i16(ip + 6, -(y >> 1));
    x -= x >> 2;
    y -= y >> 1;
    ip = skip_cmd(ip);
    ip = skip_cmd(ip);
    put_i16(ip + 4, -n_wide);
    ip = skip_cmd(ip);
    put_i16(ip + 2, col);
    put_i16(ip + 4, -(x / 3));
    put_i16(ip + 6, -(y >> 1));
    x -= x / 3;
    y = y - (y >> 1);
    ip = skip_cmd(ip);
    ip = skip_cmd(ip);
    put_i16(ip + 2, col);
    put_i16(ip + 4, -(x >> 1));
    put_i16(ip + 6, -(y >> 1));
    x -= x >> 1;
    y -= y >> 1;
    ip = skip_cmd(ip);
    ip = skip_cmd(ip);
    put_i16(ip + 2, col);
    put_i16(ip + 4, -x);
    put_i16(ip + 6, -y);
    sprite_set_script(s, slot->script);
    if (s->move)
        s->move->hook = script_hook;
}

/* orig: HitRound6Klutz (0021:0626) */
static void hit_klutz(Sprite *s, Sprite *other, int first)
{
    Round6 *r = s->user;

    (void)other;
    (void)first;
    bounce_klutz(r, s);
    add_score(r, (long)(r->level + r->level + 0x14));
}

/* orig: HitRound6Peel (0021:0648) */
static void hit_peel(Sprite *s, Sprite *other, int first)
{
    Round6 *r = s->user;

    (void)other;
    (void)first;
    s->visible = 0;
    sound_play(0x4652, 1);
    add_score(r, (long)(-15 - r->level - r->level));
}

/* orig: HookRound6 (0021:01FA) */
static int script_hook(int n, Sprite *s, MoveState *pm, int ready)
{
    Round6 *r = s->user;
    Slot *slot = slot_for(r, s);

    (void)ready;
    if (n < 0) {
        switch (n) {
        case -3:
            free_slot(r, s);
            if (slot && slot->mark)
                *slot->mark = -1;
            r->active--;
            break;
        case -2:
            if (slot && slot->mark)
                *slot->mark = -1;
            break;
        case -1:
            sprite_set_script(s, k_idle);
            if (s->move)
                s->move->hook = NULL;
            break;
        }
        return 1;
    }
    if (n > 0) {
        pm->n_steps = 2;
        if (slot && slot->mark && *slot->mark >= n) {
            pm->n_step = 2;
            return 1;
        }
        pm->n_step = 1;
        return 0;
    }
    if (slot && slot->mark)
        (*slot->mark)++;
    return 1;
}

static Sprite *insert_faller(Round6 *r, Pic *img, Pic *mask, int x, int y)
{
    Sprite *before = NULL;

    if (r->player && r->player->next)
        before = r->player->next->next;
    return scene_insert(r->scene, before, img, mask, x, y, -1);
}

/* orig: SpawnRound6 (0021:0677) */
static void spawn(Round6 *r, uint32_t now)
{
    Sprite *s;

    if (r->left == 0)
        return;
    if (now < r->next_spawn)
        return;
    if (r->active < SLOTS && rnd_mod(r->mod) != 0) {
        int side = rand() & 1;
        int n = side * 5;
        int y = -r->bank[n]->h;
        int x0;
        int x1;

        if (r->aim < 0) {
            x0 = 0x43;
            x1 = FIELD_W - r->bank[n]->w;
        } else {
            x0 = r->aim - r->range;
            if (x0 < 0x43)
                x0 = 0x43;
            x1 = r->aim + r->range;
            if (x1 > FIELD_W - r->bank[n]->w)
                x1 = FIELD_W - r->bank[n]->w;
        }
        if (x1 <= x0)
            x1 = x0 + 1;
        {
            int tries = 0;
            int x = x0;

            do {
                int dx;

                x = x0 + rnd_mod(x1 - x0);
                dx = x - r->aim;
                if (dx < 0)
                    dx = -dx;
                /* The original loops until the gap is wide enough. The cap
                   only lands if the field can no longer satisfy it. */
                if (dx >= r->spread || ++tries >= 64) {
                    r->aim = x;
                    break;
                }
            } while (1);
        }
        s = insert_faller(r, r->bank[n], r->bank[n + 10], r->aim, y);
        if (s) {
            s->user = r;
            s->group = side + 2;
            s->hit_mask = side != 0 ? 0x8000u : 0x4000u;
            s->dy = r->dy;
            s->on_moved = moved_faller;
            s->on_hit = hit_klutz;
        }
        r->active++;
        r->left--;
    } else {
        int x = 0x43 + rnd_mod(0x1BD - r->bank[20]->w);
        int y = -r->bank[20]->h;

        s = insert_faller(r, r->bank[20], r->bank[21], x, y);
        if (s) {
            s->user = r;
            s->group = 4;
            s->hit_mask = 0x2000u;
            s->dy = 0x0A;
            s->on_moved = moved_faller;
            s->on_hit = hit_peel;
        }
    }
    r->next_spawn = now + (uint32_t)r->delay;
}

/* orig: LayRound6 (0021:0891) */
static void lay(Round6 *r)
{
    int i;

    scene_add(r->scene, r->bank[22], NULL, 0, 0, 0);
    for (i = 0; i < 5; i++)
        scene_add(r->scene, r->bank[23], NULL, 10, i * 0x48 - 3, 0);
}

/* orig: ArmRound6 (0021:095E) */
static void arm(Round6 *r)
{
    r->time_left = 0x1E;
    r->time_last = time(NULL);
}

/* orig: SetupRound6 (0021:08C5) */
static void setup(Round6 *r)
{
    int n = r->level;

    if (n >= 10)
        n = (n & 3) + 6;
    r->aim = -1;
    r->left += k_count[n];
    r->mod = k_mod[n];
    r->spread = k_spread[n];
    r->range = k_range[n];
    r->dy = k_dy[n];
    r->delay = k_delay[n];
    if (r->level >= 10) {
        n = r->level - 9;
        r->spread += n * 5;
        if (r->spread > 0x5A)
            r->spread = 0x5A;
        r->dy += n;
        if (r->dy > 0x26)
            r->dy = 0x26;
    }
}

int round6_holding(const Round6 *round)
{
    return round && round->hold;
}

void round6_wave_info(const Round6 *round, int *wave, long *score, long *time_left)
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

void round6_add_score(Round6 *round, long n)
{
    if (round)
        add_score(round, n);
}

/* orig: SetTimeLeft (001D:0446) */
void round6_set_time(Round6 *round, long time_left)
{
    if (!round)
        return;
    round->time_left = time_left;
    if (round->time_left < 0)
        round->time_left = 0;
    round->time_last = time(NULL);
}

/* orig: TickTimeLeft (001D:03AC) / PauseTimeLeft (001D:0425) */
void round6_tick_time(Round6 *round)
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

/* orig: NextPlayWave + ArmRound6 + SetupRound6 + SpawnRound6 after RunStepDialog */
void round6_finish_wave(Round6 *round, uint32_t now)
{
    if (!round)
        return;
    if (now == 0)
        now = round->hold_now;
    round->hold = 0;
    round->wave++;
    if (round->wave > 98)
        round->wave = 98;
    round->level++;
    arm(round);
    round->active = 0;
    round->left = 0;
    setup(round);
    spawn(round, now);
    round->saved = round->cursor;
}

/* orig: TickRound6 lose / hold / miss / TickScene (0021:0A75) */
static int once(Round6 *r, uint32_t now)
{
    time_t wall;
    int steps;

    if (r->over)
        return 1;
    if (r->hold)
        return 0;
    if (r->active == 0 && r->left == 0) {
        r->hold = 1;
        r->hold_now = now;
        return 0;
    }
    if (now >= r->gate) {
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
        spawn(r, now);
        r->gate = now + 3;
        scene_run(r->scene, steps);
        if (r->gone) {
            if (r->saves != 0) {
                r->saves--;
                if (r->level != 0) {
                    r->level--;
                    setup(r);
                    r->left /= 2;
                }
                r->gone = 0;
                sound_play(0x1770, 1);
            } else
                r->over = 1;
        }
    }
    wall = time(NULL);
    r->time_left -= (long)(wall - r->time_last);
    if (r->time_left < 0)
        r->time_left = 0;
    r->time_last = wall;
    return r->over;
}

static int load_bank(const char *resources, Pic **bank)
{
    char path[1024];
    size_t nbytes = 0;
    void *file;
    int i;

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/18000.pic", resources);
    file = SDL_LoadFile(path, &nbytes);
    if (!file)
        return 0;
    for (i = 0; i < BANK; i++) {
        bank[i] = pic_decode_ppic(file, nbytes, i);
        if (!bank[i]) {
            SDL_free(file);
            return 0;
        }
    }
    SDL_free(file);
    return 1;
}

/* orig: OpenRound6 (0021:099B) */
Round6 *round6_new(const char *resources)
{
    Round6 *r = SDL_calloc(1, sizeof(*r));
    int i;
    Pic *player;

    if (!r)
        return NULL;
    srand((unsigned)time(NULL));
    /* orig: ClearRound6Marks (0021:01BF) */
    for (i = 0; i < SLOTS; i++)
        r->marks[i] = -1;
    /* orig: AddPlayAmmo (001D:0144) — OpenRound6 sets g_nPlayAmmo = 3 */
    r->saves = 3;
    if (!load_bank(resources, r->bank)) {
        round6_free(r);
        return NULL;
    }
    /* orig: ShrinkRound6Bottom (0021:096E) — last tumble frame is one pixel short */
    r->bank[4]->h--;
    r->bank[9]->h--;
    r->bank[14]->h--;
    r->bank[19]->h--;
    r->scene = scene_new(3);
    if (!r->scene) {
        round6_free(r);
        return NULL;
    }
    /* orig: AddRound6MomETron (0021:032A) */
    player = r->bank[24];
    r->player = scene_add(r->scene, player, r->bank[25],
                          (FIELD_W - player->w) / 2, CLIP_H - player->h, -1);
    if (!r->player) {
        round6_free(r);
        return NULL;
    }
    r->player->user = r;
    r->player->group = 1;
    r->player->hit_mask = 0xE000u;
    r->player->move_rate = 1;
    r->player->on_move = move_mom_e_tron;
    r->player->on_hit_test = hit_test_mom_e;
    arm(r);
    setup(r);
    lay(r);
    sound_preload(0x4650, 4);
    sound_preload(0x1770, 1);
    return r;
}

/* orig: TickRound6 GetMessagePos read (0021:0A75) */
void round6_set_cursor(Round6 *round, int screen_x)
{
    if (round && !round->over)
        round->cursor = screen_x;
}

/* orig: TickRound6 (0021:0A75) */
int round6_timer(Round6 *round)
{
    if (!round)
        return 0;
    round->step_clock++;
    return once(round, round->step_clock);
}

/* orig: TickScene timing prelude (0004:00D9) via GetTicks/17 */
int round6_pump(Round6 *round)
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

/* orig: PaintRound6 (0021:0B53) */
void round6_paint(const Round6 *round, Pic *field)
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

/* orig: DrawStatusBar round 6 columns (001D:054F) */
void round6_status(const Round6 *round, PlayHud *hud)
{
    if (!hud)
        return;
    SDL_zerop(hud);
    if (!round)
        return;
    hud->kind = 0;
    hud->left_label = "Saves:";
    hud->left_x = 0x28;
    SDL_snprintf(hud->left_value, sizeof(hud->left_value), "%03d", round->saves);
    SDL_snprintf(hud->wave, sizeof(hud->wave), "%02d", round->wave + 1);
    format_time(round->time_left, hud->mid_value);
    hud->score = round->score;
}

/* orig: CloseRound6 (0021:0B62) */
void round6_free(Round6 *round)
{
    int i;

    if (!round)
        return;
    scene_free(round->scene);
    for (i = 0; i < BANK; i++)
        pic_free(round->bank[i]);
    SDL_free(round);
}
