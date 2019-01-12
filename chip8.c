#include <stdio.h>
#include <string.h>
#include "chip8.h"
#ifdef DEBUG
#include "debug.h"
#endif


uint8_t random_byte(void)
{
    return (uint8_t)(rand() % 256);
}

/* Builtin font sprites */
const uint8_t sprite_data[16][5] = {
    {0xF0, 0x90, 0x90, 0x90, 0xF0,}, /* 0 */
    {0x20, 0x60, 0x20, 0x20, 0x70,}, /* 1 */
    {0xF0, 0x10, 0xF0, 0x80, 0xF0,}, /* 2 */
    {0xF0, 0x10, 0xF0, 0x10, 0xF0,}, /* 3 */
    {0x90, 0x90, 0xF0, 0x10, 0x10,}, /* 4 */
    {0xF0, 0x80, 0xF0, 0x10, 0xF0,}, /* 5 */
    {0xF0, 0x80, 0xF0, 0x90, 0xF0,}, /* 6 */
    {0xF0, 0x10, 0x20, 0x40, 0x40,}, /* 7 */
    {0xF0, 0x90, 0xF0, 0x90, 0xF0,}, /* 8 */
    {0xF0, 0x90, 0xF0, 0x10, 0xF0,}, /* 9 */
    {0xF0, 0x90, 0xF0, 0x90, 0x90,}, /* A */
    {0xE0, 0x90, 0xE0, 0x90, 0xE0,}, /* B */
    {0xF0, 0x80, 0x80, 0x80, 0xF0,}, /* C */
    {0xE0, 0x90, 0x90, 0x90, 0xE0,}, /* D */
    {0xF0, 0x80, 0xF0, 0x80, 0xF0,}, /* E */
    {0xF0, 0x80, 0xF0, 0x80, 0x80,}, /* F */
};

struct Chip8State *chip8state_create(uint8_t *rom, size_t rom_size)
{
    struct Chip8State *c8 = calloc(1, sizeof(struct Chip8State));
    c8->mem = calloc(CHIP8_MEM, sizeof(uint8_t));

    /* initialize hex digit sprite data */
    memcpy(c8->mem, sprite_data, 16*5);

    /* load program into memory */
    c8->pc = 0x200;
    memcpy(c8->mem + 0x200, rom, rom_size);

    /* c8->keyboard = SDL_GetKeyboardState(NULL); */
    c8->keyboard = NULL;
    return c8;
}

void chip8state_destroy(struct Chip8State *c8)
{
    free(c8->mem);
    free(c8);
}

enum OpType run_opcode(struct Chip8State *c8, uint16_t op)
{
    uint8_t hex[4];             /* opcode hex digits, most significant first */
    hex[0] = (op >> 12) & 0xF;
    hex[1] = (op >> 8) & 0xF;
    hex[2] = (op >> 4) & 0xF;
    hex[3] = op & 0xF;

    /* 00EE - return */
    CHIP8_MATCH_OP(hex, 0, 0, 0xE, 0xE) {
        c8->pc = c8->stack[c8->stack_ptr];
        c8->stack[c8->stack_ptr] = 0;
        c8->stack_ptr--;
        return 0;
    }
    /* 00E0 - clear display */
    CHIP8_MATCH_OP(hex, 0, 0, 0xE, 0) {
        memset(c8->screen, 0, CHIP8_HEIGHT * CHIP8_WIDTH);
        c8->pc += 2;
        return OP_DRAW;
    }
    /* 0NNN - call program at address NNN */
    CHIP8_MATCH_OP(hex, 0, -1, -1, -1) {
        /* ignored */
        c8->pc += 2;
        return 0;
    }
    /* 1NNN - goto NNN */
    CHIP8_MATCH_OP(hex, 1, -1, -1, -1) {
        c8->pc = DIGITS2HEX3(hex[1], hex[2], hex[3]);
        return 0;
    }
    /* 2NNN - call subroutine at NNN */
    CHIP8_MATCH_OP(hex, 2, -1, -1, -1) {
        c8->stack_ptr++;
        c8->stack[c8->stack_ptr] = c8->pc + 2;
        c8->pc = DIGITS2HEX3(hex[1], hex[2], hex[3]);
        return 0;
    }
    /* 3XNN - skip next if VX == NN */
    CHIP8_MATCH_OP(hex, 3, -1, -1, -1) {
        if (c8->reg[hex[1]] == DIGITS2HEX2(hex[2], hex[3])) {
            /* printf("skipping\n"); */
            c8->pc += 2;
        }
        c8->pc += 2;
        return 0;
    }
    /* 4XNN - skip next if VX != NN */
    CHIP8_MATCH_OP(hex, 4, -1, -1, -1) {
        if (c8->reg[hex[1]] != DIGITS2HEX2(hex[2], hex[3])) {
            /* printf("skipping\n"); */
            c8->pc += 2;
        }
        c8->pc += 2;
        return 0;
    }
    /* 5XY0 - skip next if VX == VY */
    CHIP8_MATCH_OP(hex, 5, -1, -1, -1) {
        if (c8->reg[hex[1]] == c8->reg[hex[2]]) {
            /* printf("skipping\n"); */
            c8->pc += 2;
        }
        c8->pc += 2;
        return 0;
    }
    /* 6XNN - set VX to NN */
    CHIP8_MATCH_OP(hex, 6, -1, -1, -1) {
        int val = DIGITS2HEX2(hex[2], hex[3]);
        c8->reg[hex[1]] = val;
        c8->pc += 2;
        return 0;
    }
    /* 7XNN - VX += NN, carry flag not changed */
    CHIP8_MATCH_OP(hex, 7, -1, -1, -1) {
        c8->reg[hex[1]] += DIGITS2HEX2(hex[2], hex[3]);
        c8->pc += 2;
        return 0;
    }
    /* 8XY0  - VX = VY */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 0) {
        c8->reg[hex[1]] = c8->reg[hex[2]];
        c8->pc += 2;
        return 0;
    }
    /* 8XY1 - VX = VX | VY */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 1) {
        c8->reg[hex[1]] |= c8->reg[hex[2]];
        c8->pc += 2;
        return 0;
    }
    /* 8XY2 - VX = VX & VY */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 2) {
        c8->reg[hex[1]] &= c8->reg[hex[2]];
        c8->pc += 2;
        return 0;
    }
    /* 8XY3 - VX = VX ^ VY */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 3) {
        c8->reg[hex[1]] ^= c8->reg[hex[2]];
        c8->pc += 2;
        return 0;
    }
    /* 8XY4 - VX += VY, set carry flag */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 4) {
        uint16_t sum = c8->reg[hex[1]] + c8->reg[hex[2]];
        if (sum > 255) {
            c8->reg[0xF] = 1;
        }
        c8->reg[hex[1]] = (uint8_t)sum;
        c8->pc += 2;
        return 0;
    }
    /* 8XY5 - VX -= VY, set carry flag */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 5) {
        if (c8->reg[hex[1]] < c8->reg[hex[2]]) {
            c8->reg[0xF] = 1;
            c8->reg[hex[1]] = 0;
        } else {
            c8->reg[0xF] = 0;
            c8->reg[hex[1]] -= c8->reg[hex[2]];
        }
        c8->pc += 2;
        return 0;
    }
    /* 8XY6 - store least significant bit in of VX in VF, then VX >>= 1,  */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 6) {
        c8->reg[0xF] = c8->reg[hex[1]] & 1;
        c8->reg[hex[1]] >>= 1;
        c8->pc += 2;
        return 0;
    }
    /* 8XY7 - VX = VY-VX, set VF to 0 if there is a borrow, 1 if not */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 7) {
        c8->reg[0xF] = c8->reg[hex[1]] > c8->reg[hex[2]] ? 1 : 0;
        c8->reg[hex[1]] = c8->reg[hex[2]] - c8->reg[hex[1]];
        c8->pc += 2;
        return 0;
    }
    /* 8XYE - store most significant bit of VX in VF, then VX <<= 1 */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 0xE) {
        /* c8-> */
        c8->reg[0xF] = (c8->reg[hex[1]] >> 7) & 1;
        c8->reg[hex[1]] <<= 1;
        c8->pc += 2;
        return 0;
    }
    /* 9XY0 - skip next if VX != VY */
    CHIP8_MATCH_OP(hex, 9, -1, -1, 0) {
        if (c8->reg[hex[1]] != c8->reg[hex[2]]) {
            c8->pc += 2;
        }
        c8->pc += 2;
        return 0;
    }
    /* ANNN - Sets I to the address NNN. */
    CHIP8_MATCH_OP(hex, 0xA, -1, -1, -1) {
        c8->addr_reg = DIGITS2HEX3(hex[1], hex[2], hex[3]);
        c8->pc += 2;
        return 0;
    }
    /* BNNN - Jumps to the address NNN plus V0. */
    CHIP8_MATCH_OP(hex, 0xB, -1, -1, -1) {
        c8->pc = DIGITS2HEX3(hex[1], hex[2], hex[3]) + c8->reg[0];
        return 0;
    }
    /* CXNN - VX = rand() & NN */
    CHIP8_MATCH_OP(hex, 0xC, -1, -1, -1) {
        c8->reg[hex[1]] = DIGITS2HEX2(hex[2], hex[3]) & random_byte();
        c8->pc += 2;
        return 0;
    }
    /* DXYN - draw sprite at (VX, VY), width of 8 pixels and height of N pixels */
    /* Each row of 8 pixels is read starting from memory location I; */
    /* VF is set to 1 if any screen pixels are flipped from set to unset, 0 otherwise */
    CHIP8_MATCH_OP(hex, 0xD, -1, -1, -1) {
        uint8_t *sprite = c8->mem + c8->addr_reg;
        int x = c8->reg[hex[1]];
        int y = c8->reg[hex[2]];
        /* printf("Drawing at (x, y) = (%d, %d)\n", x, y); */
        int flipped = 0;
        for (int r = 0; r < hex[3]; r++) {
            x = c8->reg[hex[1]];
            for (int c = 0; c < 8; c++) {
                int val = (*sprite >> (7 - c)) & 1;

                if (c8->screen[x][y] == PIXEL_ON && val) {
                    flipped = 1;
                    c8->screen[x][y] = PIXEL_OFF;
                } else if (val) {
                    c8->screen[x][y] = PIXEL_ON;
                }
                x++;
                if (x >= CHIP8_WIDTH) {
                    x = 0;
                }
            }
            sprite++;
            y++;
            if (y >= CHIP8_HEIGHT) {
                y = 0;
            }
        }
        c8->reg[0xF] = flipped;
        c8->pc += 2;
        return OP_DRAW;
    }
    /* EX9E - skip next if key stored in VX is pressed */
    CHIP8_MATCH_OP(hex, 0xE, -1, 9, 0xE) {
        /* if (c8->keyboard[keymap[c8->reg[hex[1]]]]) { */
        if (!c8->key_pressed(c8->reg[hex[1]])) {
            /* printf("Key was pressed, skipping...\n"); */
            c8->pc += 2;
        }
        c8->pc += 2;
        return OP_KEYPRESS;
    }
    /* EXA1 - skip next if key stored in VX is not pressed */
    CHIP8_MATCH_OP(hex, 0xE, -1, 0xA, 1) {
        /* if (!c8->keyboard[keymap[c8->reg[hex[1]]]]) { */
        if (!c8->key_pressed(c8->reg[hex[1]])) {
            /* printf("Key was not pressed, skipping...\n"); */
            c8->pc += 2;
        }
        c8->pc += 2;
        return OP_KEYPRESS;
    }
    /* FX07 - Sets VX to the value of the delay timer. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 0, 7) {
        c8->reg[hex[1]] = c8->delay_timer;
        c8->pc += 2;
        return 0;
    }
    /* FX0A - wait for a key press (blocking) and store it in VX */
    CHIP8_MATCH_OP(hex, 0xF, -1, 0, 0xA) {
        printf("Waiting for keypress..\n");
        for (uint8_t i = 0; i < 16; i++) {
            /* if (c8->keyboard[keymap[i]]) { */
            if (c8->key_pressed(i)) {
                printf("Key was pressed\n");
                c8->reg[hex[1]] = i;
                c8->pc += 2;
                break;
            }
        }
        return OP_WAIT;
    }
    /* FX15 - Sets the delay timer to VX.  */
    CHIP8_MATCH_OP(hex, 0xF, -1, 1, 5) {
        c8->delay_timer = c8->reg[hex[1]];
        c8->pc += 2;
        return 0;
    }
    /* FX18 - Sets the sound timer to VX. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 1, 8) {
        c8->sound_timer = c8->reg[hex[1]];
        c8->pc += 2;
        return 0;
    }
    /* FX1E - Adds VX to I. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 1, 0xE) {
        c8->addr_reg += c8->reg[hex[1]];
        c8->pc += 2;
        return 0;
    }
    /* FX29 - set I to location for sprite for character in VX */
    CHIP8_MATCH_OP(hex, 0xF, -1, 2, 9) {
        if (c8->reg[hex[1]] <= 0xF) {
            c8->addr_reg = 5 * c8->reg[hex[1]];
        } else {
            printf("unknown sprite\n");
        }
        c8->pc += 2;
        return 0;
    }
    /* FX33 - Stores the binary-coded decimal representation of VX */
    CHIP8_MATCH_OP(hex, 0xF, -1, 3, 3) {
        c8->mem[c8->addr_reg] = c8->reg[hex[1]] / 100;
        c8->mem[c8->addr_reg + 1] = (c8->reg[hex[1]] / 10) % 10;
        c8->mem[c8->addr_reg + 2] = c8->reg[hex[1]] % 10;
        c8->pc += 2;
        return 0;
    }
    /* FX55 - Stores V0 to VX (including VX) in memory starting at address I. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 5, 5) {
        /* for (int i = 0; i <= hex[1]; i++) { */
        /*     c8->mem[c8->addr_reg + i] = c8->reg[i]; */
        /* } */
        memcpy(c8->mem + c8->addr_reg, c8->reg, hex[1]+1);
        c8->pc += 2;
        return 0;
    }
    /* FX65 - Fills V0 to VX (including VX) with values from memory starting at address I. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 6, 5) {
        /* for (int i = 0; i <= hex[1]; i++) { */
        /*     c8->reg[i] = c8->mem[c8->addr_reg + i]; */
        /* } */
        memcpy(c8->reg, c8->mem + c8->addr_reg, hex[1]+1);
        c8->pc += 2;
        return 0;
    }

    return OP_UNKNOWN;
}
int load_rom(uint8_t *buf, FILE *rom)
{
    size_t n = 0;
    while (fread(buf + n, 1, 1, rom)) {
        n++;
    }
    return n;
}

enum OpType fetch_and_run(struct Chip8State *c8)
{
    uint16_t op = (c8->mem[c8->pc] << 8) + c8->mem[c8->pc+1];

#ifdef DEBUG
    print_opcode(op);
#endif

    int res = run_opcode(c8, op);
    if (res == OP_UNKNOWN) {
        printf("unrecognized opcode: %04x\n", op);
    }
    return res;
}

int chip8state_init(struct Chip8State **c8, char *rom)
{
    srand(808);
    FILE *f = fopen(rom, "r");
    if (f == NULL) {
        fprintf(stderr, "Could not open file: %s\n", rom);
        return 1;
    }

    uint8_t rombuf[CHIP8_MAX_ROM_SIZE];
    int romsize = load_rom(rombuf, f);
    fclose(f);
    if (romsize < 0) {
        fprintf(stderr, "Error loading rom: %s\n", rom);
        return 1;
    }

    *c8 = chip8state_create(rombuf, romsize);
    return 0;
}
