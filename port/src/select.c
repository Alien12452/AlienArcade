#include "select.h"

#include "pic.h"
#include "scene.h"
#include "text.h"

#include <time.h>

#include <SDL3/SDL.h>

/* Matching: src/layout.c, src/play.c (dialog 310), src/view.c (OpenViews).
   LayoutMain / PaintMain (000E:0000 / 000E:0214)  select_paint_frame / place
   PlayDlgProc (000D:0105)                         select_load / key / press / draw
   PlayBorderProc (000D:0000)                      select_tick deadline / paint_card
   OpenViews / TickViews / CloseViews
     (0008:1451 / 0008:14BA / 0008:1501)            open_views / select_tick / free
   LineWndProc (0008:11DA)                         strip paint
   OpenView1..6                                    per-card live scenes */

#define VIEW_W 640
#define VIEW_H 480

/* PlayDlgProc sizes the dialog to this, then CenterPopup centres it. */
#define DLG_W 0x202
#define DLG_H 0x158

/* LineWndProc draws the rule to x=512, not the strip's 514. */
#define RULE_W 512

/* g_anPlayCtl control 0x815 (caption). */
#define CAP_X 92
#define CAP_Y 61
#define CAP_W 328
#define CAP_H 16

/* g_rgbCustom[8], the peach brush g_hGdiObjects[1]. */
#define PEACH_R 255
#define PEACH_G 204
#define PEACH_B 153

/* One card on dialog 310. OpenViews passes only the icon control, so the
   in-game help scenes (the extra id arguments) stay closed. */
typedef struct Game {
    const char *name;
    int bx, by, bw, bh;
    int lx, ly, lw, lh;
    int ix, iy, iw, ih;
} Game;

/* An icon scene or a scrolling bar. The picture is rebuilt every frame. */
typedef struct Live {
    Scene *scene;
    Pic *bank[20];
    int nbank;
    Pic *canvas;
    SDL_Texture *tex;
    int x, y, w, h;
    int rule; /* 0 none, 1 along the bottom, 2 along the top */
} Live;

struct Select {
    SDL_Texture *picture;
    int dlg_x;
    int dlg_y;
    Live live[8];
    int nlives;
    int ticking;
    int tracking;
    uint32_t step_ms;
    uint32_t seen_ms;
    time_t until;
};

static const Game k_games[6] = {
    {"Stink Snakes", 44, 98, 125, 58, 44, 162, 125, 16, 71, 108, 72, 31},
    {"Nerdoids", 191, 98, 125, 58, 191, 162, 125, 16, 217, 116, 74, 19},
    {"Quasar Klutzes", 338, 98, 125, 58, 338, 162, 125, 16, 387, 99, 27, 50},
    {"Fungus Patrol", 44, 195, 125, 58, 44, 259, 125, 16, 69, 202, 77, 38},
    {"Andromedroids", 191, 195, 125, 58, 191, 259, 125, 16, 233, 198, 39, 46},
    {"Quasar Rain", 338, 195, 125, 58, 338, 259, 125, 16, 381, 198, 37, 48},
};

/* Menu-only scripts from src/view.c. Help scenes are not opened here. */
static const uint8_t k_view1[] = {
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x09, 0x04,
    0x01, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x04, 0x08,
    0x01, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x04, 0x08, 0x02, 0x00, 0xFC, 0xFF, 0x00, 0x00, 0x04, 0x08,
    0x08, 0x00, 0x10, 0x00, 0x00, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04,
    0x02, 0x00, 0x07, 0x02, 0x0A, 0x00
};
static const uint8_t k_view2[] = {
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00,
    0x06, 0x02, 0x04, 0x08, 0x02, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x04, 0x08, 0x04, 0x00, 0xF6, 0xFF,
    0x00, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02,
    0x0A, 0x00
};
static const uint8_t k_view3[] = {
    0x00, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04,
    0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x00, 0x00, 0x0A, 0x00
};
static const uint8_t k_view5a[] = {
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00,
    0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
    0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04,
    0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x09, 0x04, 0x02, 0x00,
    0x09, 0x04, 0x02, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x09, 0x04,
    0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02,
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02,
    0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
    0x0A, 0x00
};
static const uint8_t k_view5b[] = {
    0x09, 0x04, 0x04, 0x00, 0x04, 0x08, 0x00, 0x00, 0xFC, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x09, 0x04,
    0x04, 0x00, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x06, 0x02,
    0x09, 0x04, 0x04, 0x00, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02,
    0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x04, 0x08, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x09, 0x04, 0x04, 0x00, 0x0A, 0x00
};
static const uint8_t k_view6[] = {
    0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0xF0, 0xFF,
    0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x0A, 0x00
};
static const uint8_t k_line_a[] = {
    0x03, 0x08, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x02, 0x02, 0x04, 0x08, 0x80, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x0A, 0x00
};
static const uint8_t k_line_b[] = {
    0x03, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x02, 0x04, 0x08, 0x80, 0x00, 0x00, 0xFF,
    0x00, 0x00, 0x0A, 0x00
};

static const char *k_caption = "Click on the Alien Arcade game you want to play!";

static Pic *load_index(const char *resources, const char *name, int index)
{
    char path[1024];
    size_t nbytes = 0;
    void *file;
    Pic *pic;

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/%s", resources, name);
    file = SDL_LoadFile(path, &nbytes);
    if (!file) {
        SDL_Log("could not read %s: %s", path, SDL_GetError());
        return NULL;
    }
    pic = pic_decode_ppic(file, nbytes, index);
    SDL_free(file);
    if (!pic)
        SDL_Log("could not decode %s image %d", name, index);
    return pic;
}

static SDL_Texture *texture_from_pic(SDL_Renderer *renderer, Pic *pic)
{
    SDL_Texture *tex;

    tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                            pic->w, pic->h);
    if (!tex)
        return NULL;
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    if (!SDL_UpdateTexture(tex, NULL, pic->rgba, pic->w * 4)) {
        SDL_DestroyTexture(tex);
        return NULL;
    }
    return tex;
}

/* LayoutMain picks (rand() & 3) * 10 once. The same set is used for the
   title, the list, and every round. Set 3 is the moonscape: its frame
   rects use a different margin, and CenterWindow shifts every window. */
static int border_set(void)
{
    static int set = -1;

    if (set < 0)
        set = (int)(time(NULL) & 3);
    return set;
}

/* orig: CenterWindow / LayoutMain hole offset (moonscape +47,+36) */
void select_place(int w, int h, int *x, int *y)
{
    *x = (VIEW_W - w - 4) / 2;
    *y = (VIEW_H - h - 2) / 2;
    if (border_set() == 3) {
        *x += 0x2f;
        *y += 0x24;
    }
}

/* orig: LayoutMain (000E:0000) / PaintMain (000E:0214) */
void select_paint_frame(Pic *canvas, const char *resources)
{
    int set = border_set();
    int play_left = (VIEW_W - 0x200) / 2;
    int play_top = (VIEW_H - 0x156) / 2;
    int left = play_left - 0x42;
    int top = play_top - 0x57;
    int right = play_left + 0x200 + 0x3e;
    int bottom = play_top + 0x156 + 0x33;
    int margin_t = set == 3 ? 0x79 : 0x55;
    int margin_r = set == 3 ? 0x10 : 0x3f;
    int margin_b = set == 3 ? 0x0f : 0x33;
    int ox[4], oy[4];
    int i;

    ox[0] = left;
    oy[0] = top;
    ox[1] = left;
    oy[1] = top + margin_t;
    ox[2] = right - margin_r;
    oy[2] = top + margin_t;
    ox[3] = left;
    oy[3] = bottom - margin_b;
    for (i = 0; i < 4; i++) {
        char name[16];
        Pic *pic;

        SDL_snprintf(name, sizeof(name), "%d.pic", 400 + set * 10 + i);
        pic = load_index(resources, name, 0);
        if (!pic)
            continue;
        pic_blit(canvas, ox[i], oy[i], pic, pic->w, pic->h);
        pic_free(pic);
    }
}

/* RoundRect's corner ellipse is 16 by 16, so the radius is 8. The pen is 2 wide. */
static int in_round(int x, int y, int w, int h, int rad)
{
    int cx;
    int cy;

    if (x < 0 || y < 0 || x >= w || y >= h)
        return 0;
    if (x >= rad && x < w - rad)
        return 1;
    if (y >= rad && y < h - rad)
        return 1;
    cx = x < rad ? rad : w - 1 - rad;
    cy = y < rad ? rad : h - 1 - rad;
    return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= rad * rad;
}

/* orig: PlayBorderProc (000D:0000) WM_PAINT */
static void paint_card(Pic *canvas, int x, int y, int w, int h)
{
    int row;
    int col;

    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            uint8_t *p;
            int white;

            if (!in_round(col, row, w, h, 8))
                continue;
            white = in_round(col - 2, row - 2, w - 4, h - 4, 6);
            p = canvas->rgba + ((size_t)(y + row) * (size_t)canvas->w + (size_t)(x + col)) * 4;
            p[0] = white ? 255 : 0;
            p[1] = white ? 255 : 0;
            p[2] = white ? 255 : 0;
            p[3] = 255;
        }
    }
}

static void lay_dialog(Pic *canvas, int dx, int dy)
{
    int i;

    pic_fill(canvas, dx, dy, DLG_W, DLG_H, PEACH_R, PEACH_G, PEACH_B);
    for (i = 0; i < 6; i++) {
        const Game *g = &k_games[i];

        paint_card(canvas, dx + g->bx, dy + g->by, g->bw, g->bh);
    }
}

static void *load_file(const char *resources, const char *name, size_t *nbytes)
{
    char path[1024];

    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/%s", resources, name);
    return SDL_LoadFile(path, nbytes);
}

static int load_bank(const char *resources, const char *name, Pic **dst, int n)
{
    size_t nbytes = 0;
    void *file;
    int i;

    file = load_file(resources, name, &nbytes);
    if (!file)
        return 0;
    for (i = 0; i < n; i++)
        dst[i] = pic_decode_ppic(file, nbytes, i);
    SDL_free(file);
    return dst[0] != NULL;
}

static void add_strip(Live *live, const char *resources, int index, const uint8_t *script)
{
    size_t nbytes = 0;
    void *file;
    Sprite *s;

    if (!live || !live->scene)
        return;
    file = load_file(resources, "5000.pic", &nbytes);
    if (!file)
        return;
    live->bank[0] = pic_decode_ppic(file, nbytes, index);
    live->nbank = live->bank[0] ? 1 : 0;
    SDL_free(file);
    if (!live->bank[0])
        return;
    s = scene_add(live->scene, live->bank[0], NULL, 0, 0, 0);
    if (!s)
        return;
    s->visible = 0;
    sprite_set_script(s, script);
}

static Sprite *add_masked(Scene *sc, Pic **imgs, Pic **masks, int x, int y, int frames,
                          int frame, int rate, const uint8_t *script)
{
    Pic *img = imgs ? imgs[frame] : NULL;
    Pic *mask = masks ? masks[frame] : NULL;
    Sprite *s;

    if (!sc || !img)
        return NULL;
    s = scene_add(sc, img, mask, x, y, -1);
    if (!s)
        return NULL;
    s->images = imgs;
    s->masks = masks;
    s->frames = frames;
    s->frame = frame;
    s->anim_rate = rate;
    sprite_sync(s);
    if (script)
        sprite_set_script(s, script);
    return s;
}

static Live *add_live(Select *screen, SDL_Renderer *renderer, int x, int y, int w, int h,
                      int rule, int rate)
{
    Live *live;

    if (screen->nlives >= 8)
        return NULL;
    live = &screen->live[screen->nlives++];
    live->x = x;
    live->y = y;
    live->w = w;
    live->h = h;
    live->rule = rule;
    live->scene = scene_new(rate);
    live->canvas = pic_canvas(w, h, 255, 255, 255);
    if (!live->scene || !live->canvas)
        return NULL;
    live->tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if (live->tex)
        SDL_SetTextureScaleMode(live->tex, SDL_SCALEMODE_NEAREST);
    return live;
}

/* orig: OpenViews (0008:1451) */
static void open_views(Select *screen, SDL_Renderer *renderer, const char *resources)
{
    Live *live;
    int i;

    /* Controls 22 and 23. InitScene rate 2; strip starts hidden and scrolls. */
    add_strip(add_live(screen, renderer, 0, 0, DLG_W, 35, 1, 2), resources, 0, k_line_a);
    add_strip(add_live(screen, renderer, 0, 310, DLG_W, 35, 2, 2), resources, 1, k_line_b);

    /* orig: OpenView1 (0008:019C) — menu branch, help ids 0; InitScene rate 3 */
    live = add_live(screen, renderer, k_games[0].ix, k_games[0].iy, k_games[0].iw, k_games[0].ih, 0, 3);
    if (live && load_bank(resources, "6100.pic", live->bank, 8))
        add_masked(live->scene, live->bank, live->bank + 4, 16, 0, 4, 0, 0, k_view1);

    /* orig: OpenView2 (0008:0449) */
    live = add_live(screen, renderer, k_games[1].ix, k_games[1].iy, k_games[1].iw, k_games[1].ih, 0, 3);
    if (live && load_bank(resources, "7100.pic", live->bank, 8))
        add_masked(live->scene, live->bank, live->bank + 4, 0, 0, 4, 0, 0, k_view2);

    /* orig: OpenView3 (0008:06F7) */
    live = add_live(screen, renderer, k_games[2].ix, k_games[2].iy, k_games[2].iw, k_games[2].ih, 0, 3);
    if (live && load_bank(resources, "8100.pic", live->bank, 14))
        add_masked(live->scene, live->bank, live->bank + 7, 0, 0, 7, 0, 0, k_view3);

    /* orig: OpenView4 (0008:09FA) — SetSpriteFrame(0) then nFrame = i+1 without refresh */
    live = add_live(screen, renderer, k_games[3].ix, k_games[3].iy, k_games[3].iw, k_games[3].ih, 0, 3);
    if (live && load_bank(resources, "17100.pic", live->bank, 6)) {
        for (i = 0; i < 3; i++) {
            Sprite *s;
            int frame = i + 1;

            if (frame > 2)
                frame -= 3;
            s = add_masked(live->scene, live->bank, live->bank + 3, i * 22, i == 1 ? 0 : 6,
                           3, 0, 3, NULL);
            if (s)
                s->frame = frame;
        }
    }

    /* orig: OpenView5 (0008:0D04) */
    live = add_live(screen, renderer, k_games[4].ix, k_games[4].iy, k_games[4].iw, k_games[4].ih, 0, 3);
    if (live && load_bank(resources, "16100.pic", live->bank, 20)) {
        add_masked(live->scene, live->bank, live->bank + 4, 4, 0, 4, 0, 0, k_view5a);
        add_masked(live->scene, live->bank + 8, live->bank + 13, 4, 17, 5, 0, 0, k_view5b);
        add_masked(live->scene, live->bank + 18, live->bank + 19, 4, 25, 1, 0, 0, NULL);
    }

    /* orig: OpenView6 (0008:0FED) */
    live = add_live(screen, renderer, k_games[5].ix, k_games[5].iy, k_games[5].iw, k_games[5].ih, 0, 3);
    if (live && load_bank(resources, "18100.pic", live->bank, 2))
        add_masked(live->scene, live->bank, live->bank + 1, 0, 1, 5, 0, 0, k_view6);
}

/* orig: PlayDlgProc (000D:0105) WM_INITDIALOG */
Select *select_load(SDL_Renderer *renderer, const char *resources)
{
    Select *screen;
    Pic *canvas;

    screen = SDL_calloc(1, sizeof(*screen));
    if (!screen)
        return NULL;
    select_place(DLG_W, DLG_H, &screen->dlg_x, &screen->dlg_y);
    canvas = pic_canvas(VIEW_W, VIEW_H, 0, 0, 0);
    if (!canvas) {
        SDL_free(screen);
        return NULL;
    }
    select_paint_frame(canvas, resources);
    lay_dialog(canvas, screen->dlg_x, screen->dlg_y);
    screen->picture = texture_from_pic(renderer, canvas);
    pic_free(canvas);
    if (!screen->picture) {
        SDL_free(screen);
        return NULL;
    }
    open_views(screen, renderer, resources);
    return screen;
}

/* orig: LineWndProc (0008:11DA) / BackLine — rule width 512 */
static void paint_live(Live *live)
{
    if (!live->canvas || !live->tex || !live->scene)
        return;
    if (live->rule)
        pic_fill(live->canvas, 0, 0, live->w, live->h, PEACH_R, PEACH_G, PEACH_B);
    else
        pic_fill(live->canvas, 0, 0, live->w, live->h, 255, 255, 255);
    scene_paint(live->scene, live->canvas, live->w, live->h);
    if (live->rule == 1)
        pic_fill(live->canvas, 0, live->h - 2, RULE_W, 2, 0, 0, 0);
    else if (live->rule == 2)
        pic_fill(live->canvas, 0, 0, RULE_W, 2, 0, 0, 0);
    SDL_UpdateTexture(live->tex, NULL, live->canvas->rgba, live->w * 4);
}

/* orig: TickViews (0008:14BA) / PlayBorderProc WM_TIMER (000D:0000) */
int select_tick(Select *screen)
{
    uint32_t now;
    int steps;
    int i;

    if (!screen)
        return 0;
    now = (uint32_t)SDL_GetTicks();
    /* A long gap means the list was hidden, so the 30 seconds starts over. */
    if (!screen->ticking || now - screen->seen_ms > 500) {
        screen->ticking = 1;
        screen->step_ms = now;
        screen->until = time(NULL) + 30;
    }
    screen->seen_ms = now;
    if (now - screen->step_ms >= 51) {
        steps = (int)((now - screen->step_ms) / 51);
        if (steps > 3)
            steps = 3;
        screen->step_ms += (uint32_t)steps * 51;
        for (i = 0; i < screen->nlives; i++) {
            if (screen->live[i].scene)
                scene_run(screen->live[i].scene, steps);
        }
    }
    return time(NULL) >= screen->until;
}

/* orig: ParamTextProc (000A:1325) — DT_CENTER, InflateRect(-1,-1) top-aligned */
static void draw_label(SDL_Renderer *renderer, const char *text, int x, int y, int w, int h)
{
    int tw = text_width(text, 9, 1);
    float tx = (float)x + (float)(w - tw) / 2.0f;
    float ty = (float)(y + 1);

    (void)h;
    text_draw(renderer, tx, ty, text, 9, 1, 0, 0, 0);
}

static int in_rect(float x, float y, int left, int top, int w, int h)
{
    return x >= (float)left && y >= (float)top &&
           x < (float)(left + w) && y < (float)(top + h);
}

/* PlayDlgProc hit regions: card body (not icon) → 1..6; strip → -1; else 0. */
static int hit_result(const Select *screen, float x, float y)
{
    int i;
    int lx = screen->dlg_x;
    int ly = screen->dlg_y;

    if (in_rect(x, y, lx, ly, DLG_W, 35) || in_rect(x, y, lx, ly + 310, DLG_W, 35))
        return -1;
    for (i = 0; i < 6; i++) {
        const Game *g = &k_games[i];

        if (in_rect(x, y, lx + g->ix, ly + g->iy, g->iw, g->ih))
            return 0;
        if (in_rect(x, y, lx + g->bx, ly + g->by, g->bw, g->bh))
            return i + 1;
    }
    return 0;
}

/* orig: PlayDlgProc (000D:0105) — IDOK / IDCANCEL / mnemonics / accel table 2 */
int select_key(Select *screen, SDL_Keycode key, SDL_Keymod mod)
{
    int ctrl = (mod & SDL_KMOD_CTRL) != 0;
    int alt = (mod & SDL_KMOD_ALT) != 0;

    (void)screen;
    if ((ctrl || alt) && key == SDLK_Q)
        return 101;
    if (key == SDLK_ESCAPE || key == SDLK_RETURN || key == SDLK_KP_ENTER)
        return -1;
    if (ctrl)
        return 0;
    if (key == SDLK_S)
        return 1;
    if (key == SDLK_N)
        return 2;
    if (key == SDLK_K)
        return 3;
    if (key == SDLK_F)
        return 4;
    if (key == SDLK_A)
        return 5;
    if (key == SDLK_R)
        return 6;
    return 0;
}

/* orig: TrackUserButton (000A:1125) via UserWndProc on card / strip */
void select_press(Select *screen, float x, float y)
{
    if (!screen)
        return;
    screen->tracking = hit_result(screen, x, y);
}

int select_release(Select *screen, float x, float y)
{
    int armed;
    int now;

    if (!screen || screen->tracking == 0)
        return 0;
    armed = screen->tracking;
    screen->tracking = 0;
    now = hit_result(screen, x, y);
    return now == armed ? now : 0;
}

void select_draw(SDL_Renderer *renderer, Select *screen)
{
    SDL_FRect dst;
    int i;

    dst.x = 0;
    dst.y = 0;
    dst.w = VIEW_W;
    dst.h = VIEW_H;
    SDL_RenderTexture(renderer, screen->picture, NULL, &dst);

    for (i = 0; i < screen->nlives; i++) {
        Live *live = &screen->live[i];

        paint_live(live);
        if (!live->tex)
            continue;
        dst.x = (float)screen->dlg_x + (float)live->x;
        dst.y = (float)screen->dlg_y + (float)live->y;
        dst.w = (float)live->w;
        dst.h = (float)live->h;
        SDL_RenderTexture(renderer, live->tex, NULL, &dst);
    }

    {
        /* Control 0x815: g_anPlayCtl 92, 61, 328, 16. */
        int tw = text_width(k_caption, 9, 1);
        float tx = (float)screen->dlg_x + (float)CAP_X + ((float)CAP_W - (float)tw) / 2.0f;
        float ty = (float)screen->dlg_y + (float)(CAP_Y + 1);

        text_draw(renderer, tx, ty, k_caption, 9, 1, 0, 0, 0);
    }
    for (i = 0; i < 6; i++) {
        const Game *g = &k_games[i];
        draw_label(renderer, g->name, screen->dlg_x + g->lx, screen->dlg_y + g->ly, g->lw, g->lh);
    }
}

/* orig: CloseViews (0008:1501) */
void select_free(Select *screen)
{
    int i;
    int n;

    if (!screen)
        return;
    for (i = 0; i < screen->nlives; i++) {
        Live *live = &screen->live[i];

        scene_free(live->scene);
        for (n = 0; n < 20; n++)
            pic_free(live->bank[n]);
        pic_free(live->canvas);
        if (live->tex)
            SDL_DestroyTexture(live->tex);
    }
    SDL_DestroyTexture(screen->picture);
    SDL_free(screen);
}
