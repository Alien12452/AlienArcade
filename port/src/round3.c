#include "round3.h"

#include "scene.h"
#include "sound.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

/* Matching source: src/round3.c (segment 27): Quasar Klutzes.
   OpenRound3 (001B:0E19)        round3_new
   ClickRound3 (001B:0F05)       round3_click → throw_banana
   TickRound3 (001B:0F10)        round3_timer / round3_pump / once
   PaintRound3 (001B:102E)       round3_paint
   CloseRound3 (001B:103D)       round3_free
   BackRound3 (001B:0E01)        field clear in play_draw
   ThrowRound3Banana (001B:0963)        fire
   SetupRound3 (001B:04E4)       setup
   SpawnRound3Klutz (001B:057A)       spawn_klutz
   ArmRound3 (001B:0DE9)         arm
   LayRound3 (001B:0D74)         lay
   SpawnRound3Bolt (001B:019F)   spawn_bolt
   SpawnRound3RobotDog (001B:0AF4)   spawn_robot_dog
   SpawnRound3Cloud (001B:0C2E) spawn_cloud
   MoveRound3Script (001B:02CA)  script_hook
   AddRound3Player (001B:0845)   player setup in round3_new
   MoveRound3Player (001B:07CC)  move_player
   MoveRound3Bolt (001B:0124)    move_bolt
   TickRound3Bolt (001B:0141)    tick_bolt
   HitRound3Bolt (001B:0159)     hit_bolt
   MoveRound3Klutz (001B:0355)   move_klutz
   TickRound3Klutz (001B:0382)   tick_klutz
   HitRound3Klutz (001B:03AC)    hit_klutz
   MoveRound3Banana (001B:08D4)    move_banana
   TickRound3Banana (001B:08F0)    tick_banana
   HitRound3Banana (001B:0908)     hit_banana
   MoveRound3RobotDog (001B:0A6B)    move_robot_dog
   TickRound3RobotDog (001B:0A87)    tick_robot_dog
   HitRound3RobotDog (001B:0A9F)     hit_robot_dog
   MoveRound3Cloud (001B:0BC6) move_cloud
   TickRound3Cloud (001B:0BE3) tick_cloud
   HitRound3Cloud (001B:0BFB)  hit_cloud
   AddPlayAmmo / AddPlayScore    add_bananas / round3_add_score
     (001D:0144 / 001D:04B4)
   SetTimeLeft / FormatTime      round3_set_time / format_time
     (001D:0446 / 001D:0737)
   DrawStatusBar (001D:054F)     round3_status */
#include <time.h>

#define FIELD_W 0x200
#define CLIP_H 0x146
#define BANK 85

/* Level rows. Setup caps the index at 9, and left is added, not replaced. */
static const int k_max[10] = { 6, 6, 6, 8, 8, 8, 10, 10, 10, 10 };
static const int k_left[10] = { 6, 8, 8, 10, 10, 12, 12, 14, 16, 18 };
static const int k_bolt_vy[10] = { 5, 5, 5, 6, 6, 7, 7, 8, 8, 9 };
static const int k_arm[10] = { 5, 4, 4, 3, 3, 2, 2, 1, 1, 1 };
static const int k_wave[10] = { 14, 12, 10, 9, 9, 8, 8, 7, 6, 5 };
static const int k_mod[10] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
static const int k_speed[10] = { 6, 7, 8, 8, 8, 8, 8, 8, 8, 8 };
static const int k_shots[10] = { 4, 4, 4, 5, 5, 5, 5, 5, 6, 6 };
static const int k_dx[10] = { 2, 3, 3, 4, 4, 5, 5, 6, 7, 8 };
static const int k_vy[10] = { 4, 5, 5, 5, 6, 6, 6, 7, 7, 8 };
static const int k_period[10] = { 16, 15, 15, 14, 14, 12, 12, 10, 10, 10 };

/* g_r3CmdL. The first byte is a nop so a hook that restarts here skips it. */
static const uint8_t k_cmd_l[108] = {
    0x00, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x04,
    0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00,
    0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
    0x04, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x04, 0x08,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x04, 0x00, 0x00, 0x0A, 0x00
};

static const uint8_t k_cmd_l2[120] = {
    0x00, 0x02, 0x05, 0x04, 0x07, 0x00, 0x04, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x09, 0x04,
    0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00,
    0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
    0x04, 0x08, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x05, 0x00, 0x04, 0x08,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x01, 0x00
};

static const uint8_t k_cmd_r[108] = {
    0x00, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFB, 0xFF, 0x00, 0x00,
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFB, 0xFF, 0x00, 0x00, 0x09, 0x04,
    0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00,
    0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFB, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
    0x04, 0x08, 0x00, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08,
    0x00, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x0C, 0x04, 0x02, 0x00, 0x04, 0x08,
    0x00, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x05, 0x04, 0x00, 0x00, 0x0A, 0x00
};

static const uint8_t k_cmd_r2[120] = {
    0x00, 0x02, 0x05, 0x04, 0x07, 0x00, 0x04, 0x08, 0x00, 0x00, 0xF5, 0xFF, 0x00, 0x00, 0x09, 0x04,
    0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFC, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00,
    0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
    0x04, 0x08, 0x00, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08,
    0x00, 0x00, 0xFA, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x05, 0x00, 0x04, 0x08,
    0x00, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00,
    0xF9, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00,
    0xFD, 0xFF, 0x00, 0x00, 0x0C, 0x04, 0x03, 0x00
};

static const uint8_t k_cmd_hit[36] = {
    0x05, 0x04, 0x0D, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x0F, 0x00, 0x0B, 0x04,
    0x43, 0x1F, 0x06, 0x02, 0x04, 0x08, 0x0A, 0x00, 0x00, 0x00, 0x40, 0x01, 0x01, 0x02, 0x09, 0x04,
    0x01, 0x00, 0x0E, 0x02
};

static const uint8_t k_cmd_a[58] = {
    0x05, 0x04, 0x0C, 0x00, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
    0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x02,
    0x09, 0x04, 0x0F, 0x00, 0x0B, 0x04, 0x43, 0x1F, 0x06, 0x02, 0x04, 0x08, 0x0E, 0x00, 0x00, 0x00,
    0x40, 0x01, 0x01, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0E, 0x02
};

static const uint8_t k_cmd_b[58] = {
    0x05, 0x04, 0x0C, 0x00, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x00, 0x00, 0xF6, 0xFF, 0x00, 0x00,
    0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x00, 0x00, 0xFB, 0xFF, 0x00, 0x00, 0x06, 0x02,
    0x09, 0x04, 0x0F, 0x00, 0x0B, 0x04, 0x43, 0x1F, 0x06, 0x02, 0x04, 0x08, 0x0E, 0x00, 0x00, 0x00,
    0x40, 0x01, 0x01, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0E, 0x02
};

struct Round3 {
    Pic *bank[BANK];
    Scene *scene;
    Sprite *player;
    Sprite *ship;
    int cursor;
    int wave;
    int hold;
    uint32_t hold_now;
    int bananas;
    int bombs;
    int bolts;
    int active;
    int left;
    int max_out;
    int cool;
    int bolt_vy;
    int ships;
    int arm_n;
    int wave_n;
    int mod;
    int speed;
    int shots;
    int shot_max;
    int dx_max;
    int vy;
    int period;
    int delay;
    long score;
    long time_left;
    time_t time_last;
    int over;
    int deadline_set;
    uint32_t deadline;
    int clock_started;
    uint32_t clock;
    uint32_t next_tick;
    int scene_started;
    uint32_t scene_mark;
    uint32_t step_clock;
};

static void spawn_bolt(Round3 *r, Sprite *parent);
static int script_hook(int n, Sprite *s, MoveState *pm, int ready);

/* orig: AddPlayAmmo (001D:0144) */
static void add_bananas(Round3 *r, int n)
{
    r->bananas += n;
    if (r->bananas < 0)
        r->bananas = 0;
}

/* orig: SetRound3Move (001B:0051) */
static void set_script(Sprite *s, const uint8_t *script, int hooked)
{
    sprite_set_script(s, script);
    if (s->move)
        s->move->hook = hooked ? script_hook : NULL;
}

/* orig: MoveRound3Bolt (001B:0124) */
static void move_bolt(Sprite *s)
{
    if (s->top >= CLIP_H) {
        s->visible = 0;
        s->dead = 1;
    }
}

/* orig: TickRound3Bolt (001B:0141) */
static void tick_bolt(Sprite *s)
{
    Round3 *r = s->user;

    if (s->dead)
        r->bolts--;
}

/* orig: HitRound3Bolt (001B:0159) */
static void hit_bolt(Sprite *s, Sprite *other, int first)
{
    Round3 *r = s->user;

    (void)first;
    if (other->group == 9 || other->group == 2 || other->group == 10)
        return;
    s->visible = 0;
    s->dead = 1;
    if (other->group == 1) {
        sound_play(0x1770, 1);
        add_bananas(r, -5);
    }
}

/* orig: SpawnRound3Bolt (001B:019F) */
static void spawn_bolt(Round3 *r, Sprite *parent)
{
    Sprite *s;
    Pic *bolt;
    int side;

    if (r->bolts >= 10)
        return;
    if (!parent->img || parent->img->w <= 0x18)
        return;
    if (parent->x >= 0x1E8)
        return;
    if (parent->group != 4 && parent->group != 3)
        return;
    bolt = r->bank[32];
    if (!bolt || !r->bank[76])
        return;
    side = rand() & 3;
    if (side) {
        int x = parent->group == 4 ? parent->right + 1 : parent->left - bolt->w - 1;

        s = scene_insert(r->scene, r->scene->first, bolt, r->bank[76],
                         x, parent->bottom - bolt->h + 6, -1);
        if (!s)
            return;
        s->hit_mask = 0x8C00;
    } else {
        int x = parent->left + (parent->right - parent->left - bolt->w) / 2;

        s = scene_insert(r->scene, r->scene->first, bolt, r->bank[76],
                         x, parent->bottom + 1, -1);
        if (!s)
            return;
        s->dy = r->bolt_vy;
        s->hit_mask = 0x400;
    }
    s->user = r;
    s->group = 9;
    s->on_moved = move_bolt;
    s->on_hit = hit_bolt;
    s->on_tick = tick_bolt;
    r->bolts++;
}

/* orig: MoveRound3Script (001B:02CA) */
static int script_hook(int n, Sprite *s, MoveState *pm, int ready)
{
    Round3 *r = s->user;

    (void)pm;
    (void)ready;
    switch (n) {
    case 0:
        if (s->group == 5)
            set_script(s, k_cmd_l2, 1);
        break;
    case 1:
        s->group = 3;
        s->hit_mask = 0xB800;
        set_script(s, k_cmd_l, 1);
        spawn_bolt(r, s);
        break;
    case 2:
        if (s->group == 6)
            set_script(s, k_cmd_r2, 1);
        break;
    case 3:
        s->group = 4;
        s->hit_mask = 0xB800;
        set_script(s, k_cmd_r, 1);
        spawn_bolt(r, s);
        break;
    }
    return 1;
}

/* orig: MoveRound3Klutz (001B:0355) */
static void move_klutz(Sprite *s)
{
    if ((s->group == 3 && s->left > FIELD_W) || (s->group == 4 && s->right < 0))
        s->dead = 1;
}

/* orig: TickRound3Klutz (001B:0382) */
static void tick_klutz(Sprite *s)
{
    Round3 *r = s->user;

    if (!s->dead)
        return;
    r->active--;
    if (s->group >= 7) {
        r->left--;
        r->score += 10;
    }
}

/* orig: HitRound3Klutz (001B:03AC) */
static void hit_klutz(Sprite *s, Sprite *other, int first)
{
    Round3 *r = s->user;
    int n;

    (void)first;
    if (other->group == 9) {
        s->group += 4;
        s->hit_mask = 0x4000;
        set_script(s, k_cmd_hit, 0);
        sound_play(0x1F41, 0);
    } else if (s->group <= 4 && other->group == 2) {
        s->group += 2;
        s->hit_mask = 0xA800;
        sound_play(0x1F44, 0);
    } else if (other->group == 1) {
        s->visible = 0;
        s->dead = 1;
        add_bananas(r, -5);
        sound_play(0x1770, 1);
    } else if (other->group >= 3 && other->group <= 8) {
        if (other->group < 5 && s->group < 5)
            return;
        if (s->group < 7 || other->group < 7) {
            if (((s->group & 1) ^ (other->group & 1)) != 0) {
                n = other->x - s->x;
                if (other->group & 1)
                    n = -n;
                if (s->img && s->img->w - 0x10 < n) {
                    if ((s->group - 3) & 1)
                        set_script(s, k_cmd_a, 0);
                    else
                        set_script(s, k_cmd_b, 0);
                    s->group = ((s->group - 3) & 1) + 7;
                    s->hit_mask = 0x4000;
                    r->score += 25;
                    sound_play(0x1F42, 0);
                }
            }
        }
    }
}

/* orig: MoveRound3Player (001B:07CC) */
static void move_player(Sprite *s)
{
    Round3 *r = s->user;
    int limit = FIELD_W - r->bank[41]->w;

    if (r->cursor < 0)
        r->cursor = 0;
    else if (limit < r->cursor)
        r->cursor = limit;
    s->dx = r->cursor - s->left;
    if (s->left < r->cursor && s->dx > 0x30)
        s->dx = 0x30;
    else if (s->left > r->cursor) {
        if (s->dx < -0x30)
            s->dx = -0x30;
    }
}

/* orig: MoveRound3Banana (001B:08D4) */
static void move_banana(Sprite *s)
{
    if (s->bottom <= 0) {
        s->visible = 0;
        s->dead = 1;
    }
}

/* orig: TickRound3Banana (001B:08F0) */
static void tick_banana(Sprite *s)
{
    Round3 *r = s->user;

    if (s->dead)
        r->bombs--;
}

/* orig: HitRound3Banana (001B:0908) */
static void hit_banana(Sprite *s, Sprite *other, int first)
{
    Round3 *r = s->user;

    (void)first;
    if (other->group == 9)
        return;
    r->score += 5;
    s->img = r->bank[40];
    s->mask = r->bank[84];
    s->images = NULL;
    s->masks = NULL;
    sprite_sync(s);
    s->hit_mask = 0;
}

/* orig: ThrowRound3Banana (001B:0963) */
static void throw_banana(Round3 *r)
{
    Sprite *s;
    Pic *bomb;

    if (r->bombs >= 3 || r->bananas == 0 || !r->player)
        return;
    bomb = r->bank[39];
    if (!bomb || !r->bank[83])
        return;
    s = scene_insert(r->scene, r->scene->first, bomb, r->bank[83],
                     r->player->left + (r->bank[41]->w - bomb->w) / 2,
                     r->player->top - bomb->h, -1);
    if (!s)
        return;
    s->user = r;
    s->group = 2;
    s->dy = -10;
    s->on_moved = move_banana;
    s->on_hit = hit_banana;
    s->on_tick = tick_banana;
    s->hit_mask = 0x1000;
    add_bananas(r, -1);
    r->bombs++;
    sound_play(0x1F40, 0);
}

/* orig: MoveRound3RobotDog (001B:0A6B) */
static void move_robot_dog(Sprite *s)
{
    if (s->right <= 0) {
        s->visible = 0;
        s->dead = 1;
    }
}

/* orig: TickRound3RobotDog (001B:0A87) */
static void tick_robot_dog(Sprite *s)
{
    Round3 *r = s->user;

    if (!s->dead)
        return;
    r->ships--;
    if (r->ship == s)
        r->ship = NULL;
}

/* orig: HitRound3RobotDog (001B:0A9F) */
static void hit_robot_dog(Sprite *s, Sprite *other, int first)
{
    Round3 *r = s->user;

    (void)first;
    if (other->group != 9 || !r->bank[35] || !r->bank[79])
        return;
    r->score += 5;
    /* The flying frames stay in images, so the next anim step paints them again. */
    s->img = r->bank[35];
    s->mask = r->bank[79];
    s->left = s->x;
    s->top = s->y;
    s->right = s->x + s->img->w;
    s->bottom = s->y + s->img->h;
}

/* orig: SpawnRound3RobotDog (001B:0AF4) */
static void spawn_robot_dog(Round3 *r)
{
    Sprite *s;
    Pic *img;
    int lane;
    int y;

    if (r->ships != 0 || r->left <= 1 || r->mod == 0)
        return;
    img = r->bank[33];
    if (!img || !r->bank[77])
        return;
    lane = (rand() & 0x7FFF) % r->mod;
    y = lane * 0x46 + 0x5F - img->h;
    s = scene_add(r->scene, img, r->bank[77], FIELD_W, y, -1);
    if (!s)
        return;
    s->user = r;
    s->group = 10;
    s->dx = -r->speed;
    s->images = &r->bank[33];
    s->masks = &r->bank[77];
    s->frame = 0;
    s->frames = 2;
    s->anim_rate = 4;
    s->hit_mask = 0x200;
    s->on_moved = move_robot_dog;
    s->on_hit = hit_robot_dog;
    s->on_tick = tick_robot_dog;
    sprite_sync(s);
    r->ships++;
    r->ship = s;
    sound_play(0x1F46, 0);
}

/* orig: MoveRound3Cloud (001B:0BC6) */
static void move_cloud(Sprite *s)
{
    if (s->top >= CLIP_H) {
        s->visible = 0;
        s->dead = 1;
    }
}

/* orig: TickRound3Cloud (001B:0BE3) */
static void tick_cloud(Sprite *s)
{
    Round3 *r = s->user;

    if (s->dead)
        r->shots--;
}

/* orig: HitRound3Cloud (001B:0BFB) */
static void hit_cloud(Sprite *s, Sprite *other, int first)
{
    Round3 *r = s->user;

    (void)first;
    if (other->group != 1)
        return;
    s->visible = 0;
    s->dead = 1;
    add_bananas(r, -2);
    sound_play(0x1770, 1);
}

/* orig: SpawnRound3Cloud (001B:0C2E) */
static void spawn_cloud(Round3 *r)
{
    Sprite *s;
    Pic *img;

    if (r->ships == 0 || r->delay-- != 0)
        return;
    if (r->shots < r->shot_max && r->ship && r->player &&
        (img = r->bank[36]) != NULL && r->bank[80]) {
        s = scene_insert(r->scene, r->scene->first, img, r->bank[80],
                         r->ship->x + 0x28, r->ship->y + 5, -1);
        if (s) {
            s->user = r;
            s->group = 11;
            s->images = &r->bank[36];
            s->masks = &r->bank[80];
            s->frame = 0;
            s->frames = 3;
            sprite_sync(s);
            s->dy = r->vy;
            if (r->dx_max != 0 && r->vy != 0) {
                int denom = (r->player->y - s->y + r->bank[41]->h / 2) / r->vy;

                s->dx = -(s->x - r->player->x + r->bank[41]->w / 2);
                if (denom != 0)
                    s->dx /= denom;
                else
                    s->dx = 0;
                if (s->dx > r->dx_max)
                    s->dx = r->dx_max;
                else if (s->dx < -r->dx_max)
                    s->dx = -r->dx_max;
            }
            s->hit_mask = 0x100;
            s->on_moved = move_cloud;
            s->on_hit = hit_cloud;
            s->on_tick = tick_cloud;
            r->shots++;
            sound_play(0x1F45, 0);
        }
    }
    r->delay = r->period;
}

/* orig: LayRound3 (001B:0D74) */
static void lay(Round3 *r)
{
    int i;

    if (r->bank[43])
        scene_add(r->scene, r->bank[43], NULL, 0, 0, 0);
    for (i = 0; i < 3; i++) {
        if (r->bank[42])
            scene_add(r->scene, r->bank[42], NULL, 0, i * 0x46 + 0x5F, 0);
    }
}

/* orig: ArmRound3 (001B:0DE9) */
static void arm(Round3 *r)
{
    r->time_left = (long)r->wave * 10 + 0x2D;
    r->time_last = time(NULL);
}

/* orig: SetupRound3 (001B:04E4) */
static void setup(Round3 *r)
{
    int n = r->wave;

    r->cool = 0;
    if (n >= 10)
        n = 9;
    r->left += k_left[n];
    r->max_out = k_max[n];
    r->bolt_vy = k_bolt_vy[n];
    r->arm_n = k_arm[n];
    r->wave_n = k_wave[n];
    r->mod = k_mod[n];
    r->speed = k_speed[n];
    r->shot_max = k_shots[n];
    r->dx_max = k_dx[n];
    r->vy = k_vy[n];
    r->delay = r->period = k_period[n];
}

/* orig: SpawnRound3Klutz (001B:057A) */
static void spawn_klutz(Round3 *r)
{
    int an_count[3];
    int an_flat[6];
    int n_used = 0;
    int n_span;
    int n_lane;
    int n_side;
    int i;
    Sprite *s;

    if (r->active < 3 && r->cool > 0x14)
        r->cool = (rand() & 0x7FFF) % 12 + 8;
    if (r->cool != 0)
        r->cool--;
    if (r->cool != 0)
        return;
    if (r->active >= r->max_out || r->left <= r->active || r->bananas == 0)
        return;
    for (i = 0; i < 3; i++) {
        an_count[i] = 0;
        an_flat[i] = 0;
        an_flat[i + 3] = 0;
    }
    n_span = r->bank[0] ? r->bank[0]->w * 2 : 0;
    for (s = r->scene->first; s; s = s->next) {
        if (s->group < 3 || s->group > 8)
            continue;
        i = 0;
        if (s->top > 0x32) {
            i++;
            if (s->top > 0x78)
                i++;
        }
        an_count[i]++;
        if (s->right <= n_span) {
            an_flat[i * 2] = 1;
            n_used++;
        } else if (s->left >= FIELD_W - n_span) {
            an_flat[i * 2 + 1] = 1;
            n_used++;
        }
    }
    if (n_used != 6 && r->bank[0] && r->bank[16] && r->bank[44] && r->bank[60]) {
        n_lane = 0;
        if (an_count[0] != 1) {
            n_lane++;
            if (an_count[1] != 1) {
                n_lane++;
                if (an_count[2] != 1)
                    n_lane++;
            }
        }
        if (n_lane == 3) {
            do {
                n_lane = (rand() & 0x7FFF) % 3;
                if (an_flat[n_lane * 2] == 0 || an_flat[n_lane * 2 + 1] == 0)
                    break;
            } while (1);
        }
        do
            n_side = rand() & 1;
        while (an_flat[n_lane * 2 + 1 - n_side] != 0);
        n_lane = n_lane * 0x46 + 0x32;
        if (n_side != 0)
            s = scene_insert(r->scene, r->scene->first, r->bank[0], r->bank[44],
                             -0x1E, n_lane, -1);
        else
            s = scene_insert(r->scene, r->scene->first, r->bank[16], r->bank[60],
                             FIELD_W, n_lane, -1);
        if (s) {
            s->user = r;
            s->hit_mask = 0xB800;
            s->on_move = move_klutz;
            s->on_tick = tick_klutz;
            s->on_hit = hit_klutz;
            s->frames = 0x10;
            s->frame = 0;
            if (n_side != 0) {
                set_script(s, k_cmd_l, 1);
                s->images = &r->bank[0];
                s->masks = &r->bank[44];
                s->group = 3;
            } else {
                set_script(s, k_cmd_r, 1);
                s->images = &r->bank[16];
                s->masks = &r->bank[60];
                s->group = 4;
            }
            sprite_sync(s);
            r->active++;
        }
    }
    r->cool = 0x28;
}

int round3_holding(const Round3 *round)
{
    return round && round->hold;
}

void round3_wave_info(const Round3 *round, int *wave, long *score, long *time_left)
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

/* orig: AddPlayScore (001D:04B4) */
void round3_add_score(Round3 *round, long n)
{
    if (!round)
        return;
    round->score += n;
    if (round->score < 0)
        round->score = 0;
}

/* orig: SetTimeLeft (001D:0446) */
void round3_set_time(Round3 *round, long time_left)
{
    if (!round)
        return;
    round->time_left = time_left;
    if (round->time_left < 0)
        round->time_left = 0;
    round->time_last = time(NULL);
}

/* orig: TickTimeLeft (001D:03AC) / PauseTimeLeft (001D:0425) */
void round3_tick_time(Round3 *round)
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

/* orig: NextPlayWave + ArmRound3 + SetupRound3 after RunStepDialog */
void round3_finish_wave(Round3 *round, uint32_t now)
{
    if (!round)
        return;
    if (now == 0)
        now = round->hold_now;
    round->hold = 0;
    round->wave++;
    if (round->wave > 98)
        round->wave = 98;
    arm(round);
    setup(round);
    add_bananas(round, round->left);
    round->deadline = now + (uint32_t)round->arm_n * 0x3C;
    round->deadline_set = 1;
    spawn_klutz(round);
}

/* orig: TickRound3 lose / hold / ship deadline / TickScene (001B:0F10) */
static int once(Round3 *r, uint32_t now)
{
    time_t wall;
    int steps;

    if (r->over)
        return 1;
    if (r->hold)
        return 0;
    if (r->active <= 1 && r->left <= 1 && r->bombs == 0 && r->shots == 0) {
        r->hold = 1;
        r->hold_now = now;
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
        spawn_klutz(r);
        if (now >= r->deadline) {
            spawn_robot_dog(r);
            r->deadline = now + (uint32_t)r->wave_n * 0x3C;
        }
        spawn_cloud(r);
        r->next_tick = now + 3;
        scene_run(r->scene, steps);
    }
    wall = time(NULL);
    r->time_left -= (long)(wall - r->time_last);
    if (r->time_left < 0)
        r->time_left = 0;
    r->time_last = wall;
    if (r->bananas == 0 && r->bombs == 0)
        r->over = 1;
    return r->over;
}

static int load_bank(const char *resources, Pic **bank)
{
    char path[1024];
    size_t nbytes = 0;
    void *file;
    int i;

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/8000.pic", resources);
    file = SDL_LoadFile(path, &nbytes);
    if (!file)
        return 0;
    for (i = 0; i < BANK; i++) {
        bank[i] = pic_decode_ppic(file, nbytes, i);
        if (!bank[i]) {
            SDL_free(file);
            return 0;
        }
        /* DecImageBottoms trims one row off the klutz frames, not the last of each set. */
        if ((i <= 0x0E || (i >= 0x10 && i <= 0x1E)) && bank[i]->h > 1)
            bank[i]->h--;
    }
    SDL_free(file);
    return 1;
}

/* orig: OpenRound3 (001B:0E19) */
Round3 *round3_new(const char *resources)
{
    Round3 *r = SDL_calloc(1, sizeof(*r));

    if (!r)
        return NULL;
    srand((unsigned)time(NULL));
    r->bananas = 20;
    if (!load_bank(resources, r->bank)) {
        round3_free(r);
        return NULL;
    }
    r->scene = scene_new(3);
    if (!r->scene) {
        round3_free(r);
        return NULL;
    }
    r->player = scene_add(r->scene, r->bank[41], NULL,
                          (FIELD_W - r->bank[41]->w) / 2, 0x144 - r->bank[41]->h, 0);
    if (!r->player) {
        round3_free(r);
        return NULL;
    }
    r->player->user = r;
    r->player->group = 1;
    r->player->hit_mask = 0x4D00;
    r->player->on_move = move_player;
    r->cursor = r->player->left;
    sound_preload(0x1770, 1);
    sound_preload(0x1F40, 7);
    lay(r);
    arm(r);
    setup(r);
    /* Gate dwell after OpenRound3 counts toward the first ship. */
    r->deadline = (uint32_t)(SDL_GetTicks() / 17) + (uint32_t)r->arm_n * 0x3C;
    r->deadline_set = 1;
    return r;
}

/* orig: TickRound3 LOWORD(GetMessagePos) read (001B:0F10) */
void round3_set_cursor(Round3 *round, int screen_x)
{
    if (round && !round->over)
        round->cursor = screen_x;
}

/* orig: ClickRound3 (001B:0F05) */
void round3_click(Round3 *round)
{
    if (round && !round->over)
        throw_banana(round);
}

/* orig: TickRound3 (001B:0F10) */
int round3_timer(Round3 *round)
{
    if (!round)
        return 0;
    round->step_clock++;
    return once(round, round->step_clock);
}

/* orig: TickScene timing prelude (0004:00D9) via GetTicks/17 */
int round3_pump(Round3 *round)
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

/* orig: PaintRound3 (001B:102E) */
void round3_paint(const Round3 *round, Pic *field)
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

/* orig: DrawStatusBar round 3 columns (001D:054F) */
void round3_status(const Round3 *round, PlayHud *hud)
{
    if (!hud)
        return;
    SDL_zerop(hud);
    if (!round)
        return;
    hud->kind = 0;
    hud->left_label = "Bananas:";
    hud->left_x = 0x17;
    SDL_snprintf(hud->left_value, sizeof(hud->left_value), "%03d", round->bananas);
    SDL_snprintf(hud->wave, sizeof(hud->wave), "%02d", round->wave + 1);
    format_time(round->time_left, hud->mid_value);
    hud->score = round->score;
}

/* orig: CloseRound3 (001B:103D) */
void round3_free(Round3 *round)
{
    int i;

    if (!round)
        return;
    scene_free(round->scene);
    for (i = 0; i < BANK; i++)
        pic_free(round->bank[i]);
    SDL_free(round);
}
