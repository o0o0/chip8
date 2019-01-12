#include "sdlctx.h"


int sdl_init(struct SDLContext *ctx, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    ctx->win = SDL_CreateWindow("Chip8",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                width, height, SDL_WINDOW_SHOWN);
    if (ctx->win == NULL) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return 1;
    }

    ctx->rndr = SDL_CreateRenderer(ctx->win, -1, 0);
    return 0;
}

void sdl_cleanup(struct SDLContext *ctx)
{
    SDL_DestroyRenderer(ctx->rndr);
    SDL_DestroyWindow(ctx->win);
    SDL_Quit();
}
