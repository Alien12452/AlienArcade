#include "round4.h"

#include "scene.h"
#include "sound.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

/* Matching source: src/round4.c (segment 32): Fungus Patrol.
   OpenRound4 (0020:0CCE)        round4_new
   ClickRound4 (0020:100E)       round4_click
   TickRound4 (0020:106B)        round4_timer / round4_pump / once
   PaintRound4 (0020:111C)       round4_paint
   CloseRound4 (0020:112B)       round4_free
   BackRound4 (0020:0CB6)        field clear in play_draw
   SetupRound4 (0020:0C23)       setup
   ArmRound4 (0020:0C95)         arm
   LayRound4 (0020:0800)         core sprite in round4_new
   SpawnRound4Wave (0020:0BAA)   spawn_wave
   SpawnRound4Fungus (0020:0922)  spawn_fungus
   BurstRound4Ball (0020:0566)   burst_ball
   MoveRound4Ball (0020:0311)    move_ball
   HitTestRound4Ball (0020:045E) test_ball
   MovedRound4Ball (0020:0534)   moved_ball
   AddRound4Ball (0020:061D)     ball setup in round4_new
   AddRound4Bungee (0020:07B7)     goal setup in round4_new
   MoveRound4Bungee (0020:06A0)    move_bungee
   DrawRound4Bungee (0020:074E)    paint_goal
   MoveRound4Script (0020:0259)  script_hook
   TickRound4Fungus (0020:08FE)   tick_fungus
   MovedRound4Fungus (0020:08E3)  moved_fungus
   ClearRound4Marks (0020:021E)  marks init in round4_new
   AllocRound4Mark (0020:0239)   alloc_mark
   Find/FreeRound4Slot           find_slot / free_slot
     (0020:0873 / 0020:089E)
   AddPlayScore (001D:04B4)      add_score
   SetTimeLeft / FormatTime      round4_set_time / format_time
     (001D:0446 / 001D:0737)
   DrawStatusBar (001D:054F)     round4_status */
#include <time.h>

#define FIELD_W 0x200
#define CLIP_H 0x146
#define BANK 76
#define SLOTS 16

/* Scraper outlines. Point count, then x, y pairs. g_apR4Ball. */
static const int k_ball0[] = { 8, 20, 3, 52, 3, 52, 8, 45, 15, 45, 20, 26, 20, 26, 15, 20, 8 };
static const int k_ball1[] = { 8, 25, 1, 58, 6, 57, 10, 50, 16, 48, 23, 29, 18, 30, 13, 25, 6 };
static const int k_ball2[] = { 5, 33, 0, 61, 12, 49, 22, 34, 16, 31, 2 };
static const int k_ball3[] = { 8, 38, 0, 66, 16, 66, 19, 56, 24, 52, 27, 38, 16, 38, 14, 38, 4 };
static const int k_ball4[] = { 6, 49, 2, 70, 25, 63, 29, 54, 30, 44, 17, 44, 9 };
static const int k_ball5[] = { 5, 54, 4, 71, 33, 62, 34, 57, 33, 45, 19 };
static const int k_ball6[] = { 6, 61, 10, 72, 41, 67, 40, 54, 37, 48, 21, 57, 10 };
static const int k_ball7[] = { 6, 66, 13, 71, 46, 65, 46, 54, 41, 51, 24, 60, 15 };
static const int *const k_ball[8] = {
    k_ball0, k_ball1, k_ball2, k_ball3, k_ball4, k_ball5, k_ball6, k_ball7
};

static const int k_enemy0[] = { 4, 2, 0, 15, 0, 15, 13, 2, 13 };
static const int k_enemy1[] = { 8, 0, 6, 6, 2, 11, 2, 18, 6, 18, 9, 11, 13, 6, 13, 0, 9 };
static const int k_enemy2[] = { 8, 13, 8, 20, 8, 24, 11, 25, 19, 20, 26, 11, 24, 6, 18, 8, 11 };
static const int k_enemy3[] = { 8, 14, 5, 24, 9, 27, 18, 25, 23, 18, 27, 9, 25, 5, 21, 6, 9 };
static const int k_enemy4[] = {
    10, 1, 10, 7, 1, 26, 1, 30, 6, 30, 14, 27, 22, 26, 28, 22, 29, 10, 29, 1, 18
};
static const int *const k_enemy[5] = {
    k_enemy0, k_enemy1, k_enemy2, k_enemy3, k_enemy4
};

static const int k_script[10] = { 24, 24, 24, 32, 32, 32, 24, 16, 12, 8 };
static const int k_div[10] = { 2, 2, 2, 3, 3, 3, 4, 4, 5, 5 };
static const int k_wave[10] = { 3, 3, 3, 3, 4, 4, 5, 5, 5, 5 };
static const int k_burst[10] = { 2, 3, 4, 5, 5, 6, 6, 7, 7, 8 };
static const int k_arm[10] = { 10, 9, 9, 8, 8, 6, 6, 5, 4, 4 };
static const int k_vel[32][2] = {
    {0, -32}, {6, -31}, {12, -29}, {17, -26}, {22, -22}, {26, -17}, {29, -11}, {31, -6},
    {32, 0}, {31, 6}, {29, 12}, {26, 17}, {22, 22}, {17, 26}, {11, 29}, {6, 31},
    {0, 32}, {-6, 31}, {-12, 29}, {-17, 26}, {-22, 22}, {-26, 17}, {-29, 11}, {-31, 6},
    {-32, 0}, {-31, -6}, {-29, -12}, {-26, -17}, {-22, -22}, {-17, -26}, {-11, -29}, {-6, -31}
};

/* g_r4Cmd. Each fungus gets its own copy; the speeds and points are patched in. */
static const uint8_t k_cmd[58] = {
    0x00, 0x02, 0x03, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x0B, 0x04, 0x6A, 0x42, 0x06, 0x02,
    0x09, 0x04, 0x63, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x6B, 0x42, 0x04, 0x08, 0x00, 0x00, 0x63, 0x00,
    0x63, 0x00, 0x0C, 0x04, 0xFC, 0xFF, 0x03, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x01, 0x02,
    0x09, 0x04, 0x01, 0x00, 0x0C, 0x04, 0xFD, 0xFF, 0x0E, 0x02
};

static const uint8_t k_idle[8] = { 0x00, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0A, 0x00 };

struct Round4;

typedef struct Slot {
    struct Round4 *round;
    Sprite *sprite;
    uint8_t script[64];
    int *mark;
    int was_hidden;
} Slot;

struct Round4 {
    Pic *bank[BANK];
    Scene *scene;
    Sprite *ball;
    Sprite *goal;
    Slot slots[SLOTS];
    int marks[SLOTS];
    int cursor_x;
    int cursor_y;
    int saved_x;
    int saved_y;
    int button;
    int frame;
    int count;
    int left;
    int active;
    int script_n;
    int div_n;
    int wave_n;
    int burst_n;
    int arm_n;
    int wave;
    int hold;
    long score;
    long time_left;
    time_t time_last;
    int over;
    int clock_started;
    uint32_t clock;
    uint32_t next_tick;
    uint32_t deadline;
    int scene_started;
    uint32_t scene_mark;
    uint32_t step_clock;
};

static int script_hook(int n, Sprite *s, MoveState *pm, int ready);

static void put_i16(uint8_t *p, int v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

/* orig: AddPlayScore (001D:04B4) */
static void add_score(Round4 *r, long n)
{
    r->score += n;
    if (r->score < 0)
        r->score = 0;
}

/* orig: AllocRound4Mark (0020:0239) */
static int *alloc_mark(Round4 *r)
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

/* orig: FindRound4Slot (0020:0873) */
static Slot *find_slot(Round4 *r)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (!r->slots[i].sprite)
            return &r->slots[i];
    }
    return NULL;
}

/* orig: FreeRound4Slot (0020:089E) */
static void free_slot(Sprite *s)
{
    Slot *slot = s->user;

    if (slot && slot->sprite == s)
        slot->sprite = NULL;
}

static void plot(Pic *dst, int x, int y, int clip_w, int clip_h)
{
    uint8_t *p;

    if (x < 0 || y < 0 || x >= clip_w || y >= clip_h || x >= dst->w || y >= dst->h)
        return;
    p = dst->rgba + ((size_t)y * (size_t)dst->w + (size_t)x) * 4;
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p[3] = 255;
}

/* CreatePen width 2, then MoveTo/LineTo. The end pixel is included; it sits on the core. */
static void draw_line(Pic *dst, int x0, int y0, int x1, int y1, int clip_w, int clip_h)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int guard = 0;

    while (guard++ < 2048) {
        int e2;

        plot(dst, x0, y0, clip_w, clip_h);
        plot(dst, x0, y0 + 1, clip_w, clip_h);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* orig: DrawRound4Bungee (0020:074E) */
static void paint_goal(const Sprite *s, Pic *dst, int clip_w, int clip_h)
{
    /* scene_paint only calls on_paint when visible. */
    if (s->x == 0xFA && s->y == 0x9D)
        return;
    draw_line(dst, s->x, s->y, 0x100, 0xA3, clip_w, clip_h);
}

/* orig: MoveRound4Bungee (0020:06A0) */
static void move_bungee(Sprite *s, int first)
{
    Round4 *r = s->user;
    Sprite *ball;

    (void)first;
    if (s->group == 5) {
        s->visible = 0;
        return;
    }
    ball = r->ball;
    if (!ball || ball->group == 1)
        return;
    if (ball->x <= 0xFA) {
        s->x = ball->x + 0x23;
        s->left = s->x;
        s->right = 0x102;
    } else {
        s->left = 0x100;
        s->x = ball->x + 0x23;
        s->right = s->x;
    }
    if (ball->y <= 0x9D) {
        s->y = ball->y + 0x23;
        s->top = s->y;
        s->bottom = 0xA5;
    } else {
        s->top = 0xA3;
        s->y = ball->y + 0x23;
        s->bottom = s->y;
    }
}

/* orig: MoveRound4Ball (0020:0311) */
static void move_ball(Sprite *s)
{
    Round4 *r = s->user;
    int dx;

    if (s->group != 2) {
        dx = r->cursor_x - r->saved_x;
        r->frame += dx >> 2;
        if (r->frame >= 0x20)
            r->frame = 0;
        else if (r->frame < 0)
            r->frame = 0x1F;
        r->saved_x += dx & ~3;
        r->saved_y = r->cursor_y;
        if (r->frame >= 0 && r->frame < 32 && r->bank[r->frame] && r->bank[32 + r->frame]) {
            s->img = r->bank[r->frame];
            s->mask = r->bank[32 + r->frame];
        }
    }
    if (s->group == 2) {
        if (s->top <= 0 || s->bottom >= CLIP_H || s->left <= 0 || s->right >= FIELD_W) {
            s->dx = -s->dx;
            s->dy = -s->dy;
            s->group = 3;
            s->hit_mask = 0;
        }
    } else if (s->group == 3) {
        if (s->left == 0xDD && s->top == 0x80) {
            if (r->button) {
                s->hit_mask = 0x8000;
                s->dx = k_vel[r->frame][0];
                s->dy = k_vel[r->frame][1];
                s->group = 2;
            } else {
                s->dx = 0;
                s->dy = 0;
                s->group = 1;
                s->hit_mask = 0;
            }
        }
    }
}

static int cross(int ax, int ay, int bx, int by, int cx, int cy)
{
    long v = (long)(bx - ax) * (cy - ay) - (long)(by - ay) * (cx - ax);

    if (v > 0)
        return 1;
    if (v < 0)
        return -1;
    return 0;
}

static int on_seg(int ax, int ay, int bx, int by, int cx, int cy)
{
    int minx = ax < bx ? ax : bx;
    int maxx = ax > bx ? ax : bx;
    int miny = ay < by ? ay : by;
    int maxy = ay > by ? ay : by;

    return cx >= minx && cx <= maxx && cy >= miny && cy <= maxy;
}

static int segs_cross(int ax, int ay, int bx, int by, int cx, int cy, int dx, int dy)
{
    int d1 = cross(ax, ay, bx, by, cx, cy);
    int d2 = cross(ax, ay, bx, by, dx, dy);
    int d3 = cross(cx, cy, dx, dy, ax, ay);
    int d4 = cross(cx, cy, dx, dy, bx, by);

    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return 1;
    if (d1 == 0 && on_seg(ax, ay, bx, by, cx, cy))
        return 1;
    if (d2 == 0 && on_seg(ax, ay, bx, by, dx, dy))
        return 1;
    if (d3 == 0 && on_seg(cx, cy, dx, dy, ax, ay))
        return 1;
    if (d4 == 0 && on_seg(cx, cy, dx, dy, bx, by))
        return 1;
    return 0;
}

static int inside(int x, int y, const int *px, const int *py, int n)
{
    int i;
    int wind = 0;

    for (i = 0; i < n; i++) {
        int j = i + 1 == n ? 0 : i + 1;

        if (py[i] <= y) {
            if (py[j] > y && cross(px[i], py[i], px[j], py[j], x, y) > 0)
                wind++;
        } else if (py[j] <= y && cross(px[i], py[i], px[j], py[j], x, y) < 0)
            wind--;
    }
    return wind != 0;
}

static int polys_overlap(const int *ax, const int *ay, int an, const int *bx, const int *by, int bn)
{
    int i;
    int j;

    for (i = 0; i < an; i++) {
        if (inside(ax[i], ay[i], bx, by, bn))
            return 1;
    }
    for (i = 0; i < bn; i++) {
        if (inside(bx[i], by[i], ax, ay, an))
            return 1;
    }
    for (i = 0; i < an; i++) {
        int i2 = i + 1 == an ? 0 : i + 1;

        for (j = 0; j < bn; j++) {
            int j2 = j + 1 == bn ? 0 : j + 1;

            if (segs_cross(ax[i], ay[i], ax[i2], ay[i2], bx[j], by[j], bx[j2], by[j2]))
                return 1;
        }
    }
    return 0;
}

static void ball_poly(int frame, int left, int top, int *xs, int *ys, int *n_out)
{
    int flip = 0;
    int ndx = 0;
    int ndy = 0;
    int rem;
    int npts;
    int i;
    int j;
    const int *p;

    if (frame < 0)
        frame = 0;
    else if (frame > 31)
        frame = 31;
    switch (frame / 8) {
    case 1:
        flip = 1;
        ndy = -0x47;
        break;
    case 2:
        ndx = -0x47;
        ndy = -0x47;
        break;
    case 3:
        flip = 1;
        ndx = -0x47;
        break;
    default:
        break;
    }
    rem = frame % 8;
    p = k_ball[rem];
    npts = p[0];
    j = 1;
    for (i = 0; i < npts; i++) {
        xs[i] = abs(p[j + flip] + ndy) + left;
        j++;
        ys[i] = abs(p[j - flip] + ndx) + top;
        j++;
    }
    *n_out = npts;
}

static int enemy_poly(const Round4 *r, const Sprite *s, int *xs, int *ys)
{
    int i;
    int n;
    int k;
    const int *p;

    for (i = 0; i < 5; i++) {
        if (s->img == r->bank[66 + i]) {
            p = k_enemy[i];
            n = p[0];
            k = 1;
            for (i = 0; i < n; i++) {
                xs[i] = p[k++] + s->left;
                ys[i] = p[k++] + s->top;
            }
            return n;
        }
    }
    return 0;
}

/* orig: HitTestRound4Ball (0020:045E) */
static int test_ball(Sprite *s, Sprite *other, int first)
{
    Round4 *r = s->user;
    Slot *slot;
    int ax[10], ay[10], bx[12], by[12];
    int an;
    int bn;

    if (!first)
        return 0;
    if (other->left <= 0x10F && other->right >= 0xF1 &&
        other->top <= 0xB2 && other->bottom >= 0x94)
        return 0;
    ball_poly(r->frame, s->left, s->top, ax, ay, &an);
    bn = enemy_poly(r, other, bx, by);
    /* A missing outline still counts, the way a null region leaves the hit set. */
    if (bn != 0 && !polys_overlap(ax, ay, an, bx, by, bn))
        return 0;
    r->left--;
    r->active--;
    slot = other->user;
    free_slot(other);
    if (slot && slot->mark)
        *slot->mark = -1;
    sound_play(0x4269, 1);
    if (other->img == r->bank[67])
        add_score(r, 25);
    else
        add_score(r, 15);
    other->group = 5;
    s->dx = -s->dx;
    s->dy = -s->dy;
    s->group = 3;
    s->hit_mask = 0;
    return 1;
}

/* orig: MovedRound4Ball (0020:0534) */
static void moved_ball(Sprite *s)
{
    Round4 *r = s->user;

    if (s->right <= 0 || s->bottom <= 0 || s->left >= FIELD_W || s->top >= CLIP_H) {
        s->dead = 1;
        r->ball = NULL;
    }
}

/* orig: BurstRound4Ball (0020:0566) */
static void burst_ball(Round4 *r)
{
    Sprite *s;
    int roll;

    s = r->ball;
    if (!s)
        return;
    s->group = 0;
    if (r->goal)
        r->goal->group = 5;
    s->images = &r->bank[0];
    s->masks = &r->bank[32];
    s->frame = r->frame;
    s->frames = 0x20;
    sprite_sync(s);
    s->anim_rate = 1;
    s->anim_count = 0;
    s->step = 2;
    s->anim_mode = 1;
    s->on_move = NULL;
    s->on_moved = moved_ball;
    s->move_ex = NULL;
    s->hit_mask = 0;
    roll = rand() % 0x18;
    s->dx = roll;
    if (s->dx < 0)
        s->dy = s->dx + 0x18;
    else
        s->dy = 0x18 - s->dx;
    if (rand() < 0)
        s->dy = -s->dy;
    s->move_rate = 1;
    r->count++;
    sound_play(0x426C, 1);
}

/* orig: MovedRound4Fungus (0020:08E3) */
static void moved_fungus(Sprite *s)
{
    if (s->group == 5)
        s->visible = 0;
}

/* orig: TickRound4Fungus (0020:08FE) */
static void tick_fungus(Sprite *s)
{
    Slot *slot = s->user;

    if (!s->visible && slot->was_hidden)
        s->dead = 1;
    slot->was_hidden = !s->visible;
}

/* orig: MoveRound4Script (0020:0259) */
static int script_hook(int n, Sprite *s, MoveState *pm, int ready)
{
    Slot *slot = s->user;
    Round4 *r = slot->round;

    (void)ready;
    if (n < 0) {
        switch (n) {
        case -4:
            s->anim_rate = 3;
            s->anim_count = 0;
            s->images = &r->bank[68];
            s->masks = &r->bank[73];
            s->frame = 0;
            s->frames = 3;
            s->step = 1;
            break;
        case -3:
            free_slot(s);
            if (slot->mark)
                *slot->mark = -1;
            r->count++;
            break;
        case -2:
            if (slot->mark)
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
        if (slot->mark && *slot->mark >= n) {
            pm->n_step = 2;
            return 1;
        }
        pm->n_step = 1;
        return 0;
    }
    if (slot->mark)
        (*slot->mark)++;
    return 1;
}

static int major_abs(int dx, int dy)
{
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    return dy > dx ? dy : dx;
}

/* orig: SpawnRound4Fungus (0020:0922) */
static Sprite *spawn_fungus(Round4 *r)
{
    Slot *slot;
    Sprite *s;
    Pic *body;
    int pick;
    int x;
    int y;
    int tx;
    int ty;
    int n_lo;
    int n_hi;
    int span;
    int speed;
    int speed2;
    int tries;
    uint8_t *ip;

    slot = find_slot(r);
    if (!slot || !r->goal || !r->bank[66] || !r->bank[71])
        return NULL;
    body = r->bank[66];
    pick = rand() & 3;
    if (--pick < 0) {
        x = -FIELD_W;
        y = 0;
    } else if (--pick < 0) {
        x = -FIELD_W;
        y = CLIP_H;
    } else {
        if (--pick < 0)
            x = 0;
        else
            x = FIELD_W;
        y = -CLIP_H;
    }
    if (x < 0)
        x = (int)((unsigned)rand() % (unsigned)(-x));
    else if (y == 0)
        x -= body->w;
    if (y < 0)
        y = (int)((unsigned)rand() % (unsigned)(-y));
    else if (y == 0)
        y -= body->h;
    tx = x;
    ty = y;
    for (tries = 0; tries < 64; tries++) {
        if (x < 0x100) {
            n_lo = 4;
            n_hi = 0x100;
        } else {
            n_lo = 0x100;
            n_hi = 0x1FC;
        }
        n_hi -= body->w;
        span = n_hi - n_lo;
        if (span <= 0)
            return NULL;
        tx = (int)((unsigned)rand() % (unsigned)span) + n_lo;
        if (y < 0xA3) {
            n_lo = 4;
            n_hi = 0xA3;
        } else {
            n_lo = 0xA3;
            n_hi = 0x142;
        }
        n_hi -= body->h;
        span = n_hi - n_lo;
        if (span <= 0)
            return NULL;
        ty = (int)((unsigned)rand() % (unsigned)span) + n_lo;
        if (!(tx >= 0xB0 && tx < 0x150 && ty >= 0x63 && ty < 0xE3))
            break;
    }
    speed = major_abs(tx - x, ty - y) >> 3;
    speed2 = (major_abs(0x100 - tx, ty - 0xA3) << 2) / r->div_n;
    s = scene_insert(r->scene, r->goal, body, r->bank[71], x, y, -1);
    if (!s)
        return NULL;
    slot->sprite = s;
    slot->was_hidden = 0;
    slot->mark = alloc_mark(r);
    if (!slot->mark) {
        slot->sprite = NULL;
        s->dead = 1;
        return NULL;
    }
    s->user = slot;
    s->hit_mask = 0x8000;
    s->group = 4;
    s->on_tick = tick_fungus;
    s->on_moved = moved_fungus;
    s->images = &r->bank[66];
    s->masks = &r->bank[71];
    s->frame = 0;
    s->frames = 5;
    sprite_sync(s);
    SDL_memcpy(slot->script, k_cmd, sizeof(k_cmd));
    ip = slot->script;
    ip += ip[1];
    put_i16(ip + 2, speed);
    put_i16(ip + 4, tx);
    put_i16(ip + 6, ty);
    ip += ip[1];
    ip += ip[1];
    ip += ip[1];
    put_i16(ip + 2, r->script_n);
    ip += ip[1];
    ip += ip[1];
    ip += ip[1];
    put_i16(ip + 4, -8);
    put_i16(ip + 6, -8);
    ip += ip[1];
    ip += ip[1];
    put_i16(ip + 2, speed2);
    put_i16(ip + 4, 0xF0);
    put_i16(ip + 6, 0x93);
    sprite_set_script(s, slot->script);
    if (s->move)
        s->move->hook = script_hook;
    r->active++;
    return s;
}

/* orig: SpawnRound4Wave (0020:0BAA) */
static void spawn_wave(Round4 *r, uint32_t now)
{
    int n;

    if (r->wave_n == 0)
        return;
    if (now < r->deadline && r->active != 0)
        return;
    n = r->burst_n;
    while (n-- != 0) {
        if (!spawn_fungus(r))
            r->left--;
    }
    r->wave_n--;
    r->deadline = now + (uint32_t)r->arm_n * 0x3C;
}

/* orig: ArmRound4 (0020:0C95) */
static void arm(Round4 *r)
{
    r->time_left = (long)r->wave * 5 + 0x1E;
    r->time_last = time(NULL);
}

/* orig: SetupRound4 (0020:0C23) */
static void setup(Round4 *r)
{
    int n = r->wave;

    r->active = 0;
    if (n >= 10)
        n = (n & 3) + 6;
    r->script_n = k_script[n];
    r->div_n = k_div[n];
    if (r->wave >= 10)
        r->div_n += r->wave - 9;
    r->wave_n = k_wave[n];
    r->burst_n = k_burst[n];
    r->arm_n = k_arm[n];
    r->left = r->wave_n * r->burst_n;
}

int round4_holding(const Round4 *round)
{
    return round && round->hold;
}

void round4_wave_info(const Round4 *round, int *wave, long *score, long *time_left)
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

void round4_add_score(Round4 *round, long n)
{
    if (round)
        add_score(round, n);
}

/* orig: SetTimeLeft (001D:0446) */
void round4_set_time(Round4 *round, long time_left)
{
    if (!round)
        return;
    round->time_left = time_left;
    if (round->time_left < 0)
        round->time_left = 0;
    round->time_last = time(NULL);
}

/* orig: TickTimeLeft (001D:03AC) / PauseTimeLeft (001D:0425) */
void round4_tick_time(Round4 *round)
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

/* orig: NextPlayWave + ArmRound4 + SetupRound4 after RunStepDialog */
void round4_finish_wave(Round4 *round)
{
    if (!round)
        return;
    round->hold = 0;
    round->wave++;
    if (round->wave > 98)
        round->wave = 98;
    arm(round);
    setup(round);
    round->saved_x = round->cursor_x;
}

/* orig: TickRound4 lose / hold / wave / TickScene (0020:106B) */
static int once(Round4 *r, uint32_t now)
{
    time_t wall;
    int steps;

    if (r->over)
        return 1;
    if (r->hold)
        return 0;
    spawn_wave(r, now);
    if (r->active == 0 && r->left == 0 && r->ball && r->ball->group == 1) {
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
        r->next_tick = now + 3;
        scene_run(r->scene, steps);
        if (r->count == 1)
            burst_ball(r);
        if (!r->ball)
            r->over = 1;
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

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/17000.pic", resources);
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

/* orig: OpenRound4 (0020:0CCE) */
Round4 *round4_new(const char *resources)
{
    Round4 *r = SDL_calloc(1, sizeof(*r));
    int i;

    if (!r)
        return NULL;
    srand((unsigned)time(NULL));
    /* orig: ClearRound4Marks (0020:021E) */
    for (i = 0; i < SLOTS; i++) {
        r->marks[i] = -1;
        r->slots[i].round = r;
    }
    if (!load_bank(resources, r->bank)) {
        round4_free(r);
        return NULL;
    }
    r->scene = scene_new(3);
    if (!r->scene) {
        round4_free(r);
        return NULL;
    }
    r->ball = scene_add(r->scene, r->bank[0], r->bank[32], 0xDD, 0x80, -1);
    if (!r->ball) {
        round4_free(r);
        return NULL;
    }
    r->ball->user = r;
    r->ball->group = 1;
    r->ball->anim_rate = 1;
    r->ball->on_move = move_ball;
    r->ball->on_hit_test = test_ball;
    r->goal = scene_add(r->scene, NULL, NULL, 0xFA, 0x9D, 0);
    if (!r->goal) {
        round4_free(r);
        return NULL;
    }
    r->goal->user = r;
    r->goal->move_ex = move_bungee;
    r->goal->on_paint = paint_goal;
    /* orig: LayRound4 (0020:0800) — mothership core */
    if (r->bank[65])
        scene_add(r->scene, r->bank[65], NULL, 0xFA, 0x9D, 0);
    sound_preload(0x4268, 5);
    arm(r);
    setup(r);
    return r;
}

/* orig: TickRound4 GetMessagePos read (0020:106B) */
void round4_set_cursor(Round4 *round, int screen_x, int screen_y)
{
    if (!round || round->over)
        return;
    round->cursor_x = screen_x;
    round->cursor_y = screen_y;
}

void round4_set_button(Round4 *round, int down)
{
    if (round && !round->over)
        round->button = down ? 1 : 0;
}

/* orig: ClickRound4 (0020:100E) */
void round4_click(Round4 *round)
{
    Sprite *s;

    if (!round || round->over)
        return;
    s = round->ball;
    if (!s || s->group != 1)
        return;
    s->dx = k_vel[round->frame][0];
    s->dy = k_vel[round->frame][1];
    s->move_rate = 1;
    s->group = 2;
    s->hit_mask = 0x8000;
    sound_play(0x4268, 0);
}

/* orig: TickRound4 (0020:106B) */
int round4_timer(Round4 *round)
{
    if (!round)
        return 0;
    round->step_clock++;
    return once(round, round->step_clock);
}

/* orig: TickScene timing prelude (0004:00D9) via GetTicks/17 */
int round4_pump(Round4 *round)
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

/* orig: PaintRound4 (0020:111C) */
void round4_paint(const Round4 *round, Pic *field)
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

/* orig: DrawStatusBar round 4 columns (001D:054F) — title only, no supply */
void round4_status(const Round4 *round, PlayHud *hud)
{
    if (!hud)
        return;
    SDL_zerop(hud);
    if (!round)
        return;
    hud->kind = 1;
    hud->left_label = "Fungus Patrol";
    hud->left_x = 0x20;
    SDL_snprintf(hud->wave, sizeof(hud->wave), "%02d", round->wave + 1);
    format_time(round->time_left, hud->mid_value);
    hud->score = round->score;
}

/* orig: CloseRound4 (0020:112B) */
void round4_free(Round4 *round)
{
    int i;

    if (!round)
        return;
    scene_free(round->scene);
    for (i = 0; i < BANK; i++)
        pic_free(round->bank[i]);
    SDL_free(round);
}
