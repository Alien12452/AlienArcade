/* Off-screen pictures. Matching: src/ppic.c (segment 25) and blit helpers
   in src/image.c.
   DecodePixImage (0019:05B9) / UnpackColumn4 (0019:096C)  pic_decode_ppic
   SeekPicRes (0019:0187)                                  record scan
   LoadPPicImages / LoadPicResImage
     (0019:0000 / 0019:0130)                               callers' load loops
   CreateImage / FillImage / BlitImage
     (000A:0391 / 000A:041F / 000A:0615)                   canvas / fill / blit
   CopyBlitImage (000A:0581) / MaskBlitImage (000A:04FC)   blit_mode / blit_mask
   g_rgbCustom (0024:06B0); CreateGamePalette (0003:034D)
   int is 16 bits in the original, so extended-run lengths use 16-bit shifts. */

#ifndef PIC_H
#define PIC_H

#include <stddef.h>
#include <stdint.h>

typedef struct Pic {
    int w;
    int h;
    uint8_t *rgba; /* top-down, four bytes per pixel */
} Pic;

/* index selects one image in a PPIC resource. The caller frees with pic_free. */
Pic *pic_decode_ppic(const uint8_t *data, size_t len, int index);
Pic *pic_canvas(int w, int h, uint8_t r, uint8_t g, uint8_t b);
void pic_fill(Pic *dst, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
/* SRCCOPY of the source's top-left sw by sh. A negative destination skips
   that many source pixels, the way BitBlt clips. The row stride stays src->w. */
void pic_blit(Pic *dst, int x, int y, const Pic *src, int sw, int sh);
/* CopyBlitImage modes: 0 SRCCOPY, 1 SRCAND, 2 MERGEPAINT, 3 SRCINVERT. */
void pic_blit_mode(Pic *dst, int x, int y, const Pic *src, int sw, int sh, int mode);
/* MaskBlitImage: dest = ((mask AND NOT dest) XOR sprite), clipped to the rectangle.
   Exact over any background when the sprite fill is white and the mask is
   white outside / black inside the silhouette. */
void pic_blit_mask(Pic *dst, int x, int y, const Pic *sprite, const Pic *mask,
                   int clip_x, int clip_y, int clip_w, int clip_h);
void pic_free(Pic *pic);

#endif
