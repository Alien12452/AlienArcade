/* Sprite list and move scripts.
   Matching: src/scene.c (seg 4), src/sprite.c (seg 6), src/move.c (seg 12),
   and painting akin to DrawScene (src/draw.c). Painting always redraws;
   the original tracked dirty rectangles and came out the same. */

#ifndef SCENE_H
#define SCENE_H

#include "pic.h"

#include <stdint.h>

typedef struct Sprite Sprite;
typedef struct MoveState MoveState;
typedef struct Scene Scene;

typedef int (*MoveHook)(int n, Sprite *s, MoveState *pm, int ready);
typedef void (*SpriteFn)(Sprite *s);
typedef void (*SpriteHitFn)(Sprite *s, Sprite *other, int first);
typedef void (*SpriteMoveFn)(Sprite *s, int first);
/* Return 0 to reject a rectangle hit. NULL accepts it. */
typedef int (*SpriteTestFn)(Sprite *s, Sprite *other, int first);
typedef void (*SpritePaintFn)(const Sprite *s, Pic *dst, int clip_w, int clip_h);

struct MoveState {
    const uint8_t *pc;
    int pos;
    int n_step;
    int n_steps;
    int32_t x, y, dx, dy;
    MoveHook hook;
    /* Shared by every sprite on one script, the way R5POS.pMark is. */
    int *mark;
};

struct Sprite {
    int group;
    /* SPRITE.unk02. Round 5 stores a part colour, a lid column, or a roll. */
    int kind;
    int move_count;
    int move_rate;
    int dx, dy;
    int x, y;
    int left, top, right, bottom;
    int visible;
    Pic *img;
    Pic *mask;
    Pic **images;
    Pic **masks;
    uint32_t hit_mask;
    int hit_done;
    int hold;
    int dead;
    int anim_count;
    int anim_rate;
    int frame;
    int frames;
    int step;
    int anim_mode;
    int draw_mode;
    MoveState *move;
    SpriteFn on_move;
    SpriteMoveFn move_ex;
    SpriteFn on_moved;
    SpriteHitFn on_hit;
    SpriteTestFn on_hit_test;
    SpritePaintFn on_paint;
    SpriteFn on_tick;
    void *user;
    Sprite *prev;
    Sprite *next;
};

struct Scene {
    int rate;
    int started;
    uint32_t last_tick;
    Sprite *first;
    Sprite *last;
};

Scene *scene_new(int rate);
void scene_free(Scene *sc);
Sprite *scene_add(Scene *sc, Pic *img, Pic *mask, int x, int y, int draw_mode);
/* InsertSprite. before == NULL appends, the same as scene_add. */
Sprite *scene_insert(Scene *sc, Sprite *before, Pic *img, Pic *mask, int x, int y, int draw_mode);
void sprite_sync(Sprite *s);
void sprite_set_script(Sprite *s, const uint8_t *script);
/* How many scene steps this timer fire should run: 1..3, or 0 if the
   17ms timer has not fired. InitScene's rate divides the catch-up. */
int scene_due(Scene *sc);
void scene_run(Scene *sc, int steps);
void scene_paint(const Scene *sc, Pic *dst, int clip_w, int clip_h);

#endif
