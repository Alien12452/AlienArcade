#include "intro.h"

#include "pic.h"
#include "scene.h"
#include "select.h"

#include <SDL3/SDL.h>

/* Matching source: src/intro.c (segment 34). Cross-refs:
   HookStart (0022:01B3)              hook_start
   LayStartSprites (0022:06C7)        lay_start_sprites / AddStart*
   OpenStart (0022:0828)              intro_new (WM_INITDIALOG path)
   CloseStart (0022:08F7)             intro_free
   StartWndProc (0022:0782)           intro_tick / intro_draw paint path
   BackStart (0022:0732)              background blit in intro_draw
   StartDlgProc (0022:093A)           intro_key / press / release */

#define VIEW_W 640
#define VIEW_H 480
#define FIELD_W 0x200
#define FIELD_H 0x156
#define BANK 48

static const uint8_t k_idle[] = { 0x00, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0A, 0x00 };

static const uint8_t k_a[] = {
    0x0C, 0x04, 0x0F, 0x00, 0x0B, 0x04, 0xA0, 0x0F, 0x03, 0x08, 0x10, 0x00,
    0xB7, 0x01, 0x70, 0x00, 0x0C, 0x04, 0x10, 0x00, 0x09, 0x04, 0x06, 0x00,
    0x03, 0x08, 0x10, 0x00, 0x70, 0x01, 0x46, 0x00, 0x0C, 0x04, 0x00, 0x00,
    0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02
};

static const uint8_t k_b[] = { 0x0C, 0x04, 0x01, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x0B, 0x00, 0x01, 0x02, 0x0E, 0x02 };
static const uint8_t k_c[] = { 0x0C, 0x04, 0x02, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x0B, 0x00, 0x01, 0x02, 0x0E, 0x02 };
static const uint8_t k_d[] = { 0x0C, 0x04, 0x03, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x0B, 0x00, 0x01, 0x02, 0x0E, 0x02 };

static const uint8_t k_e[] = {
    0x0C, 0x04, 0x04, 0x00, 0x09, 0x04, 0x08, 0x00, 0x04, 0x08, 0x10, 0x00, 0x00, 0x00, 0xC6, 0xFF,
    0x09, 0x04, 0x08, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x10, 0x00, 0x0C, 0x04, 0x00, 0x00,
    0x09, 0x04, 0x08, 0x00, 0x06, 0x02, 0x09, 0x04, 0x10, 0x00, 0x07, 0x02, 0x09, 0x04, 0x08, 0x00,
    0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x10, 0x00,
    0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x08, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x08, 0x00,
    0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x08, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x01, 0x00,
    0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x08, 0x00,
    0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x08, 0x00,
    0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x08, 0x00, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00,
    0x07, 0x02, 0x09, 0x04, 0x14, 0x00, 0x06, 0x02, 0x06, 0x02, 0x04, 0x08, 0x04, 0x00, 0xD0, 0xFF,
    0x00, 0x00, 0x07, 0x02, 0x07, 0x02, 0x04, 0x08, 0x04, 0x00, 0xD0, 0xFF, 0x00, 0x00, 0x06, 0x02,
    0x06, 0x02, 0x04, 0x08, 0x04, 0x00, 0xD0, 0xFF, 0x00, 0x00, 0x07, 0x02, 0x07, 0x02, 0x04, 0x08,
    0x04, 0x00, 0xD0, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x06, 0x02, 0x04, 0x08, 0x04, 0x00, 0xD0, 0xFF,
    0x00, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0E, 0x02
};

static const uint8_t k_f[] = {
    0x0C, 0x04, 0x05, 0x00, 0x06, 0x02, 0x02, 0x02, 0x0C, 0x04, 0x06, 0x00, 0x01, 0x02,
    0x0C, 0x04, 0x07, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x08, 0x00, 0x01, 0x02, 0x0C, 0x04,
    0x09, 0x00, 0x07, 0x02, 0x02, 0x02, 0x0C, 0x04, 0x0A, 0x00, 0x01, 0x02, 0x0E, 0x02
};

static const uint8_t k_g[] = { 0x0C, 0x04, 0x0C, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x11, 0x00, 0x01, 0x02, 0x0E, 0x02 };
static const uint8_t k_h[] = { 0x0C, 0x04, 0x0D, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x11, 0x00, 0x01, 0x02, 0x0E, 0x02 };

static const uint8_t k_i[] = {
    0x0C, 0x04, 0x0E, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x03, 0x08, 0x0A, 0x00, 0x27, 0x00, 0x9C, 0x00,
    0x03, 0x08, 0x06, 0x00, 0x70, 0x00, 0xA7, 0x00, 0x03, 0x08, 0x07, 0x00, 0xC8, 0x00, 0xA7, 0x00,
    0x03, 0x08, 0x09, 0x00, 0x2D, 0x01, 0x9D, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x03, 0x08, 0x09, 0x00,
    0x86, 0x01, 0x86, 0x00, 0x03, 0x08, 0x05, 0x00, 0xB8, 0x01, 0x72, 0x00, 0x01, 0x02, 0x0E, 0x02
};

static const uint8_t k_j[] = { 0x0C, 0x04, 0x12, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02 };
static const uint8_t k_k[] = { 0x0C, 0x04, 0x13, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02 };
static const uint8_t k_l[] = { 0x0C, 0x04, 0x14, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02 };

static const uint8_t k_m[] = {
    0x0C, 0x04, 0x15, 0x00, 0x02, 0x02, 0x03, 0x08, 0x08, 0x00, 0x2E, 0x00, 0x4A, 0x01,
    0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02
};

/* The director. Hook -3 is the cue that the animation is over. */
static const uint8_t k_n[] = {
    0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00,
    0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00,
    0x0C, 0x04, 0x0B, 0x00, 0x09, 0x04, 0x0C, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x1E, 0x00,
    0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x1E, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0x11, 0x00,
    0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00,
    0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0xF0, 0x00,
    0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0xFD, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0E, 0x02
};

struct Intro {
    Scene *scene;
    Pic *bank[BANK];
    Pic *canvas;
    SDL_Texture *frame;
    SDL_Texture *field;
    int marks[16];
    int *mark;
    int done;
    int started;
    int tracking;
    uint32_t last_ms;
    int x;
    int y;
};

static void set_start_move(Sprite *s, const uint8_t *script, MoveHook hook, int *mark);

/* orig: HookStart (0022:01B3) */
static int hook_start(int n, Sprite *s, MoveState *pm, int ready)
{
    Intro *in = s->user;

    (void)ready;
    if (n < 0) {
        switch (n) {
        case -3:
            in->done = 1;
            break;
        case -2:
            *pm->mark = -1;
            break;
        case -1:
            set_start_move(s, k_idle, NULL, NULL);
            break;
        }
        return 1;
    }
    if (n > 0) {
        pm->n_steps = 2;
        if (*pm->mark >= n) {
            pm->n_step = 2;
            return 1;
        }
        pm->n_step = 1;
        return 0;
    }
    (*pm->mark)++;
    return 1;
}

static int load_bank(const char *resources, const char *name, Pic **dst, int n)
{
    char path[1024];
    size_t nbytes = 0;
    void *file;
    int i;

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/%s", resources, name);
    file = SDL_LoadFile(path, &nbytes);
    if (!file)
        return 0;
    for (i = 0; i < n; i++) {
        dst[i] = pic_decode_ppic(file, nbytes, i);
        if (!dst[i]) {
            SDL_free(file);
            return 0;
        }
    }
    SDL_free(file);
    return 1;
}

static SDL_Texture *texture_from(SDL_Renderer *renderer, Pic *pic)
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

/* orig: AddStartSprite (0022:0000) — cur image from scene_add; frames via set_start_frame */
static Sprite *add_start_sprite(Intro *in, Pic *img, Pic *mask, int x, int y, int mode)
{
    Sprite *s;

    s = scene_add(in->scene, img, mask, x, y, mode);
    if (s)
        s->user = in;
    return s;
}

/* orig: SetStartMove (0022:0067) */
static void set_start_move(Sprite *s, const uint8_t *script, MoveHook hook, int *mark)
{
    if (!s)
        return;
    sprite_set_script(s, script);
    if (!s->move)
        return;
    s->move->hook = hook;
    s->move->mark = mark;
}

/* orig: SetStartFrame (0022:00D0) */
static void set_start_frame(Sprite *s, Pic **images, Pic **masks, int frame, int frames)
{
    if (!s)
        return;
    s->images = images;
    s->masks = masks;
    s->frame = frame;
    s->frames = frames;
    sprite_sync(s);
}

/* orig: ClearStartMarks (0022:0178) */
static void clear_start_marks(Intro *in)
{
    int i;

    for (i = 0; i < 16; i++)
        in->marks[i] = -1;
}

/* orig: AllocStartMark (0022:0193) */
static int *alloc_start_mark(Intro *in)
{
    int *p;

    for (p = in->marks; *p >= 0; p++)
        ;
    *p = 0;
    return p;
}

/* orig: AddStartBack (0022:0232) */
static void add_start_back(Intro *in)
{
    add_start_sprite(in, in->bank[1], in->bank[2], 0x185, 0x3E, -1);
}

/* orig: AddStartFigure (0022:024D) */
static void add_start_figure(Intro *in)
{
    Sprite *s;

    s = add_start_sprite(in, in->bank[5], NULL, 0x1AD, 0x1C, 0);
    set_start_frame(s, &in->bank[5], NULL, 0, 7);
    if (s) {
        s->anim_rate = 2;
        s->anim_mode = 1;
    }
}

/* orig: AddStartMover (0022:0282) */
static void add_start_mover(Intro *in)
{
    Sprite *s;

    s = add_start_sprite(in, in->bank[3], in->bank[4], 0x170, 0x46, -1);
    set_start_move(s, k_a, hook_start, in->mark);
    set_start_frame(s, &in->bank[3], &in->bank[4], 0, 1);
}

/* orig: AddStartTrio (0022:02C6) */
static void add_start_trio(Intro *in)
{
    Sprite *s;

    s = add_start_sprite(in, in->bank[12], in->bank[13], 0x0E, 0x10, -1);
    set_start_move(s, k_b, hook_start, in->mark);
    set_start_frame(s, &in->bank[12], &in->bank[13], 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[14], in->bank[15], 0x0E, 0x2A, -1);
    set_start_move(s, k_c, hook_start, in->mark);
    set_start_frame(s, &in->bank[14], &in->bank[15], 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[16], in->bank[17], 0x0E, 0x45, -1);
    set_start_move(s, k_d, hook_start, in->mark);
    set_start_frame(s, &in->bank[16], &in->bank[17], 0, 1);
    if (s)
        s->visible = 0;
}

/* orig: AddStartPair (0022:0389) */
static void add_start_pair(Intro *in)
{
    Sprite *s;

    s = add_start_sprite(in, in->bank[30], NULL, 0xCD, 0x132, 0);
    set_start_move(s, k_f, hook_start, in->mark);
    set_start_frame(s, &in->bank[30], NULL, 0, 2);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[18], in->bank[24], 0xBE, 0x156, -1);
    set_start_move(s, k_e, hook_start, in->mark);
    set_start_frame(s, &in->bank[18], &in->bank[24], 0, 6);
}

/* orig: AddStartWing (0022:040C) */
static void add_start_wing(Intro *in)
{
    Sprite *s;

    s = add_start_sprite(in, in->bank[32], in->bank[33], 0x0E, 0x10, -1);
    set_start_move(s, k_g, hook_start, in->mark);
    set_start_frame(s, &in->bank[32], &in->bank[33], 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[34], in->bank[35], 0x0E, 0x2A, -1);
    set_start_move(s, k_h, hook_start, in->mark);
    set_start_frame(s, &in->bank[34], &in->bank[35], 0, 1);
    if (s)
        s->visible = 0;
}

/* orig: AddStartFloater (0022:0491) */
static void add_start_floater(Intro *in)
{
    Sprite *s;

    s = add_start_sprite(in, in->bank[46], in->bank[47], -0x52, 0x6D, -1);
    set_start_move(s, k_i, hook_start, in->mark);
    set_start_frame(s, &in->bank[46], &in->bank[47], 0, 1);
}

/* orig: AddStartRow (0022:04D4) */
static void add_start_row(Intro *in)
{
    Sprite *s;

    s = add_start_sprite(in, in->bank[36], in->bank[37], 0x3C, 0x13, -1);
    set_start_move(s, k_j, hook_start, in->mark);
    set_start_frame(s, &in->bank[36], &in->bank[37], 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[38], in->bank[39], 0x11, 0x55, -1);
    set_start_move(s, k_j, hook_start, in->mark);
    set_start_frame(s, &in->bank[38], &in->bank[39], 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[40], NULL, 0x3A, 0xB2, 0);
    set_start_move(s, k_k, hook_start, in->mark);
    set_start_frame(s, &in->bank[40], NULL, 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[41], NULL, 0x3A, 0xBF, 0);
    set_start_move(s, k_k, hook_start, in->mark);
    set_start_frame(s, &in->bank[41], NULL, 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[42], NULL, 0x3A, 0xCF, 0);
    set_start_move(s, k_k, hook_start, in->mark);
    set_start_frame(s, &in->bank[42], NULL, 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[43], NULL, 0x3A, 0xEE, 0);
    set_start_move(s, k_l, hook_start, in->mark);
    set_start_frame(s, &in->bank[43], NULL, 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[44], NULL, 0x3A, 0xFC, 0);
    set_start_move(s, k_l, hook_start, in->mark);
    set_start_frame(s, &in->bank[44], NULL, 0, 1);
    if (s)
        s->visible = 0;
    s = add_start_sprite(in, in->bank[45], NULL, 0x2E, 0x156, 0);
    set_start_move(s, k_m, hook_start, in->mark);
    set_start_frame(s, &in->bank[45], NULL, 0, 1);
    if (s)
        s->visible = 0;
}

/* orig: LayStartSprites (0022:06C7) */
static void lay_start_sprites(Intro *in)
{
    Sprite *s;

    clear_start_marks(in);
    in->mark = alloc_start_mark(in);
    s = add_start_sprite(in, NULL, NULL, 0, 0, 0);
    if (s)
        s->visible = 0;
    set_start_move(s, k_n, hook_start, in->mark);
    add_start_back(in);
    add_start_figure(in);
    add_start_mover(in);
    add_start_trio(in);
    add_start_pair(in);
    add_start_wing(in);
    add_start_floater(in);
    add_start_row(in);
}

static int in_field(const Intro *in, float x, float y)
{
    return x >= (float)in->x && y >= (float)in->y &&
           x < (float)(in->x + FIELD_W) && y < (float)(in->y + FIELD_H);
}

/* orig: OpenStart (0022:0828) / StartDlgProc WM_INITDIALOG (0022:093A) */
Intro *intro_new(SDL_Renderer *renderer, const char *resources)
{
    Intro *in;
    Pic *full;

    in = SDL_calloc(1, sizeof(*in));
    if (!in)
        return NULL;
    select_place(FIELD_W, FIELD_H, &in->x, &in->y);
    in->scene = scene_new(3);
    in->canvas = pic_canvas(FIELD_W, FIELD_H, 0, 0, 0);
    /* Colour bank only; the monochrome 0xFA0 path is not ported. */
    if (!in->scene || !in->canvas ||
        !load_bank(resources, "4001.pic", in->bank, 38) ||
        !load_bank(resources, "4002.pic", in->bank + 38, 10)) {
        intro_free(in);
        return NULL;
    }
    full = pic_canvas(VIEW_W, VIEW_H, 0, 0, 0);
    if (full) {
        select_paint_frame(full, resources);
        in->frame = texture_from(renderer, full);
        pic_free(full);
    }
    in->field = texture_from(renderer, in->canvas);
    if (!in->field) {
        intro_free(in);
        return NULL;
    }
    lay_start_sprites(in);
    return in;
}

/* orig: CloseStart (0022:08F7) */
void intro_free(Intro *in)
{
    int i;

    if (!in)
        return;
    scene_free(in->scene);
    for (i = 0; i < BANK; i++)
        pic_free(in->bank[i]);
    pic_free(in->canvas);
    if (in->frame)
        SDL_DestroyTexture(in->frame);
    if (in->field)
        SDL_DestroyTexture(in->field);
    SDL_free(in);
}

/* orig: StartWndProc WM_TIMER (0022:0782) */
int intro_tick(Intro *in)
{
    uint32_t now;
    int steps;

    if (!in || in->done)
        return 1;
    /* SubclassAnimItem rate 3 -> SetTimer every 51ms; TickScene catch-up max 3. */
    now = (uint32_t)SDL_GetTicks();
    if (!in->started) {
        in->started = 1;
        in->last_ms = now;
        return 0;
    }
    if (now - in->last_ms < 51)
        return 0;
    steps = (int)((now - in->last_ms) / 51);
    if (steps > 3)
        steps = 3;
    in->last_ms = now;
    scene_run(in->scene, steps);
    return in->done;
}

/* orig: BackStart (0022:0732) / StartWndProc WM_PAINT (0022:0782) */
void intro_draw(SDL_Renderer *renderer, Intro *in)
{
    SDL_FRect dst;

    if (in->frame) {
        dst.x = 0;
        dst.y = 0;
        dst.w = VIEW_W;
        dst.h = VIEW_H;
        SDL_RenderTexture(renderer, in->frame, NULL, &dst);
    }
    if (in->bank[0])
        pic_blit(in->canvas, 0, 0, in->bank[0], in->bank[0]->w, in->bank[0]->h);
    scene_paint(in->scene, in->canvas, FIELD_W, FIELD_H);
    SDL_UpdateTexture(in->field, NULL, in->canvas->rgba, FIELD_W * 4);
    dst.x = (float)in->x;
    dst.y = (float)in->y;
    dst.w = FIELD_W;
    dst.h = FIELD_H;
    SDL_RenderTexture(renderer, in->field, NULL, &dst);
}

/* orig: StartDlgProc (0022:093A) — dialog accelerators / IDOK / IDCANCEL */
int intro_key(Intro *in, SDL_Keycode key, SDL_Keymod mod)
{
    int chord;

    (void)in;
    chord = (mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT)) != 0;
    if (chord && key == SDLK_Q)
        return 101;
    if (key == SDLK_ESCAPE)
        return 2;
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
        return 1;
    return 0;
}

/* orig: TrackUserButton via UserWndProc on the start field */
void intro_press(Intro *in, float x, float y)
{
    if (!in)
        return;
    in->tracking = in_field(in, x, y);
}

int intro_release(Intro *in, float x, float y)
{
    int ok;

    if (!in || !in->tracking)
        return 0;
    ok = in_field(in, x, y);
    in->tracking = 0;
    return ok ? 1 : 0;
}
