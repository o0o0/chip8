#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SDL2/SDL.h"

#define CHIP8_SCALE 10
#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32
#define CHIP8_MEM 4096
#define CHIP8_STACK 24

#define CHIP8_REG_OP(hex, d0, d1, d2, d3)       \
    if (((d0) < 0 || (hex)[0] == (d0))          \
        && ((d1) < 0 || (hex)[1] == (d1))       \
        && ((d2) < 0 || (hex)[2] == (d2))       \
        && ((d3) < 0 || (hex)[3] == (d3)))

#define DIGITS2HEX2(d0, d1) (((d0) << 4) + d1)
#define DIGITS2HEX3(d0, d1, d2) (((d0) << 8) + ((d1) << 4) + d2)


void delay(int ms)
{
    for (int i = 0; i < ms; i++) {
        SDL_PumpEvents();
        SDL_Delay(1);
    }
}

struct Chip8State
{
    /* memory */
    uint8_t *mem;                /* main memory */
    uint16_t stack[CHIP8_STACK]; /* the stack */
    int8_t stack_ptr; /* index of top of the stack, -1 if stack is empty */

    /* registers */
    uint8_t reg[16];        /* registers 0x0 through 0xF, "V0 - VF" */
    uint16_t addr_reg;      /* address register "I" */
    uint16_t pc;            /* program counter */

    /* timers, count down at 60 Hz */
    uint8_t sound_timer;       /* sound timer - beeps when nonzero */
    uint8_t delay_timer;       /* delay timer - can be set and read */

    /* graphics data */
    uint8_t screen[CHIP8_HEIGHT][CHIP8_WIDTH];
};

struct Chip8State *chip8_create()
{
    struct Chip8State *c8 = malloc(sizeof(struct Chip8State));
    c8->mem = calloc(CHIP8_MEM, sizeof(uint8_t));
    memset(c8->stack, 0, CHIP8_STACK);
    memset(c8->reg, 0, 16);
    c8->stack_ptr = -1;
    c8->addr_reg = 0;
    c8->pc = 0;
    c8->sound_timer = 0;
    c8->delay_timer = 0;
    return c8;
}

void chip8_destroy(struct Chip8State *c8)
{
    free(c8->mem);
    free(c8);
}

int run_opcode(struct Chip8State *c8, uint8_t op)
{
    uint8_t hex[4];             /* opcode hex digits, most significant first */
    hex[0] = (op >> 12) & 0xF;
    hex[1] = (op >> 8) & 0xF;
    hex[2] = (op >> 4) & 0xF;
    hex[3] = op & 0xF;

    /* 00EE - return */
    CHIP8_REG_OP(hex, 0, 0, 0xE, 0xE) {
        c8->pc = c8->stack[c8->stack_ptr];
        c8->stack[c8->stack_ptr] = 0;
        c8->stack_ptr--;
        return 0;
    }
    /* 00E0 - clear display */
    CHIP8_REG_OP(hex, 0, 0, 0xE, 0) {
        memset(c8->screen, 0, CHIP8_HEIGHT * CHIP8_WIDTH);
        c8->pc++;
        return 0;
    }
    /* 0NNN - call program at address NNN */
    CHIP8_REG_OP(hex, 0, -1, -1, -1) {
        return 0;
    }
    /* 1NNN - goto NNN */
    CHIP8_REG_OP(hex, 1, -1, -1, -1) {
        c8->pc = DIGITS2HEX3(hex[1], hex[2], hex[3]);
        return 0;
    }
    /* 2NNN - call subroutine at NNN */
    CHIP8_REG_OP(hex, 2, -1, -1, -1) {
        c8->stack_ptr++;
        c8->stack[c8->stack_ptr] = c8->pc + 1;
        c8->pc = DIGITS2HEX3(hex[1], hex[2], hex[3]);
        return 0;
    }
    /* 3XNN - skip next if VX == NN */
    CHIP8_REG_OP(hex, 3, -1, -1, -1) {
        if (c8->reg[hex[1]] == DIGITS2HEX2(hex[2], hex[3])) {
            c8->pc++;
        }
        c8->pc++;
        return 0;
    }
    /* 4XNN - skip next if VX != NN */
    CHIP8_REG_OP(hex, 4, -1, -1, -1) {
        if (c8->reg[hex[1]] != DIGITS2HEX2(hex[2], hex[3])) {
            c8->pc++;
        }
        c8->pc++;
        return 0;
    }
    /* 5XY0 - skip next if VX == VY */
    CHIP8_REG_OP(hex, 5, -1, -1, -1) {
        if (c8->reg[hex[1]] == c8->reg[hex[2]]) {
            c8->pc++;
        }
        c8->pc++;
        return 0;
    }
    /* 6XNN - set VX to NN */
    CHIP8_REG_OP(hex, 6, -1, -1, -1) {
        c8->reg[hex[1]] = DIGITS2HEX2(hex[2], hex[3]);
        c8->pc++;
        return 0;
    }
    /* 7XNN - VX += NN, carry flag not changed */
    CHIP8_REG_OP(hex, 7, -1, -1, -1) {
        c8->reg[hex[1]] += DIGITS2HEX2(hex[2], hex[3]);
        c8->pc++;
        return 0;
    }
    /* 8XY0  - VX = VY */
    CHIP8_REG_OP(hex, 8, -1, -1, 0) {
        c8->reg[hex[1]] = c8->reg[hex[2]];
        c8->pc++;
        return 0;
    }
    /* 8XY1 - VX = VX | VY */
    CHIP8_REG_OP(hex, 8, -1, -1, 1) {
        c8->reg[hex[1]] |= c8->reg[hex[2]];
        c8->pc++;
        return 0;
    }
    /* 8XY2 - VX = VX & VY */
    CHIP8_REG_OP(hex, 8, -1, -1, 2) {
        c8->reg[hex[1]] &= c8->reg[hex[2]];
        c8->pc++;
        return 0;
    }
    /* 8XY3 - VX = VX ^ VY */
    CHIP8_REG_OP(hex, 8, -1, -1, 3) {
        c8->reg[hex[1]] ^= c8->reg[hex[2]];
        c8->pc++;
        return 0;
    }
    /* 8XY4 - VX += VY, set carry flag */
    CHIP8_REG_OP(hex, 8, -1, -1, 4) {
        return 0;
    }
    /* 8XY5 - VX -= VY, set carry flag */
    CHIP8_REG_OP(hex, 8, -1, -1, 5) {
        return 0;
    }
    /* 8XY6 - store least significant bit in of VX in VF, then VX >>= 1,  */
    CHIP8_REG_OP(hex, 8, -1, -1, 6) {
        c8->reg[0xF] = c8->reg[hex[1]] & 1;
        c8->reg[hex[1]] >>= 1;
        return 0;
    }
    /* 8XY7 - VX = VY-VX, set VF to 0 if there is a borrow, 1 if not */
    CHIP8_REG_OP(hex, 8, -1, -1, 7) {
        return 0;
    }
    /* 8XYE - store most significant bit of VX in VF, then VX <<= 1 */
    CHIP8_REG_OP(hex, 8, -1, -1, 0xE) {
        /* c8-> */
        return 0;
    }
    /* 9XY0 - skip next if VX != VY */
    CHIP8_REG_OP(hex, 9, -1, -1, 0) {
        if (c8->reg[hex[1]] != c8->reg[hex[2]]) {
            c8->pc++;
        }
        c8->pc++;
        return 0;
    }
    /* ANNN - Sets I to the address NNN. */
    CHIP8_REG_OP(hex, 0xA, -1, -1, -1) {
        c8->addr_reg = DIGITS2HEX3(hex[1], hex[2], hex[3]);
        c8->pc++;
        return 0;
    }
    /* BNNN - Jumps to the address NNN plus V0. */
    CHIP8_REG_OP(hex, 0xB, -1, -1, -1) {
        c8->pc = DIGITS2HEX3(hex[1], hex[2], hex[3]) + c8->reg[0];
        return 0;
    }
    /* CXNN */
    /* Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.  */
    CHIP8_REG_OP(hex, 0xC, -1, -1, -1) {
        return 0;
    }
    /* DXYN */
    /* Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. */
    /* Each row of 8 pixels is read as bit-coded starting from memory location I; */
    /* I value doesn’t change after the execution of this instruction. */
    /* VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that doesn’t happen  */
    CHIP8_REG_OP(hex, 0xD, -1, -1, -1) {
        return 0;
    }
    /* EX9E */
    /* Skips the next instruction if the key stored in VX is pressed. */
    CHIP8_REG_OP(hex, 0xE, -1, 9, 0xE) {
        return 0;
    }
    /* EXA1 */
    /* Skips the next instruction if the key stored in VX isn't pressed. */
    CHIP8_REG_OP(hex, 0xE, -1, 0xA, 1) {
        return 0;
    }
    /* FX07 - Sets VX to the value of the delay timer. */
    CHIP8_REG_OP(hex, 0xF, -1, 0, 7) {
        c8->reg[hex[1]] = c8->delay_timer;
        c8->pc++;
        return 0;
    }
    /* FX0A */
    /* A key press is awaited, and then stored in VX. */
    /* (Blocking Operation. All instruction halted until next key event)  */
    CHIP8_REG_OP(hex, 0xF, -1, 0, 0xA) {
        return 0;
    }
    /* FX15 - Sets the delay timer to VX.  */
    CHIP8_REG_OP(hex, 0xF, -1, 1, 5) {
        return 0;
    }
    /* FX18 - Sets the sound timer to VX. */
    CHIP8_REG_OP(hex, 0xF, -1, 1, 8) {
        c8->sound_timer = c8->reg[hex[1]];
        c8->pc++;
        return 0;
    }
    /* FX1E - Adds VX to I. */
    CHIP8_REG_OP(hex, 0xF, -1, 1, 0xE) {
        c8->addr_reg += c8->reg[hex[1]];
        c8->pc++;
        return 0;
    }
    /* FX29 */
    /* Sets I to the location of the sprite for the character in VX. */
    /* Characters 0-F (in hexadecimal) are represented by a 4x5 font.  */
    CHIP8_REG_OP(hex, 0xF, -1, 2, 9) {
        return 0;
    }
    /* FX33 */
    /* Stores the binary-coded decimal representation of VX, with the most significant of three digits */
    /* at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2. */
    /* (In other words, take the decimal representation of VX, place the hundreds digit in memory at */
    /*  location in I, the tens digit at location I+1, and the ones digit at location I+2.) */
    CHIP8_REG_OP(hex, 0xF, -1, 3, 3) {
        c8->mem[c8->addr_reg] = c8->reg[hex[1]] / 100;
        c8->mem[c8->addr_reg + 1] = (c8->reg[hex[1]] / 10) % 10;
        c8->mem[c8->addr_reg + 2] = c8->reg[hex[1]] % 10;
        c8->pc++;
        return 0;
    }
    /* FX55 */
    /* Stores V0 to VX (including VX) in memory starting at address I. */
    /* The offset from I is increased by 1 for each value written, but I itself is left unmodified.  */
    CHIP8_REG_OP(hex, 0xF, -1, 5, 5) {
        for (int i = 0; i <= hex[1]; i++) {
            c8->mem[c8->addr_reg + i] = c8->reg[i];
        }
        c8->pc++;
        return 0;
    }
    /* FX65 */
    /* Fills V0 to VX (including VX) with values from memory starting at address I. */
    /* The offset from I is increased by 1 for each value written, but I itself is left unmodified.  */
    CHIP8_REG_OP(hex, 0xF, -1, 6, 5) {
        for (int i = 0; i <= hex[1]; i++) {
             c8->reg[i] = c8->mem[c8->addr_reg + i];
        }
        c8->pc++;
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        SDL_Log("No ROM file specified");
        return 1;
    }

    struct Chip8State *c8 = chip8_create();
    /* Load program into memory */


    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    SDL_Window *win = NULL;
    int width = CHIP8_SCALE * CHIP8_WIDTH;
    int height = CHIP8_SCALE * CHIP8_HEIGHT;

    win = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                           width, height, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return 1;
    }
    SDL_Surface *surface = SDL_GetWindowSurface(win);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0x00, 0x00, 0x00));
    SDL_UpdateWindowSurface(win);

    delay(2000);
    SDL_DestroyWindow(win);
    chip8_destroy(c8);

    return 0;
}
