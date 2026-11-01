#include "text.h"

#include <ft2build.h>
#include FT_FREETYPE_H

/* Port-only FreeType path (Liberation Sans → SDL textures). Matching:
   CreateGameFont (000A:0AE0)          sizes 9/12, bold / regular
   TextOutPascal (000A:0D20)           text_draw (callers use baseline via ascent)
   GetTextExtentPascal (000A:0D51)     text_width
   ParamTextProc DrawText (000A:1325)  text_block (DT_WORDBREAK | DT_NOPREFIX) */

#define SLOTS 95

typedef struct Glyph {
    SDL_Texture *tex;
    int w, h;
    int left, top;
    int advance;
    int have;
} Glyph;

static SDL_Renderer *draw_renderer;
static FT_Library library;
static FT_Face faces[2];
static Glyph cache[2][2][SLOTS];
static int sized[2][2];
static int ascent_px[2][2];
static int line_px[2][2];

static const int k_px[2] = { 9, 12 };

static const char *k_path[2][3] = {
    {
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
    },
    {
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"
    }
};

static int size_index(int px)
{
    return px >= 11 ? 1 : 0;
}

static int face_index(int bold)
{
    if (bold && faces[1])
        return 1;
    if (faces[0])
        return 0;
    return faces[1] ? 1 : -1;
}

static void measure(int fi, int si)
{
    FT_Face face;

    if (sized[fi][si])
        return;
    face = faces[fi];
    if (!face)
        return;
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)k_px[si]);
    ascent_px[fi][si] = (int)(face->size->metrics.ascender >> 6);
    line_px[fi][si] = (int)(face->size->metrics.height >> 6);
    if (ascent_px[fi][si] < 1)
        ascent_px[fi][si] = k_px[si];
    if (line_px[fi][si] < k_px[si])
        line_px[fi][si] = k_px[si];
    sized[fi][si] = 1;
}

static Glyph *glyph_at(int bold, int px, unsigned char ch)
{
    int si = size_index(px);
    int fi = face_index(bold);
    Glyph *g;
    FT_Face face;
    FT_Bitmap *bm;
    uint8_t *rgba;
    int x, y;

    if (ch < 32 || ch > 126)
        ch = (unsigned char)'?';
    if (fi < 0)
        return NULL;
    g = &cache[fi][si][ch - 32];
    if (g->have)
        return g;
    face = faces[fi];
    measure(fi, si);
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)k_px[si]);
    if (FT_Load_Char(face, ch, FT_LOAD_RENDER) != 0) {
        g->have = 1;
        g->advance = k_px[si] / 2;
        return g;
    }
    g->advance = (int)(face->glyph->advance.x >> 6);
    g->left = face->glyph->bitmap_left;
    g->top = face->glyph->bitmap_top;
    bm = &face->glyph->bitmap;
    g->w = (int)bm->width;
    g->h = (int)bm->rows;
    if (g->w > 0 && g->h > 0 && draw_renderer && bm->pixel_mode == FT_PIXEL_MODE_GRAY) {
        rgba = SDL_malloc((size_t)g->w * (size_t)g->h * 4);
        if (rgba) {
            for (y = 0; y < g->h; y++) {
                for (x = 0; x < g->w; x++) {
                    uint8_t *p = rgba + ((size_t)y * (size_t)g->w + (size_t)x) * 4;
                    uint8_t a = bm->buffer[y * bm->pitch + x];

                    p[0] = 255;
                    p[1] = 255;
                    p[2] = 255;
                    p[3] = a;
                }
            }
            g->tex = SDL_CreateTexture(draw_renderer, SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_STATIC, g->w, g->h);
            if (g->tex) {
                SDL_SetTextureBlendMode(g->tex, SDL_BLENDMODE_BLEND);
                if (!SDL_UpdateTexture(g->tex, NULL, rgba, g->w * 4)) {
                    SDL_DestroyTexture(g->tex);
                    g->tex = NULL;
                }
            }
            SDL_free(rgba);
        }
    }
    g->have = 1;
    return g;
}

void text_open(SDL_Renderer *renderer)
{
    int bold;
    int i;

    draw_renderer = renderer;
    if (FT_Init_FreeType(&library) != 0)
        return;
    for (bold = 0; bold < 2; bold++) {
        for (i = 0; i < 3; i++) {
            if (FT_New_Face(library, k_path[bold][i], 0, &faces[bold]) == 0)
                break;
        }
    }
}

void text_close(void)
{
    int fi, si, n;

    for (fi = 0; fi < 2; fi++) {
        for (si = 0; si < 2; si++) {
            for (n = 0; n < SLOTS; n++) {
                if (cache[fi][si][n].tex)
                    SDL_DestroyTexture(cache[fi][si][n].tex);
                cache[fi][si][n].tex = NULL;
                cache[fi][si][n].have = 0;
            }
            sized[fi][si] = 0;
        }
        if (faces[fi]) {
            FT_Done_Face(faces[fi]);
            faces[fi] = NULL;
        }
    }
    if (library) {
        FT_Done_FreeType(library);
        library = NULL;
    }
    draw_renderer = NULL;
}

/* orig: GetTextExtentPascal (000A:0D51) */
int text_width(const char *s, int px, int bold)
{
    int width = 0;
    const unsigned char *p;

    if (!s)
        return 0;
    if (face_index(bold) < 0)
        return (int)SDL_strlen(s) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    for (p = (const unsigned char *)s; *p; p++) {
        Glyph *g = glyph_at(bold, px, *p);

        if (g)
            width += g->advance;
    }
    return width;
}

int text_ascent(int px, int bold)
{
    int fi = face_index(bold);
    int si = size_index(px);

    if (fi < 0)
        return SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    measure(fi, si);
    return ascent_px[fi][si];
}

int text_line(int px, int bold)
{
    int fi = face_index(bold);
    int si = size_index(px);

    if (fi < 0)
        return SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    measure(fi, si);
    return line_px[fi][si];
}

/* orig: TextOutPascal (000A:0D20) — y is line top; callers subtract ascent */
void text_draw(SDL_Renderer *renderer, float x, float y, const char *s,
               int px, int bold, uint8_t red, uint8_t green, uint8_t blue)
{
    int pen;
    int top;
    int asc;
    const unsigned char *p;

    if (!s || !s[0])
        return;
    if (face_index(bold) < 0) {
        SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
        SDL_RenderDebugText(renderer, x, y, s);
        return;
    }
    pen = (int)x;
    top = (int)y;
    asc = text_ascent(px, bold);
    for (p = (const unsigned char *)s; *p; p++) {
        Glyph *g = glyph_at(bold, px, *p);
        SDL_FRect dst;

        if (!g)
            continue;
        if (g->tex) {
            dst.x = (float)(pen + g->left);
            dst.y = (float)(top + asc - g->top);
            dst.w = (float)g->w;
            dst.h = (float)g->h;
            SDL_SetTextureColorMod(g->tex, red, green, blue);
            SDL_RenderTexture(renderer, g->tex, NULL, &dst);
        }
        pen += g->advance;
    }
}

/* orig: ParamTextProc DrawText (000A:1325) — port: left-aligned only */
void text_block(SDL_Renderer *renderer, float x, float y, float max_w, const char *text,
                int px, int bold, uint8_t red, uint8_t green, uint8_t blue)
{
    int step;

    if (!text)
        return;
    step = text_line(px, bold);
    if (step < 1)
        step = 1;
    while (text[0]) {
        char line[160];
        int n = 0;
        int last = -1;
        int width = 0;

        if (text[0] == '\n') {
            text++;
            y += (float)step;
            continue;
        }
        while (text[n] && text[n] != '\n' && n < 150) {
            char one[2];
            int adv;

            one[0] = text[n];
            one[1] = 0;
            adv = text_width(one, px, bold);
            if (n > 0 && (float)(width + adv) > max_w)
                break;
            width += adv;
            if (text[n] == ' ')
                last = n;
            n++;
        }
        if (text[n] && text[n] != '\n' && last > 0)
            n = last;
        if (n < 1)
            n = 1;
        SDL_memcpy(line, text, (size_t)n);
        line[n] = 0;
        text_draw(renderer, x, y, line, px, bold, red, green, blue);
        y += (float)step;
        text += n;
        if (text[0] == '\n')
            text++;
        while (text[0] == ' ')
            text++;
    }
}
