/* Port-only FreeType stand-in for CreateGameFont's Helv (src/image.c).
   CreateGameFont (000A:0AE0) / TextOutPascal (000A:0D20) /
   GetTextExtentPascal (000A:0D51) / ParamTextProc DrawText (000A:1325).
   Liberation Sans when installed; otherwise the debug font. Heights match
   the game: 9 bold UI, 12 bold score titles, 9 regular rows. y is line top. */

#ifndef TEXT_H
#define TEXT_H

#include <SDL3/SDL.h>

void text_open(SDL_Renderer *renderer);
void text_close(void);

void text_draw(SDL_Renderer *renderer, float x, float y, const char *s,
               int px, int bold, uint8_t red, uint8_t green, uint8_t blue);
void text_block(SDL_Renderer *renderer, float x, float y, float max_w, const char *text,
                int px, int bold, uint8_t red, uint8_t green, uint8_t blue);
int text_width(const char *s, int px, int bold);
int text_line(int px, int bold);
int text_ascent(int px, int bold);

#endif
