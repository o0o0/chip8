#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "chip8.h"
#include "sdlctx.h"

const uint8_t keymap[16] = {
    SDL_SCANCODE_B,                          /* 0 */
    SDL_SCANCODE_4,                          /* 1 */
    SDL_SCANCODE_5,                          /* 2 */
    SDL_SCANCODE_6,                          /* 3 */
    SDL_SCANCODE_R,                          /* 4 */
    SDL_SCANCODE_T,                          /* 5 */
    SDL_SCANCODE_Y,                          /* 6 */
    SDL_SCANCODE_F,                          /* 7 */
    SDL_SCANCODE_G,                          /* 8 */
    SDL_SCANCODE_H,                          /* 9 */
    SDL_SCANCODE_V,                          /* A */
    SDL_SCANCODE_N,                          /* B */
    SDL_SCANCODE_7,                          /* C */
    SDL_SCANCODE_U,                          /* D */
    SDL_SCANCODE_J,                          /* E */
    SDL_SCANCODE_M,                          /* F */
};


int key_pressed(int key)
{
    const uint8_t *keyboard = SDL_GetKeyboardState(NULL);
    return keyboard[keymap[key]];
}

void draw(struct Chip8State *c8, struct SDLContext *ctx)
{
    SDL_Renderer *rndr = ctx->rndr;
    /* Clear to background color */
    SDL_SetRenderDrawColor(rndr, BACKGROUND_R, BACKGROUND_G, BACKGROUND_B, 255);
    SDL_RenderClear(rndr);

    /* Draw foreground pixels */
    SDL_SetRenderDrawColor(rndr, FOREGROUND_R, FOREGROUND_G, FOREGROUND_B, 255);
    SDL_Rect p;
    p.w = CHIP8_SCALE;
    p.h = CHIP8_SCALE;
    for (int x = 0; x < CHIP8_WIDTH; x++) {
        for (int y = 0; y < CHIP8_HEIGHT; y++) {
            if (c8->screen[x][y] == PIXEL_ON) {
                p.x = x * CHIP8_SCALE;
                p.y = y * CHIP8_SCALE;
                SDL_RenderFillRect(rndr, &p);
            }
        }
    }
    SDL_RenderPresent(rndr);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "No ROM file specified\n");
        return 1;
    }

    struct Chip8State *c8 = NULL;
    chip8state_init(&c8, argv[1]);
    c8->key_pressed = key_pressed;

    struct SDLContext ctx;
    if (sdl_init(&ctx, CHIP8_SCALE*CHIP8_WIDTH, CHIP8_SCALE*CHIP8_HEIGHT) != 0) {
        return 1;
    }

    int quit = 0;

    int clock_speed = 1000;
    if (argc >= 3) {
        clock_speed = atoi(argv[2]);
    }
    long cycle_ns = 1000000000UL / clock_speed;

#ifdef DEBUG
    /* print_state(c8); */
    /* print_screen(c8); */
#endif

    uint32_t cycle_start = 0, prev = 0, elapsed;

    struct timespec time, prevtime, diff;
    clock_gettime(CLOCK_MONOTONIC, &time);

    while (!quit) {
        cycle_start = SDL_GetTicks();
        prevtime = time;
        clock_gettime(CLOCK_MONOTONIC, &time);

        /* Compute time difference between cycles */
        if (time.tv_nsec - prevtime.tv_nsec < 0) {
            diff.tv_sec = time.tv_sec - prevtime.tv_sec - 1;
            diff.tv_nsec = time.tv_nsec - prevtime.tv_nsec + 1000000000UL;
        } else {
            diff.tv_sec = time.tv_sec - prevtime.tv_sec;
            diff.tv_nsec = time.tv_nsec - prevtime.tv_nsec;
        }

        while (SDL_PollEvent(&ctx.ev) != 0) {
            if (ctx.ev.type == SDL_QUIT) {
#ifdef DEBUG
                printf("QUITTING...\n");
#endif
                quit = 1;
            }
        }
        c8->keyboard = SDL_GetKeyboardState(NULL);

        /* 60 Hz countdown */
        if ((cycle_start - prev) >= 16 && c8->delay_timer) {
            printf("60Hz tick\n");
            if (c8->delay_timer) {
                c8->delay_timer--;
            }
            if (c8->sound_timer) {
                c8->sound_timer--;
            }
            prev = cycle_start;
        }

        enum OpType op = fetch_and_run(c8);

        if (op == OP_DRAW) {
            draw(c8, &ctx);
        } else if (op == OP_WAIT) {
            /* block, waiting for input */
        } else if (op == OP_KEYPRESS) {
        }

        elapsed = SDL_GetTicks() - cycle_start;

#ifdef DEBUG
        /* print_state(c8); */
        /* print_screen(c8); */
#endif
        long wait = cycle_ns > diff.tv_nsec ? (cycle_ns - diff.tv_nsec) / 1000 : 0;

#ifdef DEBUG
        printf("wait: %ld\n", wait);
#endif

        usleep(wait);
    }

    sdl_cleanup(&ctx);
    chip8state_destroy(c8);

    return 0;
}
