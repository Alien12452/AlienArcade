#include "scene.h"

#include "sound.h"

#include <SDL3/SDL.h>

/* Matching sources:
   InitScene / TickScene / FreeScene / PaintScene / RectsOverlap
     (0004:0007 / 0004:00D9 / 0004:0525 / 0004:055F / 0004:05A0)  scene_*
   NewSprite / InsertSprite / AddSprite / FreeSprite / UnlinkSprite
     (0006:0000 / 0006:002E / 0006:0082 / 0006:0118 / 0006:00C5)
   UpdateSpriteRect (0006:0144)                   sprite_sync
   MoveSprite / AllocSpriteMove
     (000C:0000 / 000C:038F)  move_sprite / sprite_set_script
   DrawScene / CopyBlitImage                      scene_paint */

/* orig: InitScene (0004:0007) — rate only; no HWND / backbuffer here */
Scene *scene_new(int rate)
{
    Scene *sc = SDL_calloc(1, sizeof(*sc));
    if (!sc)
        return NULL;
    sc->rate = rate > 0 ? rate : 1;
    return sc;
}

static void move_sprite(Sprite *s, int first);

/* orig: FreeSprite (0006:0118) / UnlinkSprite (0006:00C5) */
static void free_sprite(Scene *sc, Sprite *s)
{
    if (sc->first == s)
        sc->first = s->next;
    if (sc->last == s)
        sc->last = s->prev;
    if (s->next)
        s->next->prev = s->prev;
    if (s->prev)
        s->prev->next = s->next;
    SDL_free(s->move);
    SDL_free(s);
}

/* orig: FreeScene (0004:0525) */
void scene_free(Scene *sc)
{
    if (!sc)
        return;
    while (sc->first)
        free_sprite(sc, sc->first);
    SDL_free(sc);
}

/* orig: UpdateSpriteRect (0006:0144) — also refreshes img/mask from the frame */
void sprite_sync(Sprite *s)
{
    if (s->images && s->frame >= 0)
        s->img = s->images[s->frame];
    if (s->masks && s->frame >= 0)
        s->mask = s->masks[s->frame];
    s->left = s->x;
    s->top = s->y;
    s->right = s->x + (s->img ? s->img->w : 0);
    s->bottom = s->y + (s->img ? s->img->h : 0);
}

/* orig: NewSprite (0006:0000) */
static Sprite *make_sprite(Pic *img, Pic *mask, int x, int y, int draw_mode)
{
    Sprite *s = SDL_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->visible = 1;
    s->hold = 1;
    s->move_rate = 1;
    s->step = 1;
    s->img = img;
    s->mask = mask;
    s->x = x;
    s->y = y;
    s->draw_mode = draw_mode;
    sprite_sync(s);
    return s;
}

static void link_before(Scene *sc, Sprite *s, Sprite *before)
{
    if (!before) {
        if (sc->last) {
            sc->last->next = s;
            s->prev = sc->last;
        } else
            sc->first = s;
        sc->last = s;
        return;
    }
    s->next = before;
    s->prev = before->prev;
    before->prev = s;
    if (s->prev)
        s->prev->next = s;
    else
        sc->first = s;
}

/* orig: InsertSprite (0006:002E) */
Sprite *scene_insert(Scene *sc, Sprite *before, Pic *img, Pic *mask, int x, int y, int draw_mode)
{
    Sprite *s = make_sprite(img, mask, x, y, draw_mode);
    if (!s)
        return NULL;
    link_before(sc, s, before);
    return s;
}

/* orig: AddSprite (0006:0082) */
Sprite *scene_add(Scene *sc, Pic *img, Pic *mask, int x, int y, int draw_mode)
{
    return scene_insert(sc, NULL, img, mask, x, y, draw_mode);
}

/* orig: AllocSpriteMove (000C:038F) — attach a move-script block; clears hook/mark */
void sprite_set_script(Sprite *s, const uint8_t *script)
{
    if (!s->move) {
        s->move = SDL_calloc(1, sizeof(*s->move));
        if (!s->move)
            return;
    }
    s->move->pc = script;
    s->move->pos = 0;
    s->move->n_step = 0;
    s->move->n_steps = 0;
    s->move->hook = NULL;
    s->move->mark = NULL;
    s->move_ex = move_sprite;
}

static int get_i16(const uint8_t *p)
{
    return (int16_t)(uint16_t)(p[0] | (p[1] << 8));
}

static void shift_by(Sprite *s, int dx, int dy)
{
    s->x += dx;
    s->y += dy;
    s->left += dx;
    s->right += dx;
    s->top += dy;
    s->bottom += dy;
}

/* MoveSprite. A command's length byte is how far the cursor moves when
   the command finishes. Commands that set cont run in the same step as
   the one that reached them; opcode 10 restarts the script. */
/* orig: MoveSprite (000C:0000) */
static void move_sprite(Sprite *s, int first)
{
    MoveState *pm = s->move;
    int ready = 1;
    int cont = 1;
    const uint8_t *ip;

    (void)first;
    if (!pm || !pm->pc)
        return;
    if (pm->n_step != pm->n_steps) {
        pm->n_step++;
        ready = 0;
    }
    ip = pm->pc;
    while (cont) {
        int op;

        ip = pm->pc + pm->pos;
        op = ip[0];
        cont = 0;
        switch (op) {
        case 0:
            cont = 1;
            break;
        case 1:
            s->visible = 0;
            cont = 1;
            break;
        case 2:
            s->visible = 1;
            cont = 1;
            break;
        case 3:
        case 4: {
            int32_t nx;
            int32_t ny;
            int cx;

            if (ready) {
                pm->n_steps = get_i16(ip + 2);
                pm->x = (int32_t)s->x << 16;
                pm->y = (int32_t)s->y << 16;
                nx = get_i16(ip + 4);
                ny = get_i16(ip + 6);
                if (op == 3) {
                    nx -= s->x;
                    ny -= s->y;
                }
                if (pm->n_steps) {
                    pm->n_step = 1;
                    pm->dx = (nx << 16) / pm->n_steps;
                    pm->dy = (ny << 16) / pm->n_steps;
                } else {
                    pm->n_step = 0;
                    pm->dx = nx << 16;
                    pm->dy = ny << 16;
                    cont = 1;
                }
            }
            pm->x += pm->dx;
            cx = (int)((pm->x + 0x8000) >> 16) - s->x;
            shift_by(s, cx, 0);
            pm->y += pm->dy;
            cx = (int)((pm->y + 0x8000) >> 16) - s->y;
            shift_by(s, 0, cx);
            break;
        }
        case 5:
        case 6:
        case 7: {
            int n;

            if (op == 5)
                n = get_i16(ip + 2);
            else {
                n = s->frame;
                if (op == 6)
                    n += s->step;
                else
                    n -= s->step;
                if (s->anim_mode == 0) {
                    if (n < 0 || s->frames <= n) {
                        n -= s->step;
                        s->step = -s->step;
                    }
                } else if (s->anim_mode == 1) {
                    if (n < 0)
                        n += s->frames;
                    else if (s->frames <= n)
                        n -= s->frames;
                }
            }
            s->frame = n;
            sprite_sync(s);
            cont = 1;
            break;
        }
        case 8:
            s->draw_mode = get_i16(ip + 2);
            cont = 1;
            break;
        case 9:
            if (ready) {
                pm->n_step = 1;
                pm->n_steps = get_i16(ip + 2);
                if (pm->n_steps == 0) {
                    pm->n_step = 0;
                    break;
                }
            }
            break;
        case 10:
            pm->pos = 0;
            cont = 1;
            break;
        case 11:
        case 15:
            /* PlaySfx / PlaySoundEx. The extra word on opcode 15 is unused. */
            sound_play(get_i16(ip + 2), 0);
            cont = 1;
            break;
        case 12:
            if (pm->hook) {
                cont = pm->hook(get_i16(ip + 2), s, pm, ready);
                ip = pm->pc + pm->pos;
            }
            break;
        case 13:
        case 14:
            s->dead = 1;
            break;
        default:
            break;
        }
        if (cont) {
            pm->pos += ip[1];
            ready = 1;
        }
    }
    if (pm->n_step == pm->n_steps)
        pm->pos += ip[1];
}

/* orig: RectsOverlap (0004:05A0) */
static int rects_overlap(const Sprite *a, const Sprite *b)
{
    return a->left <= b->right && b->left <= a->right &&
           a->top <= b->bottom && b->top <= a->bottom;
}

/* orig: TickScene (0004:00D9) — one move/anim slice */
static void step_sprite(Sprite *s, int first)
{
    if (s->dead)
        return;
    s->move_count++;
    if (s->move_count == s->move_rate) {
        s->move_count = 0;
        if (s->on_move)
            s->on_move(s);
        if (s->move_ex)
            s->move_ex(s, first);
        else if (!s->hold) {
            if (s->dx || s->dy)
                shift_by(s, s->dx, s->dy);
        }
        if (s->on_moved)
            s->on_moved(s);
    }
    if (s->anim_rate) {
        s->anim_count++;
        if (s->anim_count == s->anim_rate) {
            int n = s->frame;

            s->anim_count = 0;
            n += s->step;
            if (s->anim_mode == 0) {
                if (n < 0 || s->frames <= n) {
                    n -= s->step;
                    s->step = -s->step;
                }
            } else if (s->anim_mode == 1) {
                if (n < 0)
                    n += s->frames;
                else if (s->frames <= n)
                    n -= s->frames;
            }
            s->frame = n;
            sprite_sync(s);
        }
    }
    s->hold = 0;
}

/* orig: TickScene (0004:00D9) — catch-up steps, then one tick/free pass */
void scene_run(Scene *sc, int steps)
{
    int i;
    Sprite *s;
    Sprite *next;

    for (i = 0; i < steps; i++) {
        for (s = sc->first; s; s = s->next)
            step_sprite(s, i == 0);
        for (s = sc->first; s; s = s->next)
            s->hit_done = s->dead;
        for (s = sc->first; s; s = s->next) {
            Sprite *other;

            if (s->hit_done)
                continue;
            for (other = s->next; other; other = other->next) {
                int hit;

                if (other->hit_done || s->group == other->group)
                    continue;
                if (!s->visible || !other->visible)
                    continue;
                if ((s->hit_mask & other->hit_mask) == 0)
                    continue;
                if (!rects_overlap(s, other))
                    continue;
                hit = 1;
                /* other->lpfnHitTest is nested under s->lpfnHitTest in TickScene */
                if (s->on_hit_test) {
                    if (!s->on_hit_test(s, other, 1))
                        hit = 0;
                    else if (other->on_hit_test && !other->on_hit_test(other, s, 0))
                        hit = 0;
                }
                if (!hit)
                    continue;
                if (s->on_hit)
                    s->on_hit(s, other, 1);
                if (other->on_hit)
                    other->on_hit(other, s, 0);
                other->hit_done = 1;
            }
        }
    }
    for (s = sc->first; s; s = next) {
        next = s->next;
        if (s->on_tick)
            s->on_tick(s);
        if (s->dead)
            free_sprite(sc, s);
    }
}

/* orig: TickScene (0004:00D9) prelude — GetTickCount/17 catch-up */
int scene_due(Scene *sc)
{
    uint32_t now = (uint32_t)(SDL_GetTicks() / 17);
    int delta;
    int steps;

    if (!sc->started) {
        sc->started = 1;
        sc->last_tick = now;
        return 1;
    }
    delta = (int)(now - sc->last_tick);
    if (delta <= 0)
        return 0;
    steps = delta / sc->rate;
    if (steps < 1)
        steps = 1;
    else if (steps > 3)
        steps = 3;
    sc->last_tick = now;
    return steps;
}

/* orig: PaintScene (0004:055F) / DrawScene (000B:0000) — visible only; lpfnDraw exclusive */
void scene_paint(const Scene *sc, Pic *dst, int clip_w, int clip_h)
{
    const Sprite *s;

    /* The last sprite added is drawn first, so the oldest one is on top. */
    for (s = sc->last; s; s = s->prev) {
        if (!s->visible)
            continue;
        if (s->on_paint) {
            s->on_paint(s, dst, clip_w, clip_h);
            continue;
        }
        if (!s->img)
            continue;
        if (s->draw_mode < 0 && s->mask)
            pic_blit_mask(dst, s->x, s->y, s->img, s->mask, 0, 0, clip_w, clip_h);
        else {
            int h = s->img->h;
            if (s->y >= clip_h)
                continue;
            if (s->y + h > clip_h)
                h = clip_h - s->y;
            pic_blit_mode(dst, s->x, s->y, s->img, s->img->w, h, s->draw_mode);
        }
    }
}
