#include "round5.h"

#include "scene.h"
#include "sound.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

/* Matching source: src/round5.c (segment 31): Andromedroids.
   OpenRound5 (001F:0F83)        round5_new
   ClickRound5 (001F:1073)       round5_click (starts fall; DropRound5Part is hook -7)
   TickRound5 (001F:1090)        round5_timer / round5_pump / once
   PaintRound5 (001F:11D1)       round5_paint
   CloseRound5 (001F:11E0)       round5_free
   BackRound5 (001F:0F6B)        field clear in play_draw
   SetupRound5 (001F:0EB2)       setup
   LayRound5 (001F:0DBA)         lay
   SpawnRound5Part (001F:0C49)  spawn_part
   DropRound5Part (001F:0B9C)   drop_part
   MoveRound5Player (001F:0290)  move_player
   AddRound5Player (001F:0316)   player setup in round5_new
   MoveRound5Part (001F:0375)   move_part
   MovedRound5Part (001F:044C)  moved_part
   HookRound5 (001F:003B)        script_hook
   PickRound5Color (001F:0BC8)   pick_color
   ClearRound5Marks (001F:0000)  marks init in round5_new
   AllocRound5Mark (001F:001B)   alloc_mark
   SetRound5Move / SetRound5Frame set_move / set_frame
     (001F:01BE / 001F:0227)
   AddR5Count / AddPlayScore     add_count / add_score
     (001D:028E / 001D:04B4)
   DrawStatusBar (001D:054F)     round5_status */
#include <time.h>

#define FIELD_W 0x200
#define CLIP_H 0x146
#define BANK 97
#define MARKS 8
#define COLS 9

static const int k_col_x[COLS] = { 0, 39, 100, 161, 222, 283, 344, 405, 444 };
static const int k_off[3] = { 16, 8, 6 };
static const int k_extra[10] = { 2, 3, 4, 5, 5, 6, 7, 8, 9, 10 };
static const int k_odds[10] = { 5, 5, 4, 4, 3, 3, 3, 3, 4, 4 };
static const int k_vy[10] = { 2, 3, 4, 5, 6, 7, 8, 8, 9, 9 };
static const int k_bomb_vy[10] = { 4, 5, 6, 9, 9, 9, 10, 10, 11, 11 };
static const int k_delay[10] = { 30, 26, 22, 20, 18, 16, 14, 14, 12, 12 };

static const uint8_t k_idle[8] = { 0x00, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0A, 0x00 };

static const uint8_t k_clear2[194] = {
    0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x0B, 0x04, 0x86, 0x3E, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02,
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x04,
    0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02,
    0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00,
    0x06, 0x02, 0x0C, 0x04, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x04,
    0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02,
    0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x09, 0x04, 0x0A, 0x00, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04,
    0x00, 0x00, 0x09, 0x04, 0x01, 0x00, 0x04, 0x08, 0x06, 0x00, 0x00, 0x00, 0x47, 0x00, 0x09, 0x04,
    0x0A, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0x04, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04,
    0x06, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0xFA, 0xFF, 0x0C, 0x04, 0xFC, 0xFF, 0x09, 0x04,
    0x01, 0x00, 0x04, 0x08, 0x08, 0x00, 0x20, 0x00, 0x00, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02,
    0x06, 0x02, 0x06, 0x02, 0x04, 0x08, 0x78, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x0C, 0x04, 0xFE, 0xFF,
    0x0E, 0x02
};
static const uint8_t k_clear1[66] = {
    0x00, 0x02, 0x0C, 0x04, 0x01, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0C, 0x04,
    0x02, 0x00, 0x04, 0x08, 0x06, 0x00, 0x00, 0x00, 0x47, 0x00, 0x0C, 0x04, 0x03, 0x00, 0x01, 0x02,
    0x0C, 0x04, 0x07, 0x00, 0x04, 0x08, 0x09, 0x00, 0x24, 0x00, 0x00, 0x00, 0x06, 0x02, 0x06, 0x02,
    0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x02, 0x02, 0x04, 0x08, 0x77, 0x00, 0xDC, 0x01, 0x00, 0x00,
    0x0E, 0x02
};
static const uint8_t k_clear0[38] = {
    0x00, 0x02, 0x0C, 0x04, 0x02, 0x00, 0x04, 0x08, 0x06, 0x00, 0x00, 0x00, 0x47, 0x00, 0x0C, 0x04,
    0x07, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x04, 0x08, 0x80, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x0E, 0x02
};
static const uint8_t k_lid_clear[26] = {
    0x00, 0x02, 0x0C, 0x04, 0x02, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x88, 0x3E, 0x09, 0x04, 0x0A, 0x00,
    0x07, 0x02, 0x0B, 0x04, 0x89, 0x3E, 0x0C, 0x04, 0xFF, 0xFF
};
static const uint8_t k_score_a[58] = {
    0x00, 0x02, 0x0C, 0x04, 0x05, 0x00, 0x02, 0x02, 0x0B, 0x04, 0x87, 0x3E, 0x04, 0x08, 0x05, 0x00,
    0x00, 0x00, 0xF6, 0xFF, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0x07, 0x00, 0x09, 0x04, 0x01, 0x00,
    0x04, 0x08, 0x08, 0x00, 0x20, 0x00, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x08, 0x78, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x0E, 0x02
};
static const uint8_t k_score_b[36] = {
    0x00, 0x02, 0x0C, 0x04, 0x05, 0x00, 0x02, 0x02, 0x04, 0x08, 0x05, 0x00, 0x00, 0x00, 0xF6, 0xFF,
    0x0C, 0x04, 0x07, 0x00, 0x09, 0x04, 0x01, 0x00, 0x04, 0x08, 0x08, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x0E, 0x02
};
static const uint8_t k_score_c[50] = {
    0x00, 0x02, 0x0C, 0x04, 0x03, 0x00, 0x02, 0x02, 0x09, 0x04, 0x04, 0x00, 0x06, 0x02, 0x09, 0x04,
    0x04, 0x00, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x06, 0x02, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04,
    0x07, 0x00, 0x09, 0x04, 0x01, 0x00, 0x04, 0x08, 0x09, 0x00, 0x24, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x0E, 0x02
};
static const uint8_t k_lid_bad0[42] = {
    0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x88, 0x3E, 0x0C, 0x04, 0x00, 0x00,
    0x09, 0x04, 0x0A, 0x00, 0x07, 0x02, 0x0B, 0x04, 0x89, 0x3E, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04,
    0xFA, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04, 0xFF, 0xFF
};
static const uint8_t k_lid_bad1[50] = {
    0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x88, 0x3E, 0x0C, 0x04, 0x00, 0x00,
    0x09, 0x04, 0x01, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x0A, 0x00, 0x07, 0x02, 0x0B, 0x04,
    0x89, 0x3E, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04, 0xFA, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04,
    0xFF, 0xFF
};
static const uint8_t k_lid_bad2[58] = {
    0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x88, 0x3E, 0x0C, 0x04, 0x00, 0x00,
    0x09, 0x04, 0x01, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x01, 0x00, 0x0C, 0x04, 0x00, 0x00,
    0x09, 0x04, 0x0A, 0x00, 0x07, 0x02, 0x0B, 0x04, 0x89, 0x3E, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04,
    0xFA, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04, 0xFF, 0xFF
};
static const uint8_t k_bad2[16] = {
    0x00, 0x02, 0x0C, 0x04, 0x03, 0x00, 0x04, 0x08, 0x05, 0x00, 0x00, 0x00, 0x87, 0x00, 0x0E, 0x02
};
static const uint8_t k_bad1[16] = {
    0x00, 0x02, 0x0C, 0x04, 0x02, 0x00, 0x04, 0x08, 0x05, 0x00, 0x00, 0x00, 0x87, 0x00, 0x0E, 0x02
};
static const uint8_t k_bad0[16] = {
    0x00, 0x02, 0x0C, 0x04, 0x01, 0x00, 0x04, 0x08, 0x05, 0x00, 0x00, 0x00, 0x87, 0x00, 0x0E, 0x02
};
static const uint8_t k_bomb2[46] = {
    0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0B, 0x04, 0x85, 0x3E, 0x04, 0x08,
    0x01, 0x00, 0x00, 0x00, 0x15, 0x00, 0x09, 0x04, 0x08, 0x00, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04,
    0xFA, 0xFF, 0x0C, 0x04, 0xFD, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04, 0xFF, 0xFF
};
static const uint8_t k_bomb3[66] = {
    0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0B, 0x04, 0x85, 0x3E, 0x04, 0x08,
    0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x09, 0x04, 0x0A, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0B, 0x04,
    0x85, 0x3E, 0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0x15, 0x00, 0x09, 0x04, 0x08, 0x00, 0x0C, 0x04,
    0xFB, 0xFF, 0x0C, 0x04, 0xFA, 0xFF, 0x0C, 0x04, 0xFD, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04,
    0xFF, 0xFF
};
static const uint8_t k_sink0[10] = { 0x00, 0x02, 0x0C, 0x04, 0x01, 0x00, 0x01, 0x02, 0x0E, 0x02 };
static const uint8_t k_sink1[10] = { 0x00, 0x02, 0x0C, 0x04, 0x02, 0x00, 0x01, 0x02, 0x0E, 0x02 };

/* RollL is followed by RollR, and RollR by the loop. Hook -7 returns into
   the next script, and that script's wait is what stops the call. */
static const uint8_t k_roll_loop[] = {
    0x00, 0x02, 0x09, 0x04, 0x08, 0x00, 0x04, 0x08, 0x04, 0x00, 0xE2, 0xFF, 0xF6, 0xFF, 0x04, 0x08,
    0x04, 0x00, 0xE2, 0xFF, 0x0A, 0x00, 0x0C, 0x04, 0xF9, 0xFF,
    0x00, 0x02, 0x09, 0x04, 0x08, 0x00, 0x04, 0x08, 0x04, 0x00, 0x1E, 0x00, 0xF6, 0xFF, 0x04, 0x08,
    0x04, 0x00, 0x1E, 0x00, 0x0A, 0x00, 0x0C, 0x04, 0xF9, 0xFF,
    0x00, 0x02, 0x09, 0x04, 0x03, 0x00, 0x06, 0x02, 0x09, 0x04, 0x03, 0x00, 0x06, 0x02, 0x09, 0x04,
    0x03, 0x00, 0x06, 0x02, 0x0A, 0x00
};
static const uint8_t *const k_roll_l = k_roll_loop;
static const uint8_t *const k_roll_r = k_roll_loop + 26;
static const uint8_t *const k_loop = k_roll_loop + 52;

struct Round5 {
    Pic *bank[BANK];
    Scene *scene;
    Sprite *player;
    Sprite *before;
    Sprite *piece;
    Sprite *lid[7];
    Sprite *stack[COLS][3];
    int floor_y[COLS];
    int depth[COLS];
    int marks[MARKS];
    int count[4];
    int lost[3];
    int cursor;
    int saved;
    int player_x;
    int col;
    int next_col;
    int active;
    int done;
    int drop;
    int vy;
    int bomb_vy;
    int odds;
    int delay;
    int wave;
    int hold;
    long score;
    int over;
    int clock_started;
    int scene_started;
    uint32_t clock;
    uint32_t step_clock;
    uint32_t gate;
    uint32_t next_spawn;
    uint32_t scene_mark;
    uint32_t now;
};

static int script_hook(int n, Sprite *s, MoveState *pm, int ready);
static void move_part(Sprite *s);
static void moved_part(Sprite *s);

static int rnd_mod(int n)
{
    if (n <= 0)
        return 0;
    return (int)((unsigned)rand() % (unsigned)n);
}

/* orig: AddPlayScore (001D:04B4) */
static void add_score(Round5 *r, long n)
{
    r->score += n;
    if (r->score < 0)
        r->score = 0;
}

/* orig: AddR5Count (001D:028E) */
static void add_count(Round5 *r, int n, int i)
{
    r->count[i] += n;
    if (r->count[i] < 0)
        r->count[i] = 0;
}

/* orig: AllocRound5Mark (001F:001B) */
static int *alloc_mark(Round5 *r)
{
    int i;

    for (i = 0; i < MARKS; i++) {
        if (r->marks[i] < 0) {
            r->marks[i] = 0;
            return &r->marks[i];
        }
    }
    return NULL;
}

/* orig: SetRound5Move (001F:01BE) */
static void set_move(Sprite *s, const uint8_t *script, MoveHook hook, int *mark)
{
    if (!s)
        return;
    sprite_set_script(s, script);
    if (!s->move)
        return;
    s->move->hook = hook;
    s->move->mark = mark;
}

/* orig: SetRound5Frame (001F:0227) */
static void set_frame(Sprite *s, Pic **images, int frame, int frames, Pic **masks)
{
    s->images = images;
    s->masks = masks;
    s->frame = frame;
    s->frames = frames;
    sprite_sync(s);
}

static void release_piece(Round5 *r, Sprite *s)
{
    r->active--;
    r->next_spawn = r->now + (uint32_t)r->delay;
    r->piece = NULL;
    if (s->group != 6)
        sound_play(0x3E81, 0);
}

/* orig: DropRound5Part (001F:0B9C) */
static void drop_part(Sprite *s)
{
    s->move_rate = 1;
    s->dy = 8;
    s->on_move = move_part;
    s->on_moved = moved_part;
}

/* orig: HookRound5 (001F:003B) */
static int script_hook(int n, Sprite *s, MoveState *pm, int ready)
{
    Round5 *r = s->user;

    (void)ready;
    if (n < 0) {
        switch (n) {
        case -7:
            if (pm->mark)
                *pm->mark = -1;
            s->move_ex = NULL;
            drop_part(s);
            break;
        case -5:
            if (r->wave > 0) {
                r->active--;
                r->next_spawn = r->now;
                r->piece = NULL;
            }
            break;
        case -6:
            if (r->wave == 0) {
                r->active--;
                r->next_spawn = r->now;
                r->piece = NULL;
            }
            break;
        case -4:
            add_count(r, -1, 3);
            break;
        case -3:
            r->done++;
            break;
        case -2:
            if (pm->mark)
                *pm->mark = -1;
            break;
        case -1:
            set_move(s, k_idle, NULL, NULL);
            break;
        }
        return 1;
    }
    if (n > 0) {
        pm->n_steps = 2;
        if (!pm->mark || *pm->mark >= n) {
            pm->n_step = 2;
            return 1;
        }
        pm->n_step = 1;
        return 0;
    }
    if (pm->mark)
        (*pm->mark)++;
    return 1;
}

/* orig: MoveRound5Player (001F:0290) */
static void move_player(Sprite *s)
{
    Round5 *r = s->user;
    int dx = r->cursor - r->saved;
    int x;
    int limit;

    if (dx > 0x20)
        dx = 0x20;
    else if (dx < -0x20)
        dx = -0x20;
    x = s->left + dx;
    limit = FIELD_W - s->img->w;
    if (x < 0) {
        dx += -x;
        x = 0;
        r->saved = r->cursor;
    } else if (x > limit) {
        dx -= x - limit;
        x = limit;
        r->saved = r->cursor;
    } else
        r->saved += dx;
    r->player_x = s->left + dx;
    s->dx = dx;
}

/* orig: MoveRound5Part (001F:0375) */
static void move_part(Sprite *s)
{
    Round5 *r = s->user;
    int best_abs;
    int i;

    s->dx = 0;
    if (s->bottom < 0xCB) {
        s->dx = r->player_x + 0x13 - s->x;
        return;
    }
    if (r->col == -1) {
        best_abs = 0x7D00;
        for (i = 0; i < COLS; i++) {
            int d = s->x - 0x13 - k_col_x[i];
            int a = d < 0 ? -d : d;

            if (a < best_abs) {
                best_abs = a;
                r->col = i;
            }
        }
    }
    s->dx = k_col_x[r->col] - s->x + 0x13;
    if (s->bottom + s->dy > r->floor_y[r->col])
        s->dy -= s->bottom + s->dy - r->floor_y[r->col];
    else if (s->dy < 8) {
        if (s->dx < 0) {
            if (s->dx < 4)
                s->dx = -4;
        } else if (s->dx > 4)
            s->dx = 4;
    }
}

static void wipe_stack(Round5 *r, int col)
{
    r->stack[col][0] = NULL;
    r->stack[col][1] = NULL;
    r->stack[col][2] = NULL;
    r->depth[col] = 0;
    r->floor_y[col] = 0x100;
}

/* orig: MovedRound5Part (001F:044C) */
static void moved_part(Sprite *s)
{
    Round5 *r = s->user;
    Sprite **pile;
    int next = 1;
    int col;

    if (r->col == -1)
        return;
    if (r->floor_y[r->col] > s->bottom)
        return;
    col = r->col;
    pile = r->stack[col];
    /* An empty bay is not a bomb. The original reads through the null slot. */
    if (pile[0] && pile[0]->group == 6 && s->group != 6) {
        if (r->floor_y[col] == 0x100) {
            s->visible = 0;
            s->dead = 1;
            r->floor_y[col] -= 0x15;
            if (r->depth[col] == 2)
                r->floor_y[col] -= 0x15;
            sound_play(0x3E85, 0);
            add_score(r, -5);
            add_count(r, -1, s->kind);
            release_piece(r, s);
            return;
        }
        s->dy = r->drop ? 0x18 : r->vy;
        r->floor_y[col] = 0x100;
        return;
    }
    s->dx = 0;
    s->dy = 0;
    s->on_move = NULL;
    s->on_moved = NULL;
    if (col != 0 && col != 8) {
        int n;

        pile[r->depth[col]++] = s;
        if (s->img)
            r->floor_y[col] -= s->img->h;
        if (s->group == 6) {
            n = r->depth[col];
            s->anim_rate = 2;
            if (n == 1) {
                r->floor_y[col] = 0xEB;
                r->done++;
            } else if (n == 2 && pile[0] && pile[0]->group == 6) {
                r->floor_y[col] = 0xD6;
                pile[0]->anim_rate = 0;
            } else if (n == 2) {
                int *mark = alloc_mark(r);

                next = 0;
                set_move(s, k_bomb2, script_hook, mark);
                set_move(pile[0], k_sink0, script_hook, mark);
                pile[0] = s;
                pile[1] = NULL;
                r->depth[col] = 1;
                r->floor_y[col] = 0xEB;
                add_score(r, -5);
            } else if (pile[1] && pile[1]->group == 6) {
                int *mark = alloc_mark(r);

                next = 0;
                if (col == 1) {
                    r->next_col = 2;
                    s->kind = 1;
                } else if (col == 7) {
                    r->next_col = 6;
                    s->kind = -1;
                } else if (s->kind == 0) {
                    /* Borland rand() is never negative, so this rolls right. */
                    if (rand() < 0) {
                        r->next_col = col - 1;
                        s->kind = -1;
                    } else {
                        r->next_col = col + 1;
                        s->kind = 1;
                    }
                } else
                    r->next_col = col + s->kind;
                if (r->next_col < col)
                    set_move(s, k_roll_l, script_hook, mark);
                else
                    set_move(s, k_roll_r, script_hook, mark);
                r->floor_y[col] = 0xD6;
                r->depth[col]--;
                r->col = r->next_col;
                pile[2] = NULL;
            } else {
                int *mark = alloc_mark(r);

                next = 0;
                set_move(s, k_bomb3, script_hook, mark);
                set_move(pile[0], k_sink1, script_hook, mark);
                set_move(pile[1], k_sink0, script_hook, mark);
                pile[0] = s;
                pile[1] = NULL;
                pile[2] = NULL;
                r->depth[col] = 1;
                r->floor_y[col] = 0xEB;
                add_score(r, -10);
            }
        } else {
            int i;

            add_count(r, -1, s->kind);
            for (i = 0; i < 3; i++) {
                Sprite *p = pile[i];

                if (!p)
                    continue;
                if (p->kind != 2 - i) {
                    int *mark = alloc_mark(r);

                    sound_play(0x3E83, 0);
                    int penalty = i == 0 ? -10 : i == 1 ? -20 : -30;
                    const uint8_t *lid_cmd = i == 0 ? k_lid_bad0 : i == 1 ? k_lid_bad1 : k_lid_bad2;

                    set_move(pile[0], k_bad0, script_hook, mark);
                    if (pile[1]) {
                        set_move(pile[1], k_bad1, script_hook, mark);
                        if (pile[2])
                            set_move(pile[2], k_bad2, script_hook, mark);
                    }
                    add_score(r, penalty);
                    set_move(r->lid[col - 1], lid_cmd, script_hook, mark);
                    next = 0;
                    break;
                }
                if (i == 2) {
                    Sprite *q = pile[0];
                    Sprite *t;
                    int *mark = alloc_mark(r);
                    int shape;
                    Pic **body;

                    sound_play(0x3E81, 0);
                    if (q->group == pile[1]->group && q->group == pile[2]->group)
                        add_score(r, 50);
                    else
                        add_score(r, 30);
                    set_move(q, k_clear0, script_hook, mark);
                    set_move(pile[1], k_clear1, script_hook, mark);
                    set_move(pile[2], k_clear2, script_hook, mark);
                    set_move(r->lid[col - 1], k_lid_clear, script_hook, mark);
                    t = scene_add(r->scene, r->bank[61 + pile[2]->group], NULL,
                                  q->left, 0x11C, 1);
                    if (t) {
                        t->user = r;
                        t->visible = 0;
                        set_move(t, k_score_a, script_hook, mark);
                    }
                    shape = pile[2]->group - 3;
                    t = scene_add(r->scene, r->bank[64 + shape], NULL,
                                  q->left + k_off[shape], 0x11C, 1);
                    if (t) {
                        t->user = r;
                        t->visible = 0;
                        set_move(t, k_score_b, script_hook, mark);
                    }
                    body = &r->bank[48 + ((pile[1]->group - 3) << 2)];
                    t = scene_add(r->scene, body[0], r->bank[60], q->left - 4, 0x12A, -1);
                    if (t) {
                        t->user = r;
                        t->visible = 0;
                        set_frame(t, body, 0, 4, &r->bank[60]);
                        set_move(t, k_score_c, script_hook, mark);
                    }
                    next = 0;
                }
            }
            if (next)
                add_score(r, 5);
            else
                wipe_stack(r, col);
        }
    } else {
        int pts;
        int id;

        s->visible = 0;
        s->dead = 1;
        if (col == 0) {
            if (s->group == 6) {
                pts = -5;
                id = 0x3E85;
                add_count(r, -1, 0);
                add_count(r, -1, 1);
                add_count(r, -1, 2);
            } else {
                pts = -2;
                id = 0x3E82;
                r->lost[s->kind]++;
            }
        } else if (s->group == 6) {
            pts = 5;
            id = 0x3E84;
        } else {
            pts = -5;
            id = 0x3E85;
            add_count(r, -1, s->kind);
        }
        sound_play(id, 0);
        add_score(r, pts);
    }
    if (next)
        release_piece(r, s);
}

/* orig: PickRound5Color (001F:0BC8) */
static int pick_color(Round5 *r)
{
    int i;
    int n = 0;
    int a, b, c;

    for (i = 0; i < 2; i++) {
        a = r->count[0] - r->lost[0];
        b = r->count[1] - r->lost[1];
        c = r->count[2] - r->lost[2];
        n = a + b + c;
        if (n != 0)
            break;
        r->lost[0] = 0;
        r->lost[1] = 0;
        r->lost[2] = 0;
    }
    if (n <= 0)
        return 0;
    n = rnd_mod(n);
    if (n < a)
        return 0;
    n -= a;
    if (n < b)
        return 1;
    return 2;
}

/* orig: SpawnRound5Part (001F:0C49) */
static void spawn_part(Round5 *r)
{
    Sprite *s;
    int n;

    s = scene_insert(r->scene, r->before, NULL, NULL, 0, 0, -1);
    if (!s)
        return;
    r->piece = s;
    s->user = r;
    s->move_rate = 1;
    s->on_move = move_part;
    s->on_moved = moved_part;
    if (rnd_mod(r->odds) != (r->odds >> 1)) {
        s->group = rnd_mod(3) + 3;
        s->kind = pick_color(r);
        n = (s->group - 3) * 8 + s->kind;
        set_frame(s, &r->bank[n], 0, 8, &r->bank[24 + n]);
        s->draw_mode = -1;
        s->dy = r->vy;
    } else {
        s->group = 6;
        set_frame(s, &r->bank[67], 0, 6, &r->bank[73]);
        s->draw_mode = -1;
        s->anim_rate = 1;
        s->dy = r->bomb_vy;
    }
    s->x = r->player->x + 0x13;
    s->y = r->player->img->h - s->img->h + 4;
    sprite_sync(s);
    r->col = -1;
    r->active++;
    r->drop = 0;
    sound_play(0x3E80, 1);
}

static int bay_has_color(const Round5 *r, int color)
{
    int col;
    int i;

    for (col = 1; col <= 7; col++) {
        for (i = 0; i < 3; i++) {
            const Sprite *p = r->stack[col][i];

            if (p && p->group != 6 && p->kind == color)
                return 1;
        }
    }
    return 0;
}

/* orig: LayRound5 (001F:0DBA) */
static void lay(Round5 *r)
{
    Sprite *s;
    int i;

    r->before = NULL;
    scene_add(r->scene, r->bank[84], r->bank[85], 0, 0xA4, -1);
    scene_add(r->scene, r->bank[87], r->bank[88], 0x1C6, 0xA3, -1);
    s = scene_add(r->scene, r->bank[90], r->bank[93], 0x0C, 0xD6, -1);
    if (s) {
        s->user = r;
        s->anim_mode = 1;
        set_frame(s, &r->bank[90], 0, 3, &r->bank[93]);
        set_move(s, k_loop, NULL, NULL);
    }
    for (i = 0; i < 6; i++)
        scene_add(r->scene, r->bank[82], r->bank[83], i * 0x3D + 0x58, 0xCB, -1);
    for (i = 0; i < 7; i++) {
        s = scene_add(r->scene, r->bank[80], NULL, i * 0x3D + 0x22, 0xFF, 0);
        if (s) {
            if (!r->before)
                r->before = s;
            s->user = r;
            s->group = 2;
            s->kind = i;
            set_frame(s, &r->bank[80], 0, 2, NULL);
            r->lid[i] = s;
        }
    }
    scene_add(r->scene, r->bank[86], NULL, 0x0C, 0xB0, 0);
    scene_add(r->scene, r->bank[89], NULL, 0x1D3, 0xB1, 0);
}

/* orig: SetupRound5 (001F:0EB2) */
static void setup(Round5 *r)
{
    int n = r->wave;
    int k;

    if (n >= 10)
        n = 9;
    if (n == 0)
        r->count[3] = k_extra[n];
    else
        add_count(r, k_extra[n], 3);
    r->vy = k_vy[n];
    r->bomb_vy = k_bomb_vy[n];
    r->odds = k_odds[n];
    r->delay = k_delay[n];
    k = r->count[3] + 1;
    if (n == 0) {
        r->count[0] += k;
        r->count[1] += k;
        r->count[2] += k;
    } else {
        if (r->wave >= 5)
            k--;
        add_count(r, k, 0);
        add_count(r, k, 1);
        add_count(r, k, 2);
    }
    r->lost[0] = 0;
    r->lost[1] = 0;
    r->lost[2] = 0;
    r->next_spawn = 0;
}

int round5_holding(const Round5 *round)
{
    return round && round->hold;
}

void round5_wave_info(const Round5 *round, int *wave, long *score, long *time_left)
{
    if (!round)
        return;
    if (wave)
        *wave = round->wave + 1;
    if (score)
        *score = round->score;
    if (time_left)
        *time_left = 0;
}

void round5_add_score(Round5 *round, long n)
{
    if (round)
        add_score(round, n);
}

/* orig: NextPlayWave + SetupRound5 after RunStepDialog */
void round5_finish_wave(Round5 *round)
{
    if (!round)
        return;
    round->hold = 0;
    round->wave++;
    if (round->wave > 98)
        round->wave = 98;
    setup(round);
}

/* orig: TickRound5 lose / hold / spawn / TickScene (001F:1090) */
static int once(Round5 *r, uint32_t now)
{
    int steps;
    int i;

    if (r->over)
        return 1;
    if (r->hold)
        return 0;
    r->now = now;
    if (r->count[3] == 0 && !r->piece) {
        r->hold = 1;
        return 0;
    }
    if (r->next_spawn == 0) {
        r->next_spawn = now + (uint32_t)r->delay;
        r->saved = r->cursor;
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
        if (r->active == 0 && now >= r->next_spawn)
            spawn_part(r);
        r->gate = now + 3;
        scene_run(r->scene, steps);
        if (!r->piece) {
            if (r->count[3] != 0) {
                for (i = 0; i < 3; i++) {
                    if (r->count[i] == 0 && !bay_has_color(r, i))
                        r->over = 1;
                }
                if (!r->over && r->done == 7)
                    r->over = 1;
            }
        } else if (r->done == 7)
            r->over = 1;
    }
    return r->over;
}

static int load_bank(const char *resources, Pic **bank)
{
    char path[1024];
    size_t nbytes = 0;
    void *file;
    int i;

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/16000.pic", resources);
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

/* orig: OpenRound5 (001F:0F83) */
Round5 *round5_new(const char *resources)
{
    Round5 *r = SDL_calloc(1, sizeof(*r));
    int i;

    if (!r)
        return NULL;
    srand((unsigned)time(NULL));
    /* orig: ClearRound5Marks (001F:0000) */
    for (i = 0; i < MARKS; i++)
        r->marks[i] = -1;
    for (i = 0; i < COLS; i++)
        r->floor_y[i] = 0x100;
    /* The last column's fourth zero spills onto floor[0], so the left
       chute's 0xEF does not survive OpenRound5. */
    r->floor_y[0] = 0;
    r->col = -1;
    if (!load_bank(resources, r->bank)) {
        round5_free(r);
        return NULL;
    }
    r->scene = scene_new(3);
    if (!r->scene) {
        round5_free(r);
        return NULL;
    }
    /* orig: AddRound5Player (001F:0316) */
    r->player = scene_add(r->scene, r->bank[79], NULL, k_col_x[4], 0, 0);
    if (!r->player) {
        round5_free(r);
        return NULL;
    }
    r->player->user = r;
    r->player->group = 1;
    r->player->move_rate = 1;
    r->player->on_move = move_player;
    r->player_x = r->player->left;
    sound_preload(0x3E80, 0xB);
    setup(r);
    lay(r);
    return r;
}

/* orig: TickRound5 GetMessagePos read (001F:1090) */
void round5_set_cursor(Round5 *round, int screen_x)
{
    if (round && !round->over)
        round->cursor = screen_x * 2;
}

/* orig: ClickRound5 (001F:1073) */
void round5_click(Round5 *round)
{
    if (!round || round->over || !round->piece)
        return;
    round->piece->dy = 0x18;
    round->drop = 1;
}

/* orig: TickRound5 (001F:1090) */
int round5_timer(Round5 *round)
{
    if (!round)
        return 0;
    round->step_clock++;
    return once(round, round->step_clock);
}

/* orig: TickScene timing prelude (0004:00D9) via GetTicks/17 */
int round5_pump(Round5 *round)
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

/* orig: PaintRound5 (001F:11D1) */
void round5_paint(const Round5 *round, Pic *field)
{
    if (round)
        scene_paint(round->scene, field, FIELD_W, CLIP_H);
}

/* orig: DrawStatusBar round 5 / DrawR5Parts / DrawR5Droids (001D:054F) */
void round5_status(const Round5 *round, PlayHud *hud)
{
    if (!hud)
        return;
    SDL_zerop(hud);
    if (!round)
        return;
    hud->kind = 2;
    hud->left_label = "Parts:";
    hud->left_x = 0x20;
    SDL_snprintf(hud->left_value, sizeof(hud->left_value), "%02d-%02d-%02d",
                 round->count[0], round->count[1], round->count[2]);
    SDL_snprintf(hud->wave, sizeof(hud->wave), "%02d", round->wave + 1);
    SDL_snprintf(hud->mid_value, sizeof(hud->mid_value), "%02d", round->count[3]);
    hud->score = round->score;
}

/* orig: CloseRound5 (001F:11E0) */
void round5_free(Round5 *round)
{
    int i;

    if (!round)
        return;
    scene_free(round->scene);
    for (i = 0; i < BANK; i++)
        pic_free(round->bank[i]);
    SDL_free(round);
}
