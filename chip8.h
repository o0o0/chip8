#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdlib.h>
/* #include "sdlctx.h" */

#define PIXEL_ON 255
#define PIXEL_OFF 0
#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32
#define CHIP8_MEM 4096
#define CHIP8_STACK 24
#define CHIP8_MAX_ROM_SIZE 4096
#define CHIP8_MATCH_OP(hex, d0, d1, d2, d3)     \
    if (((d0) < 0 || (hex)[0] == (d0))          \
        && ((d1) < 0 || (hex)[1] == (d1))       \
        && ((d2) < 0 || (hex)[2] == (d2))       \
        && ((d3) < 0 || (hex)[3] == (d3)))

#define DIGITS2HEX2(d0, d1) (((d0) << 4) + d1)
#define DIGITS2HEX3(d0, d1, d2) (((d0) << 8) + ((d1) << 4) + d2)
#define DIG2HEX2(h) (((h)[1] << 4) + (h)[2])
#define DIG2HEX3(h) (((h)[1] << 8) + ((h)[2] << 4) + (h)[3])

struct Chip8State
{
    /* memory */
    uint8_t *mem;                /* main memory */
    uint16_t stack[CHIP8_STACK]; /* the stack */
    uint8_t stack_ptr; /* index of top of the stack */

    /* registers */
    uint8_t reg[16];        /* registers 0x0 through 0xF, "V0 - VF" */
    uint16_t addr_reg;      /* address register "I" */
    uint16_t pc;            /* program counter */

    /* timers, count down at 60 Hz */
    uint8_t sound_timer;       /* sound timer - beeps when nonzero */
    uint8_t delay_timer;       /* delay timer - can be set and read */

    /* graphics data */
    uint8_t screen[CHIP8_WIDTH][CHIP8_HEIGHT];

    /* currently pressed keys */
    const uint8_t *keyboard;

    int (*key_pressed)(int key);
};

enum OpType
{
    OP_OTHER = 0,
    OP_DRAW,
    OP_KEYPRESS,
    OP_WAIT,
    OP_UNKNOWN,
};

struct Chip8State *chip8state_create(uint8_t *rom, size_t rom_size);
int chip8state_init(struct Chip8State **c8, char *rom);
void chip8state_destroy(struct Chip8State *c8);
enum OpType run_opcode(struct Chip8State *c8, uint16_t op);
enum OpType fetch_and_run(struct Chip8State *c8);
/* void draw(struct Chip8State *c8, struct SDLContext *ctx); */

#endif
