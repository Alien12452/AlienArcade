#include "between.h"

#include "pic.h"
#include "scene.h"
#include "select.h"
#include "sound.h"
#include "text.h"

/* Matching source: src/between.c (segment 35). Cross-refs below are
   orig: Name (SSSS:OOOO) from that file's FUNCTION annotations. One
   original function may map to several port helpers.

   CountTimeBonus (0023:0000)     between_begin / between_tick / time_step /
                                  score_chunk / prime_steps / tally / pay /
                                  between_grant / wait_ticks
   RunRoundHelp (0023:042C)       between_instructions / open_help
   RoundHelpDlgProc (0023:044C)   draw_help / WM_ERASEBKGND colours (k_bg)
   RunStepDialog (0023:06E2)      gate loop + between_begin (bonus)
   StepDlgProc (0023:0743)        draw_gate / hit / cancel / OK / Instructions
   WaveDlgProc (0023:0784)       draw_bonus / between_begin setup
   OpenView1..6 / CloseView1..6   load_help / drop_help (help branch)
   View*WndProc WM_TIMER          tick_help (SubclassAnimItem rate 3)
   Delay (000A:1CCD)              delay_until / step_until
   PadNumString (001D:0000)       pad6
   RoundDoneDlgProc (0023:0902)   scores.c scores_draw_over

   Dialog units are pixels times two. CountTimeBonus waits in 17ms ticks:
   60 before the count, 2 between steps, 15 around the label, 60 before OK. */

enum {
    PH_GATE = 0,
    PH_HELP,
    PH_PLAY,
    PH_WAIT,
    PH_TIME,
    PH_GAP,
    PH_LABEL,
    PH_SCORE,
    PH_TAIL,
    PH_OK
};

/* orig: g_anTimeStep (0024:1CCA) — 10 min, 1 min, 10 sec, 1 sec */
static const long k_step[4] = { 600, 60, 10, 1 };

typedef struct Pane {
    Scene *scene;
    Pic *canvas;
    SDL_Texture *tex;
    int x, y, w, h;
} Pane;

struct Between {
    SDL_Renderer *renderer;
    char *root;
    Pane pane[3];
    int npane;
    Pic *bank[36];
    int nbank;
    uint32_t help_ms;
    int round;
    int phase;
    float mx, my;
    int wave;
    long score_shown;
    long bonus;
    long time_left;
    int mult;
    int step_i;
    int score_ready;
    int score_phase;
    long place;
    long place_step;
    int show_rows;
    int show_bonus;
    int show_sorry;
    int show_score;
    int show_ok;
    int clock_shown;
    int taken;
    int arm; /* 0 none, 1 OK, 2 Instructions */
    Uint64 until;
};

typedef struct Line {
    int round;
    int x, y, w;
    const char *text;
} Line;

static const uint8_t k_bg[6][3] = {
    { 102, 204, 51 },
    { 153, 255, 255 },
    { 255, 204, 153 },
    { 192, 192, 192 },
    { 255, 255, 102 },
    { 255, 204, 153 }
};

/* Help-only scripts. The menu copies of view 1, 2, 3A, 5 and 6A live in select.c. */
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
static const uint8_t k_view3a[] = {
    0x00, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04,
    0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
    0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x00, 0x00, 0x0A, 0x00
};
static const uint8_t k_view3b[] = {
    0x00, 0x02, 0x02, 0x02, 0x09, 0x04, 0x03, 0x00, 0x06, 0x02, 0x09, 0x04, 0x03, 0x00, 0x06, 0x02,
    0x09, 0x04, 0x03, 0x00, 0x01, 0x02, 0x09, 0x04, 0x07, 0x00, 0x07, 0x02, 0x07, 0x02, 0x0A, 0x00
};
static const uint8_t k_view3c[] = {
    0x00, 0x02, 0x06, 0x02, 0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0xF6, 0xFF, 0x04, 0x08, 0x03, 0x00,
    0x00, 0x00, 0xFD, 0xFF, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0x0D, 0x00,
    0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x0A, 0x00
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
static const uint8_t k_view6b[] = {
    0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0x18, 0x00, 0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0xE8, 0xFF,
    0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0xFA, 0xFF, 0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0x06, 0x00,
    0x0A, 0x00
};

static const Line k_line[] = {
    { 1, 62, 6, 130, "Instructions for Stink Snakes of Saturn" },
    { 1, 15, 23, 223, "Turn nasty Stink Snakes into happy faces by throwing pies at them!" },
    { 1, 66, 45, 158, "Earn 1 points for each snake you hit with a pie." },
    { 1, 66, 71, 151, "Earn 10 points for each happy face you catch." },
    { 1, 66, 91, 146, "Catch unused pies and throw them again, but you lose 5 pies if a snake gets past you." },
    { 1, 19, 121, 217, "Press Control-P to pause, Control-A to abort, or Control-Q to quit at any time during the game." },
    { 2, 85, 7, 85, "Instructions for Nerdoids" },
    { 2, 14, 25, 227, "Keep the Nerdoids out of the computer room by giving them comics!" },
    { 2, 58, 40, 173, "Earn at least 10 points for each Nerdoid that leaves reading a comic you gave them." },
    { 2, 58, 68, 170, "You lose 5 comics if hit by a falling calculator or pocket protector." },
    { 2, 58, 96, 175, "Avoid hitting Nerdoids with a comic more than once.  They might accidentally drop it." },
    { 2, 19, 123, 218, "Press Control-P to pause, Control-A to abort, or Control-Q to quit at any time during the game." },
    { 3, 76, 5, 105, "Instructions for Quasar Klutzes" },
    { 3, 23, 21, 211, "Rid your ship of those Quasar Klutzes by feeding them bananas!" },
    { 3, 83, 34, 139, "Earn 5 points for each banana you feed to a Klutz and earn 25 points if two Klutzes collide going for the same banana." },
    { 3, 83, 71, 134, "Beware of Robot Dog gas clouds!  You lose 2 bananas if a cloud hits your ship." },
    { 3, 83, 97, 149, "Earn 10 points for each Klutz that falls but lose 5 bananas if your ship is hit by one." },
    { 3, 20, 123, 218, "Press Control-P to pause, Control-A to abort, or Control-Q to quit at any time during the game." },
    { 4, 69, 7, 130, "Instructions for Fungus Patrol" },
    { 4, 15, 23, 223, "You are on patrol to keep the Mothership free of fungus build-up!" },
    { 4, 66, 45, 158, "Earn 25 points for each fungus egg you remove." },
    { 4, 66, 71, 151, "Earn 15 points for every fungus you clean off." },
    { 4, 66, 91, 146, "Your patrol is over when a fungus eats away your ship's bungee cord." },
    { 4, 19, 121, 217, "Press Control-P to pause, Control-A to abort, or Control-Q to quit at any time during the game." },
    { 5, 80, 6, 104, "Instructions for Andromedroids" },
    { 5, 14, 25, 227, "Your goal is to keep the Andromedroid factory operating smoothly!" },
    { 5, 58, 40, 173, "Earn 30 points for every completed robot.\nEarn 50 points if each part is the same color.  Put unused parts in the recycle bin." },
    { 5, 58, 71, 170, "Put Mercurian Metal Bugs in the bug bin.  You lose valuable parts if a bug gets in the recycle bin." },
    { 5, 58, 96, 175, "The game is over when you run out of heads, arms, or legs or if bugs have filled every assembly bin." },
    { 5, 19, 123, 218, "Press Control-P to pause, Control-A to abort, or Control-Q to quit at any time during the game." },
    { 6, 76, 5, 105, "Instructions for Quasar Rain" },
    { 6, 23, 21, 211, "Catch the falling Quasar Klutzes and bounce them into bed!" },
    { 6, 66, 37, 156, "Earn points for each Quasar Klutz you bounce into bed with the Mom-E-Tron 2000." },
    { 6, 66, 65, 155, "The game is over when you miss 3 Quasar Klutzes with the Mom-E-Tron.  (They will be caught by the Safe-T-Net)" },
    { 6, 66, 97, 156, "Lose points if you are hit by a banana peel." },
    { 6, 20, 123, 218, "Press Control-P to pause, Control-A to abort, or Control-Q to quit at any time during the game." }
};

/* orig: Delay (000A:1CCD) — wait while tick - start <= n → n+1 boundaries */
static Uint64 delay_until(Uint64 now, int n)
{
    return (now / 17 + (Uint64)n + 1) * 17;
}

/* CountTimeBonus per-step: dw = GetTickCount()/17 + 2 */
static Uint64 step_until(Uint64 now)
{
    return (now / 17 + 2) * 17;
}

static void wait_ticks(Between *ui, Uint64 now, int ticks)
{
    ui->until = delay_until(now, ticks);
}

static void wait_step(Between *ui, Uint64 now)
{
    ui->until = step_until(now);
}

/* orig: CloseView1..6 (0008:0367 / 0613 / 0916 / 0C20 / 0F19 / 119A) */
static void drop_help(Between *ui)
{
    int i;

    for (i = 0; i < ui->npane; i++) {
        scene_free(ui->pane[i].scene);
        pic_free(ui->pane[i].canvas);
        SDL_DestroyTexture(ui->pane[i].tex);
        ui->pane[i].scene = NULL;
        ui->pane[i].canvas = NULL;
        ui->pane[i].tex = NULL;
    }
    ui->npane = 0;
    for (i = 0; i < ui->nbank; i++) {
        pic_free(ui->bank[i]);
        ui->bank[i] = NULL;
    }
    ui->nbank = 0;
    ui->help_ms = 0;
}

static int load_bank(Between *ui, const char *file, int count)
{
    char path[1024];
    void *data;
    size_t nbytes = 0;
    int i;

    if (!ui->root || count < 1 || count > 36)
        return 0;
    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/%s", ui->root, file);
    data = SDL_LoadFile(path, &nbytes);
    if (!data)
        return 0;
    for (i = 0; i < count; i++)
        ui->bank[i] = pic_decode_ppic((const uint8_t *)data, nbytes, i);
    ui->nbank = count;
    SDL_free(data);
    return ui->bank[0] != NULL;
}

static Pane *add_pane(Between *ui, int x, int y, int w, int h)
{
    Pane *pane;
    const uint8_t *bg;

    if (ui->npane >= 3 || w < 1 || h < 1)
        return NULL;
    pane = &ui->pane[ui->npane++];
    pane->x = x * 2;
    pane->y = y * 2;
    pane->w = w * 2;
    pane->h = h * 2;
    pane->scene = scene_new(3);
    bg = k_bg[ui->round - 1];
    pane->canvas = pic_canvas(pane->w, pane->h, bg[0], bg[1], bg[2]);
    if (ui->renderer)
        pane->tex = SDL_CreateTexture(ui->renderer, SDL_PIXELFORMAT_RGBA32,
                                      SDL_TEXTUREACCESS_STATIC, pane->w, pane->h);
    if (pane->tex)
        SDL_SetTextureScaleMode(pane->tex, SDL_SCALEMODE_NEAREST);
    return pane;
}

static void add_sprite(Scene *sc, Pic **imgs, Pic **masks, int x, int y, int frames,
                       int frame, int rate, int mode, int step, const uint8_t *script)
{
    Sprite *s;

    if (!sc || !imgs || frame < 0 || !imgs[frame])
        return;
    s = scene_add(sc, imgs[frame], masks ? masks[frame] : NULL, x, y, -1);
    if (!s)
        return;
    s->images = imgs;
    s->masks = masks;
    s->frames = frames;
    s->frame = frame;
    s->anim_rate = rate;
    s->anim_mode = mode;
    s->step = step;
    sprite_sync(s);
    if (script)
        sprite_set_script(s, script);
}

static Scene *open_pane(Between *ui, int x, int y, int w, int h)
{
    Pane *pane = add_pane(ui, x, y, w, h);

    return pane ? pane->scene : NULL;
}

static Scene *last_pane(const Between *ui)
{
    if (ui->npane < 1)
        return NULL;
    return ui->pane[ui->npane - 1].scene;
}

/* Help branch of OpenViewN (idB/idC set). Menu branch is select.c. */
static void load_help(Between *ui)
{
    Pic **b;

    if (!ui || ui->round < 1 || ui->round > 6)
        return;
    drop_help(ui);
    b = ui->bank;
    switch (ui->round) {
    case 1: /* orig: OpenView1 (0008:019C) */
        if (!load_bank(ui, "6100.pic", 18))
            return;
        add_sprite(open_pane(ui, 23, 40, 36, 19), b, b + 4, 16, 0, 4, 0, 0, 0, 1, k_view1);
        add_sprite(open_pane(ui, 36, 67, 18, 20), b + 8, b + 12, 0, 0, 4, 0, 3, 1, -1, NULL);
        add_sprite(open_pane(ui, 39, 98, 14, 10), b + 16, b + 17, 0, 0, 1, 0, 0, 0, 1, NULL);
        break;
    case 2: /* orig: OpenView2 (0008:0449) */
        if (!load_bank(ui, "7100.pic", 14))
            return;
        add_sprite(open_pane(ui, 19, 46, 34, 10), b, b + 4, 0, 0, 4, 0, 0, 0, 1, k_view2);
        add_sprite(open_pane(ui, 34, 73, 14, 10), b + 8, b + 11, 0, 1, 1, 0, 0, 0, 1, NULL);
        add_sprite(last_pane(ui), b + 9, b + 12, 14, 0, 1, 0, 0, 0, 1, NULL);
        add_sprite(open_pane(ui, 36, 100, 12, 14), b + 10, b + 13, 0, 0, 1, 0, 0, 0, 1, NULL);
        break;
    case 3: /* orig: OpenView3 (0008:06F7) */
        if (!load_bank(ui, "8100.pic", 28))
            return;
        add_sprite(open_pane(ui, 44, 37, 15, 24), b, b + 7, 0, 0, 7, 0, 0, 0, 1, k_view3a);
        add_sprite(open_pane(ui, 36, 73, 34, 18), b + 18, b + 20, 0, 0, 2, 0, 4, 0, 1, NULL);
        add_sprite(last_pane(ui), b + 22, b + 25, 48, 0, 3, 0, 0, 0, 1, k_view3b);
        add_sprite(open_pane(ui, 39, 94, 22, 25), b + 14, b + 16, 0, 0, 2, 0, 0, 0, 1, k_view3c);
        break;
    case 4: /* orig: OpenView4 (0008:09FA) — OpenView4(dlg, 9, 8, 10) */
        if (!load_bank(ui, "17100.pic", 10))
            return;
        add_sprite(open_pane(ui, 39, 70, 17, 17), b, b + 3, 0, 0, 3, 0, 3, 0, 1, NULL);
        add_sprite(open_pane(ui, 41, 43, 17, 16), b + 6, b + 7, 0, 0, 1, 0, 0, 0, 1, NULL);
        add_sprite(open_pane(ui, 41, 94, 21, 18), b + 8, b + 9, 0, 0, 1, 0, 0, 0, 1, NULL);
        break;
    case 5: /* orig: OpenView5 (0008:0D04) */
        if (!load_bank(ui, "16100.pic", 34))
            return;
        add_sprite(open_pane(ui, 24, 42, 24, 25), b, b + 4, 4, 0, 4, 0, 0, 0, 1, k_view5a);
        add_sprite(last_pane(ui), b + 8, b + 13, 4, 17, 5, 0, 0, 0, 1, k_view5b);
        add_sprite(last_pane(ui), b + 18, b + 19, 4, 25, 1, 0, 0, 0, 1, NULL);
        add_sprite(open_pane(ui, 29, 74, 16, 15), b + 20, b + 26, 0, 0, 6, 0, 1, 0, 1, NULL);
        add_sprite(open_pane(ui, 20, 100, 34, 12), b + 32, b + 33, 0, 0, 1, 0, 0, 0, 1, NULL);
        break;
    case 6: /* orig: OpenView6 (0008:0FED) */
        if (!load_bank(ui, "18100.pic", 6))
            return;
        add_sprite(open_pane(ui, 31, 39, 29, 52), b, b + 1, 5, 3, 5, 0, 0, 0, 1, k_view6b);
        add_sprite(last_pane(ui), b + 2, b + 3, 0, 38, 1, 0, 0, 0, 1, NULL);
        add_sprite(open_pane(ui, 40, 99, 20, 15), b + 4, b + 5, 0, 0, 1, 0, 0, 0, 1, NULL);
        break;
    }
}

/* orig: RunRoundHelp (0023:042C) */
static void open_help(Between *ui)
{
    ui->phase = PH_HELP;
    load_help(ui);
}

static void close_help(Between *ui)
{
    drop_help(ui);
    ui->phase = PH_GATE;
}

/* orig: CountTimeBonus (0023:0000) — step index into g_anTimeStep */
static int prime_steps(long t)
{
    int n = 0;

    /* Steps past the first size that fits, then pulls a 10-minute start
       back onto the 1-second row. */
    while (n < 4 && k_step[n] > t)
        n++;
    n++;
    if (n == 4)
        n = 3;
    if (n > 4)
        n = 4;
    return n;
}

/* orig: CountTimeBonus (0023:0000) — total bonus for a remaining time */
static long tally(int round, long t)
{
    int mult = round <= 3 ? 5 : 10;
    int n;
    long bonus = 0;

    if (t <= 0)
        return 0;
    n = prime_steps(t);
    while (n < 4) {
        while (k_step[n] <= t) {
            bonus += k_step[n] * mult;
            t -= k_step[n];
        }
        n++;
    }
    return bonus;
}

/* orig: CountTimeBonus (0023:0000) — animate score by place value */
static void pay(long bonus, void *user, void (*add)(void *, long n))
{
    long n = 100000;

    if (bonus <= 0 || !add)
        return;
    while (n > bonus)
        n /= 10;
    n /= 10;
    if (n == 0)
        n = 1;
    while (n > 0) {
        long step = n;

        if (bonus / n > 4)
            n *= 2;
        while (bonus >= n) {
            bonus -= n;
            add(user, n);
        }
        if (n != step)
            n = (n >> 1) * 10;
        n /= 10;
    }
}

/* orig: CountTimeBonus (0023:0000) — one time-row decrement */
static int time_step(Between *ui)
{
    while (ui->step_i < 4) {
        long step = k_step[ui->step_i];

        if (step <= ui->time_left) {
            ui->bonus += step * ui->mult;
            ui->time_left -= step;
            return 1;
        }
        ui->step_i++;
    }
    return 0;
}

/* orig: CountTimeBonus (0023:0000) — one score-place chunk */
static int score_chunk(Between *ui, long *n_out)
{
    if (!ui->score_ready) {
        ui->place = 100000;
        while (ui->place > ui->bonus)
            ui->place /= 10;
        ui->place /= 10;
        if (ui->place == 0)
            ui->place = 1;
        ui->score_phase = 0;
        ui->score_ready = 1;
    }
    while (ui->place > 0) {
        if (ui->score_phase == 0) {
            ui->place_step = ui->place;
            if (ui->bonus / ui->place > 4)
                ui->place *= 2;
            ui->score_phase = 1;
        }
        if (ui->bonus >= ui->place) {
            ui->bonus -= ui->place;
            ui->score_shown += ui->place;
            *n_out = ui->place;
            ui->show_score = 1;
            return 1;
        }
        if (ui->place != ui->place_step)
            ui->place = (ui->place >> 1) * 10;
        ui->place /= 10;
        ui->score_phase = 0;
    }
    return 0;
}

static void origin(int dw, int dh, int *x, int *y)
{
    select_place(dw * 2, dh * 2, x, y);
}

static void box(int ox, int oy, int x, int y, int w, int h, SDL_FRect *r)
{
    r->x = (float)(ox + x * 2);
    r->y = (float)(oy + y * 2);
    r->w = (float)(w * 2);
    r->h = (float)(h * 2);
}

static int inside(const SDL_FRect *r, float x, float y)
{
    return x >= r->x && y >= r->y && x < r->x + r->w && y < r->y + r->h;
}

static void panel(SDL_Renderer *renderer, const SDL_FRect *r, uint8_t red, uint8_t green, uint8_t blue)
{
    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_RenderFillRect(renderer, r);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, r);
}

static void button(SDL_Renderer *renderer, const SDL_FRect *r, const char *label, int def)
{
    float tw;
    float tx;
    SDL_FRect edge;

    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderFillRect(renderer, r);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, r);
    if (def && r->w > 4.0f && r->h > 4.0f) {
        edge = *r;
        edge.x += 1.0f;
        edge.y += 1.0f;
        edge.w -= 2.0f;
        edge.h -= 2.0f;
        SDL_RenderRect(renderer, &edge);
    }
    tw = (float)text_width(label, 9, 1);
    tx = r->x + (r->w - tw) / 2.0f;
    if (tx < r->x + 2.0f)
        tx = r->x + 2.0f;
    text_draw(renderer, tx, r->y + (r->h - (float)text_line(9, 1)) / 2.0f,
              label, 9, 1, 0, 0, 0);
}

/* ParamTextProc InflateRect(-1,-1): draw inset by 1. */
static void draw_text(SDL_Renderer *renderer, float x, float y, float max_w, const char *text)
{
    text_block(renderer, x + 1.0f, y + 1.0f, max_w - 2.0f, text, 9, 1, 0, 0, 0);
}

static void text_right(SDL_Renderer *renderer, float x, float y, float w, const char *text)
{
    int tw = text_width(text, 9, 1);
    float left = x + 1.0f;
    float tx = x + w - 1.0f - (float)tw;

    if (tx < left)
        tx = left;
    text_draw(renderer, tx, y + 1.0f, text, 9, 1, 0, 0, 0);
}

static void text_center(SDL_Renderer *renderer, float x, float y, float w, const char *text)
{
    int tw = text_width(text, 9, 1);
    float left = x + 1.0f;
    float tx = x + (w - (float)tw) / 2.0f;

    if (tx < left)
        tx = left;
    text_draw(renderer, tx, y + 1.0f, text, 9, 1, 0, 0, 0);
}

/* orig: FormatTime (001D:0737) — same divisors as g_anTimeStep */
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
    dst[5] = 0;
}

static void pad6(long n, char *dst)
{
    int i;

    if (n < 0)
        n = 0;
    for (i = 5; i >= 0; i--) {
        dst[i] = (char)('0' + (int)(n % 10));
        n /= 10;
    }
    dst[6] = 0;
}

/* orig: StepDlgProc (0023:0743) — gate dialog 360 */
static void draw_gate(SDL_Renderer *renderer, Between *ui)
{
    int ox, oy;
    SDL_FRect frame, ok, help;

    origin(124, 86, &ox, &oy);
    box(ox, oy, 0, 0, 124, 86, &frame);
    panel(renderer, &frame, 192, 192, 192);
    text_draw(renderer, (float)(ox + 33 * 2 + 1), (float)(oy + 17 * 2 + 1),
              "Beginning Wave 1.", 9, 1, 0, 0, 0);
    box(ox, oy, 78, 43, 36, 17, &ok);
    box(ox, oy, 11, 43, 56, 17, &help);
    button(renderer, &ok, "OK", 1);
    button(renderer, &help, "Instructions...", 0);
    (void)ui;
}

/* orig: View*WndProc WM_TIMER — SubclassAnimItem rate 3 */
static void tick_help(Between *ui)
{
    uint32_t now = (uint32_t)SDL_GetTicks();
    int steps;
    int i;

    if (ui->npane < 1)
        return;
    if (ui->help_ms == 0) {
        ui->help_ms = now ? now : 1;
        return;
    }
    if (now - ui->help_ms < 51)
        return;
    steps = (int)((now - ui->help_ms) / 51);
    if (steps > 3)
        steps = 3;
    ui->help_ms += (uint32_t)steps * 51;
    for (i = 0; i < ui->npane; i++) {
        if (ui->pane[i].scene)
            scene_run(ui->pane[i].scene, steps);
    }
}

/* orig: RoundHelpDlgProc (0023:044C) */
static void draw_help(SDL_Renderer *renderer, Between *ui)
{
    int ox, oy;
    SDL_FRect frame, ok;
    const uint8_t *bg = k_bg[ui->round - 1];
    size_t i;
    int n;

    origin(256, 171, &ox, &oy);
    box(ox, oy, 0, 0, 256, 171, &frame);
    panel(renderer, &frame, bg[0], bg[1], bg[2]);
    tick_help(ui);
    for (n = 0; n < ui->npane; n++) {
        Pane *pane = &ui->pane[n];
        SDL_FRect at;

        if (!pane->canvas || !pane->tex || !pane->scene)
            continue;
        pic_fill(pane->canvas, 0, 0, pane->w, pane->h, bg[0], bg[1], bg[2]);
        scene_paint(pane->scene, pane->canvas, pane->w, pane->h);
        SDL_UpdateTexture(pane->tex, NULL, pane->canvas->rgba, pane->w * 4);
        at.x = (float)(ox + pane->x);
        at.y = (float)(oy + pane->y);
        at.w = (float)pane->w;
        at.h = (float)pane->h;
        SDL_RenderTexture(renderer, pane->tex, NULL, &at);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &frame);
    for (i = 0; i < sizeof(k_line) / sizeof(k_line[0]); i++) {
        if (k_line[i].round != ui->round)
            continue;
        draw_text(renderer, (float)(ox + k_line[i].x * 2), (float)(oy + k_line[i].y * 2),
                  (float)(k_line[i].w * 2), k_line[i].text);
    }
    box(ox, oy, 107, 149, 42, 17, &ok);
    button(renderer, &ok, "OK", 1);
}

/* orig: WaveDlgProc (0023:0784) — SwampGasDlg white; items 3/5 DT_RIGHT, 7 centre */
static void draw_bonus(SDL_Renderer *renderer, Between *ui)
{
    int ox, oy;
    SDL_FRect frame, ok;
    char title[32];
    char clock[8];
    char num[8];
    const char *row_label;

    origin(124, 96, &ox, &oy);
    box(ox, oy, 0, 0, 124, 96, &frame);
    panel(renderer, &frame, 255, 255, 255);
    SDL_snprintf(title, sizeof(title), "Wave %d Complete.", ui->wave);
    text_draw(renderer, (float)(ox + 20 * 2 + 1), (float)(oy + 11 * 2 + 1), title, 9, 1, 0, 0, 0);
    if (ui->show_sorry)
        text_center(renderer, (float)(ox + 24 * 2), (float)(oy + 34 * 2), (float)(77 * 2),
                    "Sorry, no time bonus!");
    if (ui->show_rows) {
        if (ui->clock_shown)
            format_time(ui->time_left, clock);
        else
            SDL_strlcpy(clock, "00:00", sizeof(clock));
        pad6(ui->show_score ? ui->score_shown : ui->bonus, num);
        row_label = ui->phase == PH_WAIT || ui->phase == PH_TIME || ui->phase == PH_GAP
                        ? "Time Left:"
                        : "Your Score:";
        text_right(renderer, (float)(ox + 26 * 2), (float)(oy + 28 * 2), (float)(40 * 2), row_label);
        text_draw(renderer, (float)(ox + 68 * 2 + 1), (float)(oy + 28 * 2 + 1),
                  ui->show_score ? num : clock, 9, 1, 0, 0, 0);
        if (ui->show_bonus) {
            pad6(ui->bonus, num);
            text_right(renderer, (float)(ox + 18 * 2), (float)(oy + 40 * 2), (float)(48 * 2),
                       "Bonus Points:");
            text_draw(renderer, (float)(ox + 68 * 2 + 1), (float)(oy + 40 * 2 + 1), num, 9, 1, 0, 0, 0);
        }
    }
    if (ui->show_ok) {
        box(ox, oy, 44, 68, 37, 16, &ok);
        button(renderer, &ok, "OK", 1);
    }
}

Between *between_new(SDL_Renderer *renderer, const char *resources, int round)
{
    Between *ui = SDL_calloc(1, sizeof(*ui));

    if (!ui)
        return NULL;
    ui->renderer = renderer;
    ui->round = round;
    ui->phase = PH_GATE;
    if (resources)
        ui->root = SDL_strdup(resources);
    return ui;
}

void between_free(Between *ui)
{
    if (!ui)
        return;
    drop_help(ui);
    SDL_free(ui->root);
    SDL_free(ui);
}

int between_playing(const Between *ui)
{
    return ui && ui->phase != PH_GATE && ui->phase != PH_HELP;
}

int between_busy(const Between *ui)
{
    return ui && ui->phase >= PH_WAIT;
}

int between_modal(const Between *ui)
{
    return ui && ui->phase != PH_PLAY;
}

void between_motion(Between *ui, float x, float y)
{
    if (!ui)
        return;
    ui->mx = x;
    ui->my = y;
}

static int hit_ok(Between *ui)
{
    int ox, oy;
    SDL_FRect ok;

    if (ui->phase == PH_GATE) {
        origin(124, 86, &ox, &oy);
        box(ox, oy, 78, 43, 36, 17, &ok);
        return inside(&ok, ui->mx, ui->my);
    }
    if (ui->phase == PH_HELP) {
        origin(256, 171, &ox, &oy);
        box(ox, oy, 107, 149, 42, 17, &ok);
        return inside(&ok, ui->mx, ui->my);
    }
    if (ui->phase == PH_OK && ui->show_ok) {
        origin(124, 96, &ox, &oy);
        box(ox, oy, 44, 68, 37, 16, &ok);
        return inside(&ok, ui->mx, ui->my);
    }
    return 0;
}

static int hit_help(Between *ui)
{
    int ox, oy;
    SDL_FRect help;

    if (ui->phase != PH_GATE)
        return 0;
    origin(124, 86, &ox, &oy);
    box(ox, oy, 11, 43, 56, 17, &help);
    return inside(&help, ui->mx, ui->my);
}

static void accept_ok(Between *ui)
{
    if (ui->phase == PH_GATE)
        ui->phase = PH_PLAY;
    else if (ui->phase == PH_HELP)
        close_help(ui);
    else if (ui->phase == PH_OK && ui->show_ok) {
        ui->phase = PH_PLAY;
        ui->taken = 1;
    }
}

/* orig: TrackUserButton via gate / help / bonus OK */
void between_press(Between *ui)
{
    if (!ui)
        return;
    if (hit_help(ui))
        ui->arm = 2;
    else if (hit_ok(ui))
        ui->arm = 1;
    else
        ui->arm = 0;
}

int between_release(Between *ui)
{
    int armed;

    if (!ui || ui->arm == 0)
        return 0;
    armed = ui->arm;
    ui->arm = 0;
    if (armed == 2 && hit_help(ui)) {
        open_help(ui);
        return 1;
    }
    if (armed == 1 && hit_ok(ui)) {
        accept_ok(ui);
        return 1;
    }
    return 0;
}

void between_click(Between *ui)
{
    between_press(ui);
    between_release(ui);
}

void between_ok(Between *ui)
{
    if (!ui)
        return;
    if (ui->phase == PH_GATE || ui->phase == PH_HELP || (ui->phase == PH_OK && ui->show_ok))
        accept_ok(ui);
}

/* orig: StepDlgProc / WaveDlgProc Esc → IDCANCEL (2); help ignores */
void between_cancel(Between *ui)
{
    if (!ui)
        return;
    if (ui->phase == PH_GATE || (ui->phase == PH_OK && ui->show_ok))
        accept_ok(ui);
}

/* orig: RunRoundHelp (0023:042C) */
void between_instructions(Between *ui)
{
    if (ui && ui->phase == PH_GATE)
        open_help(ui);
}

void between_skip_gate(Between *ui)
{
    if (!ui)
        return;
    if (ui->phase == PH_GATE || ui->phase == PH_HELP) {
        drop_help(ui);
        ui->phase = PH_PLAY;
    }
}

/* orig: RunStepDialog (0023:06E2) + CountTimeBonus (0023:0000) setup */
void between_begin(Between *ui, int wave, long score, long time_left)
{
    if (!ui || ui->phase >= PH_WAIT)
        return;
    ui->wave = wave;
    ui->score_shown = score;
    ui->bonus = 0;
    ui->time_left = time_left;
    ui->mult = ui->round <= 3 ? 5 : 10;
    ui->step_i = 0;
    ui->score_ready = 0;
    ui->score_phase = 0;
    ui->place = 0;
    ui->show_score = 0;
    ui->taken = 0;
    ui->show_sorry = 0;
    ui->show_rows = 0;
    ui->show_bonus = 0;
    ui->show_ok = 0;
    ui->clock_shown = 0;
    ui->arm = 0;
    sound_play(0x232B, 1);
    if (time_left == 0) {
        ui->phase = PH_OK;
        ui->show_ok = 1;
        ui->show_sorry = ui->round != 5;
        return;
    }
    ui->phase = PH_WAIT;
    ui->show_rows = 1;
    ui->show_bonus = 1;
    wait_ticks(ui, SDL_GetTicks(), 60);
}

/* orig: CountTimeBonus (0023:0000) — one Delay slice of the bonus count */
void between_tick(Between *ui, Uint64 now, long *added, int *timed, long *time_left)
{
    long chunk = 0;

    if (added)
        *added = 0;
    if (timed)
        *timed = 0;
    if (!ui || ui->phase < PH_WAIT || ui->phase == PH_OK)
        return;
    if (now < ui->until)
        return;
    if (ui->phase == PH_WAIT) {
        ui->step_i = prime_steps(ui->time_left);
        ui->phase = PH_TIME;
    }
    if (ui->phase == PH_TIME) {
        if (time_step(ui)) {
            ui->clock_shown = 1;
            if (timed)
                *timed = 1;
            if (time_left)
                *time_left = ui->time_left;
            sound_play(0x232F, 0);
            wait_step(ui, now);
            return;
        }
        ui->phase = PH_GAP;
        wait_ticks(ui, now, 15);
        return;
    }
    if (ui->phase == PH_GAP) {
        ui->phase = PH_LABEL;
        wait_ticks(ui, now, 15);
        return;
    }
    if (ui->phase == PH_LABEL)
        ui->phase = PH_SCORE;
    if (ui->phase == PH_SCORE) {
        if (score_chunk(ui, &chunk)) {
            if (added)
                *added = chunk;
            sound_play(0x2330, 0);
            wait_step(ui, now);
            return;
        }
        ui->phase = PH_TAIL;
        wait_ticks(ui, now, 60);
        return;
    }
    if (ui->phase == PH_TAIL) {
        ui->show_bonus = 0;
        ui->show_ok = 1;
        ui->phase = PH_OK;
    }
}

int between_take(Between *ui, int *supply)
{
    if (!ui || !ui->taken)
        return 0;
    ui->taken = 0;
    if (supply)
        *supply = (ui->round == 1 || ui->round == 2) ? 20 : 0;
    return 1;
}

/* orig: CountTimeBonus (0023:0000) — headless full bonus (no UI) */
void between_grant(int round, long time_left, void *user, void (*add)(void *, long n))
{
    pay(tally(round, time_left), user, add);
}

void between_draw(SDL_Renderer *renderer, Between *ui)
{
    if (!ui || !renderer)
        return;
    if (ui->phase == PH_GATE)
        draw_gate(renderer, ui);
    else if (ui->phase == PH_HELP)
        draw_help(renderer, ui);
    else if (ui->phase >= PH_WAIT)
        draw_bonus(renderer, ui);
}
