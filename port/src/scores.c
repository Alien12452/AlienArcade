#include "scores.h"

#include "pic.h"
#include "scene.h"
#include "select.h"
#include "sound.h"
#include "text.h"

#include <stdio.h>
#include <time.h>

/* Matching: score.c / slist.c / board.c / next.c / result.c / play.c / clear.c.
   LoadRoundScores / SaveRoundScores (0016:0025 / 0016:01D3)  load_file / save_file
   ReadScoreList / WriteScoreList (0016:0212 / 0016:03A0)    parse_value / write_list
   RecordScore (0016:00F2)                                   scores_present / commit
   ClearRanks (0016:0000)                                    scores_clear_ranks
   AllocScoreList / SetScoreDay / CheckScoreDay
     (0007:0000 / 0007:009E / 0007:0149)                     clear_list / roll_day
   InsertScore / ScoreQualifies / GetYDay
     (0007:021E / 0007:0197 / 0007:011C)                     insert / qualifies / today
   RunNameDialog / NameDlgProc / NamePaintProc
     (000D:062C / 000D:0645 / 000D:05BC)                     scores_draw_name / type
   RunRoundDone / RoundDoneDlgProc (0023:08E9 / 0023:0902)   scores_draw_over
   PaintNextScores / RunNextDialog / NextDlgProc
     (0013:01B4 / 0012:0372 / 0012:0390)                     scores_draw_board / show
   OpenNext / AddNextSprite (0012:007E / 0012:0000)          open_bars
   DrawRankRow / DrawRankList / TextOutRight / DrawPictureAt
     (0013:003C / 0013:0152 / 0013:0000 / 0013:039D)
   RunResetScores / ResetScoresDlgProc (0011:0000 / 0011:001C)
   RunEraseScores / EraseScoresDlgProc / EraseScoreFile
     (0011:00AF / 0011:00CB / 0011:0132) */

#define MAX_NAME 20
#define MAX_RANK 10

static const char *k_game[] = {
    "Stink Snakes", "Nerdoids", "Quasar Klutzes",
    "Fungus Patrol", "Andromedroids", "Quasar Rain"
};

typedef struct Entry {
    long score;
    char name[MAX_NAME + 1];
} Entry;

typedef struct List {
    int all_time;
    int day;
    int count;
    Entry entry[MAX_RANK];
} List;

struct Scores {
    SDL_Renderer *renderer;
    char path[1024];
    List daily[6];
    List alltime[6];
    char name[MAX_NAME + 1];
    int replace;
    int round;
    long score;
    int need_name;
    int daily_rank;
    int all_rank;
    SDL_Texture *portrait;
    SDL_Texture *mark_hi;
    SDL_Texture *mark_lo;
    Scene *bar[2];
    Pic *bar_pic[2];
    Pic *bar_canvas[2];
    SDL_Texture *bar_tex[2];
    uint32_t bar_ms;
};

/* orig: GetYDay (0007:011C) */
static int today(void)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    return tm ? tm->tm_yday : 0;
}

static void copy_name(char *dst, const char *src)
{
    int n = 0;

    if (!src)
        src = "";
    while (src[n] && n < MAX_NAME) {
        dst[n] = src[n];
        n++;
    }
    dst[n] = 0;
}

static SDL_Texture *texture_from_pic(SDL_Renderer *renderer, Pic *pic)
{
    SDL_Texture *tex;

    if (!pic)
        return NULL;
    tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
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

static SDL_Texture *load_pic(SDL_Renderer *renderer, const char *resources,
                             const char *kind, const char *name)
{
    char path[1024];
    size_t nbytes = 0;
    void *file;
    Pic *pic;
    SDL_Texture *tex;

    if (!resources)
        return NULL;
    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/%s/%s", resources, kind, name);
    file = SDL_LoadFile(path, &nbytes);
    if (!file)
        return NULL;
    pic = pic_decode_ppic((const uint8_t *)file, nbytes, 0);
    SDL_free(file);
    tex = texture_from_pic(renderer, pic);
    pic_free(pic);
    return tex;
}

/* orig: AllocScoreList (0007:0000) / SetScoreDay (0007:009E) */
static void clear_list(List *list, int all_time)
{
    list->all_time = all_time;
    list->count = 0;
    list->day = all_time ? 0 : today();
}

/* orig: CheckScoreDay (0007:0149) — day==0 never rolls */
static void roll_day(List *list)
{
    int yday;

    if (list->all_time || list->day == 0)
        return;
    yday = today();
    if (list->day != yday) {
        list->count = 0;
        list->day = yday;
    }
}

/* orig: ScoreQualifies (0007:0197) */
static int qualifies(List *list, long score)
{
    int i;

    if (score == 0)
        return 0;
    roll_day(list);
    for (i = 0; i < list->count; i++) {
        if (list->entry[i].score <= score)
            return i + 1;
    }
    if (list->count < MAX_RANK)
        return list->count + 1;
    return 0;
}

/* orig: InsertScore (0007:021E) */
static int insert_score(List *list, long score, const char *name)
{
    int rank;
    int i;

    rank = qualifies(list, score);
    if (!rank)
        return 0;
    if (list->count < MAX_RANK)
        list->count++;
    for (i = list->count - 1; i >= rank; i--)
        list->entry[i] = list->entry[i - 1];
    list->entry[rank - 1].score = score;
    copy_name(list->entry[rank - 1].name, name);
    return rank;
}

static int list_index(int id, int *all_time)
{
    int round;

    if (id >= 2001 && id <= 2006) {
        *all_time = 1;
        round = id - 2000;
    } else if (id >= 1001 && id <= 1006) {
        *all_time = 0;
        round = id - 1000;
    } else
        return -1;
    return round - 1;
}

static void add_entry(List *list, const char *name, long score, int first)
{
    if (!list->all_time && first && SDL_strcmp(name, "DATE") == 0) {
        if ((long)today() != score)
            list->count = -1;
        else {
            list->day = (int)score;
            if (list->count < 0)
                list->count = 0;
        }
        return;
    }
    if (list->count < 0 || list->count >= MAX_RANK || score == 0)
        return;
    copy_name(list->entry[list->count].name, name);
    list->entry[list->count].score = score;
    list->count++;
}

static void parse_value(List *list, char *value)
{
    char *p = value;
    int first = 1;

    list->count = 0;
    while (p && *p) {
        char *semi = SDL_strchr(p, ';');
        char *eq;
        long score;

        if (semi)
            *semi = 0;
        eq = SDL_strchr(p, '=');
        if (eq) {
            *eq = 0;
            score = SDL_strtol(eq + 1, NULL, 10);
            add_entry(list, p, score, first);
            first = 0;
        }
        if (!semi)
            break;
        p = semi + 1;
    }
    if (list->count < 0)
        list->count = 0;
}

/* orig: LoadRoundScores (0016:0025) / ReadScoreList (0016:0212) */
static void load_file(Scores *scores)
{
    char *text;
    size_t nbytes = 0;
    char *line;
    int i;

    for (i = 0; i < 6; i++) {
        clear_list(&scores->daily[i], 0);
        clear_list(&scores->alltime[i], 1);
    }
    if (!scores->path[0])
        return;
    text = (char *)SDL_LoadFile(scores->path, &nbytes);
    if (!text)
        return;
    line = text;
    while (line < text + nbytes) {
        char *end = line;
        int all_time = 0;
        int id;
        int index;
        char *eq;

        while (end < text + nbytes && *end != '\n' && *end != '\r')
            end++;
        *end = 0;
        if (SDL_strncmp(line, "List", 4) == 0) {
            id = (int)SDL_strtol(line + 4, &eq, 10);
            index = list_index(id, &all_time);
            if (index >= 0 && eq && *eq == '=') {
                List *list = all_time ? &scores->alltime[index] : &scores->daily[index];
                parse_value(list, eq + 1);
            }
        }
        line = end + 1;
    }
    SDL_free(text);
}

static void write_list(FILE *fp, const List *list, int id)
{
    int i;

    fprintf(fp, "List%d=", id);
    if (!list->all_time)
        fprintf(fp, "DATE=%d;", list->day);
    for (i = 0; i < list->count; i++)
        fprintf(fp, "%s=%ld;", list->entry[i].name, list->entry[i].score);
    fputc('\n', fp);
}

/* orig: SaveRoundScores (0016:01D3) / WriteScoreList (0016:03A0) */
static void save_file(Scores *scores)
{
    FILE *fp;
    int i;

    if (!scores->path[0])
        return;
    fp = fopen(scores->path, "w");
    if (!fp) {
        SDL_Log("could not write %s", scores->path);
        return;
    }
    fputs("[Daily High Scores]\n", fp);
    for (i = 0; i < 6; i++)
        write_list(fp, &scores->daily[i], 1001 + i);
    fputs("[All Time High Scores]\n", fp);
    for (i = 0; i < 6; i++)
        write_list(fp, &scores->alltime[i], 2001 + i);
    fclose(fp);
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

static void panel(SDL_Renderer *renderer, const SDL_FRect *r)
{
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderFillRect(renderer, r);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, r);
}

static void button(SDL_Renderer *renderer, const SDL_FRect *r, const char *label, int def)
{
    float tw = (float)text_width(label, 9, 1);
    SDL_FRect edge;

    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderFillRect(renderer, r);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, r);
    if (def) {
        edge = *r;
        edge.x += 1.0f;
        edge.y += 1.0f;
        edge.w -= 2.0f;
        edge.h -= 2.0f;
        SDL_RenderRect(renderer, &edge);
    }
    text_draw(renderer, r->x + (r->w - tw) / 2.0f,
              r->y + (r->h - (float)text_line(9, 1)) / 2.0f,
              label, 9, 1, 0, 0, 0);
}

static void draw_wrapped(SDL_Renderer *renderer, float x, float y, float max_w, const char *text)
{
    text_block(renderer, x, y, max_w, text, 9, 1, 0, 0, 0);
}

static List *daily_of(Scores *scores)
{
    if (scores->round < 1 || scores->round > 6)
        return NULL;
    return &scores->daily[scores->round - 1];
}

static List *all_of(Scores *scores)
{
    if (scores->round < 1 || scores->round > 6)
        return NULL;
    return &scores->alltime[scores->round - 1];
}

static void open_bars(Scores *scores, const char *resources);

Scores *scores_open(SDL_Renderer *renderer, const char *resources)
{
    Scores *scores = SDL_calloc(1, sizeof(*scores));
    char *dir;

    if (!scores)
        return NULL;
    scores->renderer = renderer;
    dir = SDL_GetPrefPath("alienarcade", "AlienArcade");
    if (dir) {
        SDL_snprintf(scores->path, sizeof(scores->path), "%sAARCADE.HI", dir);
        SDL_free(dir);
    }
    load_file(scores);
    scores->portrait = load_pic(renderer, resources, "PPIC", "250.pic");
    scores->mark_hi = load_pic(renderer, resources, "PPCT", "270.pct");
    scores->mark_lo = load_pic(renderer, resources, "PPCT", "271.pct");
    open_bars(scores, resources);
    return scores;
}

void scores_close(Scores *scores)
{
    int i;

    if (!scores)
        return;
    SDL_DestroyTexture(scores->portrait);
    SDL_DestroyTexture(scores->mark_hi);
    SDL_DestroyTexture(scores->mark_lo);
    for (i = 0; i < 2; i++) {
        scene_free(scores->bar[i]);
        pic_free(scores->bar_pic[i]);
        pic_free(scores->bar_canvas[i]);
        SDL_DestroyTexture(scores->bar_tex[i]);
    }
    SDL_free(scores);
}

/* orig: RecordScore (0016:00F2) */
void scores_present(Scores *scores, int round, long score)
{
    List *daily;
    List *alltime;

    if (!scores)
        return;
    scores->round = round;
    scores->score = score;
    scores->daily_rank = 0;
    scores->all_rank = 0;
    scores->replace = scores->name[0] != 0;
    daily = daily_of(scores);
    alltime = all_of(scores);
    scores->need_name = (daily && qualifies(daily, score)) ||
                        (alltime && qualifies(alltime, score));
    /* Board opens later via ending=3 without scores_show; clear strip debt. */
    scores->bar_ms = 0;
    sound_play(0x232C, 1);
}

/* orig: NextDlgProc WM_INITDIALOG (0012:0390) — keep ranks; fanfare if daily */
void scores_show(Scores *scores, int round)
{
    if (!scores || round < 1 || round > 6)
        return;
    scores->round = round;
    scores->need_name = 0;
    /* OpenNext rebuilds strips; restamp so a long gap does not dump catch-up. */
    scores->bar_ms = 0;
    if (scores->daily_rank != 0)
        sound_play(0x2331, 1);
}

/* orig: ClearRanks (0016:0000) */
void scores_clear_ranks(Scores *scores)
{
    if (!scores)
        return;
    scores->daily_rank = 0;
    scores->all_rank = 0;
}

int scores_needs_name(const Scores *scores)
{
    return scores && scores->need_name;
}

void scores_type(Scores *scores, const char *text)
{
    int n;

    if (!scores || !text)
        return;
    if (scores->replace) {
        scores->name[0] = 0;
        scores->replace = 0;
    }
    n = (int)SDL_strlen(scores->name);
    while (*text && n < MAX_NAME) {
        unsigned char c = (unsigned char)*text++;
        if (c < 32 || c >= 127 || c == '=' || c == ';')
            continue;
        scores->name[n++] = (char)c;
    }
    scores->name[n] = 0;
}

void scores_backspace(Scores *scores)
{
    int n;

    if (!scores)
        return;
    if (scores->replace) {
        scores->name[0] = 0;
        scores->replace = 0;
        return;
    }
    n = (int)SDL_strlen(scores->name);
    if (n > 0)
        scores->name[n - 1] = 0;
}

int scores_name_blank(const Scores *scores)
{
    return !scores || scores->name[0] == 0;
}

/* orig: RecordScore name path + SaveRoundScores */
void scores_commit(Scores *scores)
{
    List *daily;
    List *alltime;

    if (!scores)
        return;
    daily = daily_of(scores);
    alltime = all_of(scores);
    scores->daily_rank = 0;
    scores->all_rank = 0;
    if (scores->name[0] == 0)
        return;
    if (daily)
        scores->daily_rank = insert_score(daily, scores->score, scores->name);
    if (alltime)
        scores->all_rank = insert_score(alltime, scores->score, scores->name);
    if (scores->daily_rank || scores->all_rank)
        save_file(scores);
}

int scores_daily_place(const Scores *scores)
{
    return scores && scores->daily_rank != 0;
}

static void over_ok(const Scores *scores, SDL_FRect *ok)
{
    int ox, oy;

    (void)scores;
    origin(125, 82, &ox, &oy);
    box(ox, oy, 44, 54, 37, 17, ok);
}

int scores_hit_over(const Scores *scores, float x, float y)
{
    SDL_FRect ok;

    over_ok(scores, &ok);
    return inside(&ok, x, y);
}

static void name_ok(SDL_FRect *ok)
{
    int ox, oy;

    origin(199, 114, &ox, &oy);
    box(ox, oy, 127, 84, 39, 16, ok);
}

int scores_hit_name_ok(const Scores *scores, float x, float y)
{
    SDL_FRect ok;

    (void)scores;
    name_ok(&ok);
    return inside(&ok, x, y);
}

/* orig: RunRoundDone (0023:08E9) / RoundDoneDlgProc (0023:0902) — SwampGasDlg white */
void scores_draw_over(SDL_Renderer *renderer, const Scores *scores)
{
    int ox, oy;
    SDL_FRect frame, ok;
    char digits[8];
    char line[64];
    long n;
    int i;

    if (!scores)
        return;
    origin(125, 82, &ox, &oy);
    box(ox, oy, 0, 0, 125, 82, &frame);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &frame);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &frame);
    text_draw(renderer,
              (float)ox + (125.0f * 2.0f - (float)text_width("Game Over", 9, 1)) / 2.0f,
              (float)(oy + 10 * 2 + 1), "Game Over", 9, 1, 0, 0, 0);
    /* PadNumString(g_lPlayScore, 6) into ParamText "^0"; control (32,30) left-aligned */
    n = scores->score;
    if (n < 0)
        n = 0;
    for (i = 5; i >= 0; i--) {
        digits[i] = (char)('0' + (int)(n % 10));
        n /= 10;
    }
    digits[6] = 0;
    SDL_snprintf(line, sizeof(line), "Your Score: %s", digits);
    text_draw(renderer, (float)(ox + 64), (float)(oy + 60), line, 9, 1, 0, 0, 0);
    over_ok(scores, &ok);
    button(renderer, &ok, "OK", 1);
}

/* orig: RunNameDialog (000D:062C) / NameDlgProc (000D:0645) / NamePaintProc (000D:05BC) */
void scores_draw_name(SDL_Renderer *renderer, const Scores *scores)
{
    int ox, oy;
    SDL_FRect frame, pic, edit, ok;
    float text_w;
    const char *prompt = "What is your name?";

    if (!scores)
        return;
    origin(199, 114, &ox, &oy);
    box(ox, oy, 0, 0, 199, 114, &frame);
    panel(renderer, &frame);
    box(ox, oy, 6, 7, 86, 100, &pic);
    /* DrawItemPic BitBlts PPIC/250 1:1 into the control; do not stretch. */
    if (scores->portrait) {
        SDL_FRect src;
        SDL_FRect dst;
        float tw = 0;
        float th = 0;

        SDL_GetTextureSize(scores->portrait, &tw, &th);
        src.x = 0.0f;
        src.y = 0.0f;
        src.w = tw;
        src.h = th;
        dst.x = (float)(ox + 12);
        dst.y = (float)(oy + 14);
        dst.w = tw;
        dst.h = th;
        if (dst.w > pic.w) {
            src.w = pic.w;
            dst.w = pic.w;
        }
        if (dst.h > pic.h) {
            src.h = pic.h;
            dst.h = pic.h;
        }
        SDL_RenderTexture(renderer, scores->portrait, &src, &dst);
    }
    draw_wrapped(renderer, (float)(ox + 100 * 2), (float)(oy + 10 * 2), 92.0f * 2.0f,
                 "Congratulations!  You have achieved a high score!");
    /* SS_CENTER in (103,40,87,10) → 174 px wide at (ox+206, oy+80) */
    text_draw(renderer,
              (float)(ox + 206) + (174.0f - (float)text_width(prompt, 9, 1)) / 2.0f,
              (float)(oy + 80), prompt, 9, 1, 0, 0, 0);
    box(ox, oy, 100, 57, 93, 12, &edit);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &edit);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &edit);
    text_w = (float)text_width(scores->name, 9, 1);
    if (scores->replace && scores->name[0]) {
        SDL_FRect sel;

        sel.x = edit.x + 4.0f;
        sel.y = edit.y + 4.0f;
        sel.w = text_w;
        sel.h = (float)text_line(9, 1);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &sel);
        text_draw(renderer, edit.x + 4.0f, edit.y + 4.0f, scores->name, 9, 1, 255, 255, 255);
    } else
        text_draw(renderer, edit.x + 4.0f, edit.y + 4.0f, scores->name, 9, 1, 0, 0, 0);
    if (!scores->replace && ((SDL_GetTicks() / 500) & 1) == 0) {
        SDL_FRect caret;

        caret.x = edit.x + 4.0f + text_w;
        caret.y = edit.y + 4.0f;
        caret.w = 1.0f;
        caret.h = (float)text_line(9, 1);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &caret);
    }
    name_ok(&ok);
    button(renderer, &ok, "OK", 1);
}

/* orig: OpenNext scripts — same bytes as g_abNextA / g_abNextB */
static const uint8_t k_line_a[] = {
    0x03, 0x08, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x02, 0x02, 0x04, 0x08, 0x80, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x0A, 0x00
};
static const uint8_t k_line_b[] = {
    0x03, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x02, 0x04, 0x08, 0x80, 0x00, 0x00, 0xFF,
    0x00, 0x00, 0x0A, 0x00
};

/* orig: OpenNext (0012:007E) / AddNextSprite (0012:0000) */
static void open_bars(Scores *scores, const char *resources)
{
    char path[1024];
    void *file;
    size_t nbytes = 0;
    const uint8_t *script[2];
    int i;

    if (!resources || !scores->renderer)
        return;
    script[0] = k_line_a;
    script[1] = k_line_b;
    SDL_snprintf(path, sizeof(path), "%s/AACOLOR/PPIC/5000.pic", resources);
    file = SDL_LoadFile(path, &nbytes);
    if (!file)
        return;
    for (i = 0; i < 2; i++) {
        Sprite *s;

        scores->bar_pic[i] = pic_decode_ppic(file, nbytes, i);
        /* InitScene rate 2; SubclassAnimItem timer rate 3 (~51 ms). */
        scores->bar[i] = scene_new(2);
        scores->bar_canvas[i] = pic_canvas(0x202, 0x23, 255, 204, 153);
        scores->bar_tex[i] = SDL_CreateTexture(scores->renderer, SDL_PIXELFORMAT_RGBA32,
                                               SDL_TEXTUREACCESS_STATIC, 0x202, 0x23);
        if (scores->bar_tex[i])
            SDL_SetTextureScaleMode(scores->bar_tex[i], SDL_SCALEMODE_NEAREST);
        if (!scores->bar_pic[i] || !scores->bar[i])
            continue;
        s = scene_add(scores->bar[i], scores->bar_pic[i], NULL, 0, 0, 0);
        if (!s)
            continue;
        s->visible = 0;
        sprite_set_script(s, script[i]);
    }
    SDL_free(file);
}

/* orig: NextItemProc timer (0012:0182) — 51 ms = rate 3 × 17.
   Accumulate like select_tick so heavy board frames do not lose pace, but
   discard debt after a long hide (post-game / attract gap) so catch-up
   does not run at max steps for many frames. */
static void tick_bars(Scores *scores)
{
    uint32_t now = (uint32_t)SDL_GetTicks();
    int steps;
    int i;

    if (!scores->bar[0] && !scores->bar[1])
        return;
    if (scores->bar_ms == 0 || now - scores->bar_ms > 500) {
        scores->bar_ms = now ? now : 1;
        return;
    }
    if (now - scores->bar_ms < 51)
        return;
    steps = (int)((now - scores->bar_ms) / 51);
    if (steps > 3)
        steps = 3;
    scores->bar_ms += (uint32_t)steps * 51;
    for (i = 0; i < 2; i++) {
        if (scores->bar[i])
            scene_run(scores->bar[i], steps);
    }
}

/* orig: NextItemProc WM_PAINT — strip scene plus 2 px rule */
static void paint_bar(Scores *scores, int index, int rule)
{
    Pic *canvas = scores->bar_canvas[index];

    if (!canvas || !scores->bar_tex[index] || !scores->bar[index])
        return;
    pic_fill(canvas, 0, 0, 0x202, 0x23, 255, 204, 153);
    scene_paint(scores->bar[index], canvas, 0x202, 0x23);
    if (rule == 1)
        pic_fill(canvas, 0, 0x23 - 2, 0x202, 2, 0, 0, 0);
    else
        pic_fill(canvas, 0, 0, 0x202, 2, 0, 0, 0);
    SDL_UpdateTexture(scores->bar_tex[index], NULL, canvas->rgba, 0x202 * 4);
}

/* orig: DrawRankRow (0013:003C) / DrawRankList (0013:0152) / TextOutRight (0013:0000)
   Rank label is always black; only score and name take the hot colour. */
static void draw_rank(SDL_Renderer *renderer, const List *list, int hot, int x, int ybase, int first)
{
    int i;
    int asc = text_ascent(9, 0);

    for (i = 0; i < 5; i++) {
        char sz[32];
        int rank = first + i;
        const Entry *e = (list && rank <= list->count) ? &list->entry[rank - 1] : NULL;
        int y = ybase + i * 0x0E;
        uint8_t red = hot == rank ? 255 : 0;
        uint8_t green = hot == rank ? 51 : 0;

        SDL_snprintf(sz, sizeof(sz), "%d)", rank);
        text_draw(renderer, (float)(x - text_width(sz, 9, 0)), (float)(y - asc),
                  sz, 9, 0, 0, 0, 0);
        if (e && e->score) {
            SDL_snprintf(sz, sizeof(sz), "%ld", e->score);
            text_draw(renderer, (float)(x + 0x2F - text_width(sz, 9, 0)), (float)(y - asc),
                      sz, 9, 0, red, green, 0);
            text_draw(renderer, (float)(x + 0x36), (float)(y - asc), e->name, 9, 0, red, green, 0);
        }
    }
}

/* orig: title + underline inside PaintNextScores */
static void draw_title(SDL_Renderer *renderer, int x, int y, const char *title)
{
    int width = text_width(title, 12, 1);
    int asc = text_ascent(12, 1);

    text_draw(renderer, (float)x, (float)(y - asc), title, 12, 1, 0, 0, 0);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderLine(renderer, (float)(x - 8), (float)(y + 4), (float)(x + width + 8), (float)(y + 4));
}

int scores_hit_board(float x, float y)
{
    int ox, oy;
    SDL_FRect frame;

    select_place(0x202, 0x158, &ox, &oy);
    frame.x = (float)ox;
    frame.y = (float)oy;
    frame.w = 0x202;
    frame.h = 0x158;
    return inside(&frame, x, y);
}

/* orig: PaintNextScores (0013:01B4) / RunNextDialog (0012:0372)
   Body coords are relative to item 2 at y=0x23 (NextDlgProc MoveWindow). */
void scores_draw_board(SDL_Renderer *renderer, Scores *scores)
{
    int ox, oy;
    int by;
    SDL_FRect frame;
    SDL_FRect deco;
    char title[80];
    const char *game;
    const List *daily;
    const List *alltime;

    if (!scores || scores->round < 1 || scores->round > 6)
        return;
    game = k_game[scores->round - 1];
    daily = &scores->daily[scores->round - 1];
    alltime = &scores->alltime[scores->round - 1];
    select_place(0x202, 0x158, &ox, &oy);
    by = oy + 0x23;
    tick_bars(scores);
    frame.x = (float)ox;
    frame.y = (float)oy;
    frame.w = 0x202;
    frame.h = 0x158;
    SDL_SetRenderDrawColor(renderer, 255, 204, 153, 255);
    SDL_RenderFillRect(renderer, &frame);
    paint_bar(scores, 0, 1);
    paint_bar(scores, 1, 2);
    if (scores->bar_tex[0]) {
        deco.x = (float)ox;
        deco.y = (float)oy;
        deco.w = 0x202;
        deco.h = 0x23;
        SDL_RenderTexture(renderer, scores->bar_tex[0], NULL, &deco);
    }
    if (scores->bar_tex[1]) {
        deco.x = (float)ox;
        deco.y = (float)(oy + 0x135);
        deco.w = 0x202;
        deco.h = 0x23;
        SDL_RenderTexture(renderer, scores->bar_tex[1], NULL, &deco);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &frame);
    /* orig: DrawPictureAt (0013:039D) — marks relative to item 2 */
    if (scores->mark_hi) {
        deco.x = (float)(ox + 0x19F);
        deco.y = (float)(by + 0x29);
        SDL_GetTextureSize(scores->mark_hi, &deco.w, &deco.h);
        SDL_RenderTexture(renderer, scores->mark_hi, NULL, &deco);
    }
    if (scores->mark_lo) {
        deco.x = (float)(ox + 0x28);
        deco.y = (float)(by + 0xA5);
        SDL_GetTextureSize(scores->mark_lo, &deco.w, &deco.h);
        SDL_RenderTexture(renderer, scores->mark_lo, NULL, &deco);
    }
    SDL_snprintf(title, sizeof(title), "Today's %s High Scores", game);
    draw_title(renderer, ox + 0x59, by + 0x3B, title);
    SDL_snprintf(title, sizeof(title), "All-time %s High Scores", game);
    draw_title(renderer, ox + 0xAF, by + 0xB8, title);
    draw_rank(renderer, daily, scores->daily_rank, ox + 0x32, by + 0x52, 1);
    draw_rank(renderer, daily, scores->daily_rank, ox + 0x32 + 0xBA, by + 0x52, 6);
    draw_rank(renderer, alltime, scores->all_rank, ox + 0x8E, by + 0xCE, 1);
    draw_rank(renderer, alltime, scores->all_rank, ox + 0x8E + 0xBA, by + 0xCE, 6);
}

/* orig: RunResetScores (0011:0000) / ResetScoresDlgProc (0011:001C) / ResetScoreList */
void scores_reset_round(Scores *scores)
{
    List *daily;
    List *alltime;

    if (!scores)
        return;
    daily = daily_of(scores);
    alltime = all_of(scores);
    if (daily)
        daily->count = 0;
    if (alltime)
        alltime->count = 0;
    scores->daily_rank = 0;
    scores->all_rank = 0;
    save_file(scores);
}

/* orig: RunEraseScores (0011:00AF) / EraseScoresDlgProc (0011:00CB) / EraseScoreFile (0011:0132) */
void scores_erase_all(Scores *scores)
{
    int i;

    if (!scores)
        return;
    for (i = 0; i < 6; i++) {
        clear_list(&scores->daily[i], 0);
        clear_list(&scores->alltime[i], 1);
    }
    scores->daily_rank = 0;
    scores->all_rank = 0;
    if (scores->path[0])
        remove(scores->path);
}

static void confirm_layout(int all, int *ox, int *oy, SDL_FRect *yes, SDL_FRect *no)
{
    int dw = all ? 114 : 117;
    int dh = all ? 68 : 73;

    origin(dw, dh, ox, oy);
    box(*ox, *oy, 15, 39, 35, 16, yes);
    box(*ox, *oy, 65, 39, 37, 16, no);
}

void scores_draw_confirm(SDL_Renderer *renderer, int all)
{
    int ox, oy;
    SDL_FRect frame, yes, no;
    int dw = all ? 114 : 117;
    int dh = all ? 68 : 73;

    confirm_layout(all, &ox, &oy, &yes, &no);
    box(ox, oy, 0, 0, dw, dh, &frame);
    panel(renderer, &frame);
    text_block(renderer, (float)(ox + 8 * 2), (float)(oy + 6 * 2), 99.0f * 2.0f,
               all ? "Are you sure you want to clear ALL high score lists?"
                   : "Are you sure you want to clear these high score lists?",
               9, 1, 0, 0, 0);
    button(renderer, &yes, "Yes", 0);
    button(renderer, &no, "No", 0);
}

int scores_hit_confirm(float x, float y, int all)
{
    int ox, oy;
    SDL_FRect yes, no;

    confirm_layout(all, &ox, &oy, &yes, &no);
    if (inside(&yes, x, y))
        return 3;
    if (inside(&no, x, y))
        return 1;
    return 0;
}
