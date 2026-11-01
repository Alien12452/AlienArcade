#include "pic.h"

#include <SDL3/SDL.h>

/* Matching: src/ppic.c (segment 25) and blit helpers in src/image.c.
   DecodePixImage (0019:05B9) / UnpackColumn4 (0019:096C)  pic_decode_ppic
   DecodePicImage (0019:0426)                              dispatcher (mono unused)
   SeekPicRes (0019:0187)                                  record scan
   LoadPPicImages / LoadPicResImage
     (0019:0000 / 0019:0130)                               callers' load loops
   CreateImage (000A:0391)                                 pic_canvas
   FillImage (000A:041F)                                   pic_fill
   BlitImage (000A:0615)                                   pic_blit
   CopyBlitImage (000A:0581)                               pic_blit_mode
   MaskBlitImage (000A:04FC)                               pic_blit_mask
   g_rgbCustom (0024:06B0); port always uses this (orig uses
   g_rgbVga when g_nColorDepth <= 4 via CreateGamePalette 0003:034D). */

/* orig: g_rgbCustom — Windows RGBQUAD order (B,G,R) stored here as R,G,B */
static const uint8_t k_pal[16][3] = {
    {255, 255, 255}, {0, 153, 204}, {153, 102, 204}, {255, 255, 102},
    {153, 102, 0}, {102, 204, 51}, {51, 102, 0}, {255, 153, 0},
    {255, 204, 153}, {255, 51, 0}, {136, 136, 136}, {255, 51, 255},
    {153, 255, 255}, {204, 153, 102}, {0, 0, 0}, {0, 0, 0},
};

static int be_u16(const uint8_t *p)
{
    return (p[0] << 8) | p[1];
}

static uint32_t be_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

/* orig: UnpackColumn4 (0019:096C) */
static const uint8_t *unpack_column4(const uint8_t *p, const uint8_t *end,
                                     uint8_t *dst, int col, int height, int stride)
{
    int y = height - 1;
    int left = height;

    while (left > 0) {
        int c;
        if (p >= end)
            return NULL;
        c = (int8_t)*p++;
        if (c == 0) {
            int n;
            uint16_t count;
            const uint8_t *run;
            const uint8_t *lit;
            if (p + 1 >= end)
                return NULL;
            n = *p >> 4;
            count = *p++;
            count = (count << 4) & 0xFFFFu;
            count &= 0xFFu;
            count = (count << 4) & 0xFFFFu;
            count |= *p++;
            count = (count - 1) & 0xFFFFu;
            run = p;
            lit = run;
            for (;;) {
                int cnt = n;
                lit = run;
                for (;;) {
                    if (lit >= end || y < 0)
                        return NULL;
                    dst[y * stride + col] = *lit++;
                    y--;
                    if (--cnt == -1)
                        break;
                }
                left -= n + 1;
                if (--count == 0xFFFFu)
                    break;
            }
            p = lit;
        } else if (c < 0) {
            int b;
            left += c;
            c = ~c;
            if (p >= end)
                return NULL;
            b = *p++;
            for (;;) {
                if (y < 0)
                    return NULL;
                dst[y * stride + col] = (uint8_t)b;
                y--;
                if (--c == -1)
                    break;
            }
        } else {
            left -= c;
            c--;
            for (;;) {
                if (p >= end || y < 0)
                    return NULL;
                dst[y * stride + col] = *p++;
                y--;
                if (--c == -1)
                    break;
            }
        }
    }
    return p;
}

/* orig: DecodePixImage (0019:05B9) via SeekPicRes (0019:0187) */
Pic *pic_decode_ppic(const uint8_t *data, size_t len, int index)
{
    const uint8_t *p;
    const uint8_t *end;
    const uint8_t *img_end;
    int count;
    int i;
    uint32_t nbytes;
    int flags;
    int cx_word;
    int cx;
    int width;
    int height;
    int stride;
    uint8_t *bits;
    uint8_t *rgba;
    Pic *pic;

    if (len < 2)
        return NULL;
    count = be_u16(data);
    p = data + 2;
    end = data + len;
    /* SeekPicRes also clamps index > nLast; callers only pass in-range. */
    for (i = 0; i < index && i < count; i++) {
        if (p + 4 > end)
            return NULL;
        nbytes = be_u32(p);
        if (nbytes < 4 || p + nbytes > end)
            return NULL;
        p += nbytes;
    }
    if (index >= count || p + 12 > end)
        return NULL;
    nbytes = be_u32(p);
    if (nbytes < 12 || p + nbytes > end)
        return NULL;
    img_end = p + nbytes;
    flags = be_u16(p + 4);
    cx_word = be_u16(p + 6);
    width = be_u16(p + 8);
    height = be_u16(p + 10);
    /* Bit 15 of the column word selects the colour unpacker. A negative
       flags word would skip (clut[9] << 3) + 16 bytes of embedded CLUT
       (SeekPicRes / DecodePixImage); none of the colour pictures set it. */
    if (flags >= 0x8000 || cx_word < 0x8000 || width == 0 || height == 0)
        return NULL;
    cx = cx_word & 0x3FFF;
    stride = (cx + 3) & ~3;
    if (stride <= 0 || width > stride * 2)
        return NULL;
    bits = SDL_calloc((size_t)height, (size_t)stride);
    if (!bits)
        return NULL;
    p += 12;
    for (i = 0; i < cx; i++) {
        p = unpack_column4(p, img_end, bits, i, height, stride);
        if (!p) {
            SDL_free(bits);
            return NULL;
        }
    }
    rgba = SDL_malloc((size_t)width * (size_t)height * 4);
    if (!rgba) {
        SDL_free(bits);
        return NULL;
    }
    for (int y = 0; y < height; y++) {
        const uint8_t *row = bits + (size_t)(height - 1 - y) * (size_t)stride;
        uint8_t *out = rgba + (size_t)y * (size_t)width * 4;
        for (int x = 0; x < width; x++) {
            int idx = (x & 1) ? (row[x / 2] & 0x0F) : (row[x / 2] >> 4);
            out[0] = k_pal[idx][0];
            out[1] = k_pal[idx][1];
            out[2] = k_pal[idx][2];
            out[3] = 255;
            out += 4;
        }
    }
    SDL_free(bits);
    pic = SDL_malloc(sizeof(*pic));
    if (!pic) {
        SDL_free(rgba);
        return NULL;
    }
    pic->w = width;
    pic->h = height;
    pic->rgba = rgba;
    return pic;
}

/* orig: CreateImage (000A:0391) / InitScene off-screen image */
Pic *pic_canvas(int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    Pic *pic;
    int i;
    int n;

    if (w <= 0 || h <= 0)
        return NULL;
    pic = SDL_malloc(sizeof(*pic));
    if (!pic)
        return NULL;
    pic->w = w;
    pic->h = h;
    pic->rgba = SDL_malloc((size_t)w * (size_t)h * 4);
    if (!pic->rgba) {
        SDL_free(pic);
        return NULL;
    }
    n = w * h;
    for (i = 0; i < n; i++) {
        pic->rgba[i * 4 + 0] = r;
        pic->rgba[i * 4 + 1] = g;
        pic->rgba[i * 4 + 2] = b;
        pic->rgba[i * 4 + 3] = 255;
    }
    return pic;
}

/* orig: FillImage (000A:041F) */
void pic_fill(Pic *dst, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    int row;
    int col;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > dst->w)
        w = dst->w - x;
    if (y + h > dst->h)
        h = dst->h - y;
    for (row = 0; row < h; row++) {
        uint8_t *p = dst->rgba + ((size_t)(y + row) * (size_t)dst->w + (size_t)x) * 4;
        for (col = 0; col < w; col++) {
            p[0] = r;
            p[1] = g;
            p[2] = b;
            p[3] = 255;
            p += 4;
        }
    }
}

/* orig: CopyBlitImage (000A:0581) — modes 0..3 = SRCCOPY/AND/MERGEPAINT/INVERT */
void pic_blit_mode(Pic *dst, int x, int y, const Pic *src, int sw, int sh, int mode)
{
    int sx = 0;
    int sy = 0;
    int row;
    int col;

    if (sw > src->w)
        sw = src->w;
    if (sh > src->h)
        sh = src->h;

    if (x < 0) {
        sx = -x;
        sw += x;
        x = 0;
    }
    if (y < 0) {
        sy = -y;
        sh += y;
        y = 0;
    }
    if (x + sw > dst->w)
        sw = dst->w - x;
    if (y + sh > dst->h)
        sh = dst->h - y;
    if (sw <= 0 || sh <= 0)
        return;
    if (mode <= 0) {
        for (row = 0; row < sh; row++) {
            uint8_t *d = dst->rgba + ((size_t)(y + row) * (size_t)dst->w + (size_t)x) * 4;
            const uint8_t *s = src->rgba + ((size_t)(sy + row) * (size_t)src->w + (size_t)sx) * 4;
            SDL_memcpy(d, s, (size_t)sw * 4);
        }
        return;
    }
    for (row = 0; row < sh; row++) {
        uint8_t *d = dst->rgba + ((size_t)(y + row) * (size_t)dst->w + (size_t)x) * 4;
        const uint8_t *s = src->rgba + ((size_t)(sy + row) * (size_t)src->w + (size_t)sx) * 4;
        for (col = 0; col < sw; col++) {
            if (mode == 1) {
                d[0] = (uint8_t)(s[0] & d[0]);
                d[1] = (uint8_t)(s[1] & d[1]);
                d[2] = (uint8_t)(s[2] & d[2]);
            } else if (mode == 2) {
                d[0] = (uint8_t)((uint8_t)~s[0] | d[0]);
                d[1] = (uint8_t)((uint8_t)~s[1] | d[1]);
                d[2] = (uint8_t)((uint8_t)~s[2] | d[2]);
            } else if (mode == 3) {
                /* Modes outside 0..3 are unused; orig left dwRop uninitialised. */
                d[0] = (uint8_t)(s[0] ^ d[0]);
                d[1] = (uint8_t)(s[1] ^ d[1]);
                d[2] = (uint8_t)(s[2] ^ d[2]);
            }
            d[3] = 255;
            d += 4;
            s += 4;
        }
    }
}

/* orig: BlitImage (000A:0615) SRCCOPY */
void pic_blit(Pic *dst, int x, int y, const Pic *src, int sw, int sh)
{
    pic_blit_mode(dst, x, y, src, sw, sh, 0);
}

/* orig: MaskBlitImage (000A:04FC) */
void pic_blit_mask(Pic *dst, int x, int y, const Pic *sprite, const Pic *mask,
                   int clip_x, int clip_y, int clip_w, int clip_h)
{
    int sw = sprite->w;
    int sh = sprite->h;
    int sx = 0;
    int sy = 0;
    int row;
    int col;
    int clip_r = clip_x + clip_w;
    int clip_b = clip_y + clip_h;

    /* port: clamp to the smaller of sprite/mask; orig blits the sprite rect. */
    if (mask->w < sw)
        sw = mask->w;
    if (mask->h < sh)
        sh = mask->h;
    if (x < clip_x) {
        sx += clip_x - x;
        sw -= clip_x - x;
        x = clip_x;
    }
    if (y < clip_y) {
        sy += clip_y - y;
        sh -= clip_y - y;
        y = clip_y;
    }
    if (x + sw > clip_r)
        sw = clip_r - x;
    if (y + sh > clip_b)
        sh = clip_b - y;
    if (x < 0) {
        sx -= x;
        sw += x;
        x = 0;
    }
    if (y < 0) {
        sy -= y;
        sh += y;
        y = 0;
    }
    if (x + sw > dst->w)
        sw = dst->w - x;
    if (y + sh > dst->h)
        sh = dst->h - y;
    if (sw <= 0 || sh <= 0)
        return;
    for (row = 0; row < sh; row++) {
        uint8_t *d = dst->rgba + ((size_t)(y + row) * (size_t)dst->w + (size_t)x) * 4;
        const uint8_t *s = sprite->rgba + ((size_t)(sy + row) * (size_t)sprite->w + (size_t)sx) * 4;
        const uint8_t *m = mask->rgba + ((size_t)(sy + row) * (size_t)mask->w + (size_t)sx) * 4;
        for (col = 0; col < sw; col++) {
            /* SRCERASE then SRCINVERT: (mask & ~dest) ^ sprite. With white
               sprite fill and a white-outside / black-inside mask this is
               exact over any background (transparent: leaves dest). */
            d[0] = (uint8_t)((m[0] & (uint8_t)~d[0]) ^ s[0]);
            d[1] = (uint8_t)((m[1] & (uint8_t)~d[1]) ^ s[1]);
            d[2] = (uint8_t)((m[2] & (uint8_t)~d[2]) ^ s[2]);
            d[3] = 255;
            d += 4;
            s += 4;
            m += 4;
        }
    }
}

void pic_free(Pic *pic)
{
    if (!pic)
        return;
    SDL_free(pic->rgba);
    SDL_free(pic);
}
