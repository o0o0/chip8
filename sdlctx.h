#ifndef SDLCTX_H
#define SDLCTX_H

#include <stdint.h>
#include "SDL2/SDL.h"

#define CHIP8_SCALE 10
#define BACKGROUND_R 0
#define BACKGROUND_G 0
#define BACKGROUND_B 0
#define FOREGROUND_R 255
#define FOREGROUND_G 255
#define FOREGROUND_B 255

struct SDLContext
{
    SDL_Renderer *rndr;
    SDL_Window *win;
    SDL_Event ev;
};

int sdl_init(struct SDLContext *ctx, int width, int height);
void sdl_cleanup(struct SDLContext *ctx);

#endif
