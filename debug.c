#include <stdio.h>
/* #include "chip8.h" */
/* #include "sdlctx.h" */
#include "debug.h"
struct Chip8State;


void print_registers(struct Chip8State *c8)
{
    printf("PC = %04X\tI = %04X\tDEL = %02X\tSND = %02X\n",
           c8->pc, c8->addr_reg, c8->delay_timer, c8->sound_timer);
    for (int i = 0; i < 16; i += 4) {
        printf("V%X = %02X  \t", i, c8->reg[i]);
        printf("V%X = %02X  \t", i+1, c8->reg[i+1]);
        printf("V%X = %02X  \t", i+2, c8->reg[i+2]);
        printf("V%X = %02X\n", i+3, c8->reg[i+3]);
    }
}

void print_stack(struct Chip8State *c8)
{
    printf("Stack: ");
    for (int i = 0; i <= c8->stack_ptr; i++) {
        printf("%02X ", c8->stack[i]);
    }
    printf("\n");
}

void print_screen(struct Chip8State *c8)
{
    for (int y = 0; y < CHIP8_HEIGHT; y++) {
        for (int x = 0; x < CHIP8_WIDTH; x++) {
            if (c8->screen[x][y]) {
                printf("#");
            } else {
                printf(".");
            }
        }
        printf("\n");
    }
}

void print_keyboard(struct Chip8State *c8)
{
    printf("Keys: ");
    for (int k = 0; k < 16; k++) {
        /* if (c8->keyboard[keymap[k]]) { */
        if (c8->key_pressed(k)) {
            printf("1");
        } else {
            printf("0");
        }
    }
    printf("\n");
}

void print_state(struct Chip8State *c8)
{
    print_registers(c8);
    print_stack(c8);
    print_keyboard(c8);
    printf("\n");
}

void print_opcode(uint16_t op)
{
    uint8_t hex[4];             /* opcode hex digits, most significant first */
    hex[0] = (op >> 12) & 0xF;
    hex[1] = (op >> 8) & 0xF;
    hex[2] = (op >> 4) & 0xF;
    hex[3] = op & 0xF;
    uint16_t h22 = DIGITS2HEX2(hex[2], hex[3]);
    uint16_t h3 = DIGITS2HEX3(hex[1], hex[2], hex[3]);

    printf("%04X : ", op);
    /* 00EE - return */
    CHIP8_MATCH_OP(hex, 0, 0, 0xE, 0xE) {
        printf("RETURN\n");
        return;
    }
    /* 00E0 - clear display */
    CHIP8_MATCH_OP(hex, 0, 0, 0xE, 0) {
        printf("CLEAR DISPLAY\n");
        return;
    }
    /* 0NNN - call program at address NNN */
    CHIP8_MATCH_OP(hex, 0, -1, -1, -1) {
        /* ignored */
        printf("CALL PROGRAM @ %03X\n", h3);
        return;
    }
    /* 1NNN - goto NNN */
    CHIP8_MATCH_OP(hex, 1, -1, -1, -1) {
        printf("GOTO %03X\n", h3);
        return;
    }
    /* 2NNN - call subroutine at NNN */
    CHIP8_MATCH_OP(hex, 2, -1, -1, -1) {
        printf("SUBCALL %03X\n", h3);
        return;
    }
    /* 3XNN - skip next if VX == NN */
    CHIP8_MATCH_OP(hex, 3, -1, -1, -1) {
        printf("SKIPIF V%X == %02X\n", hex[1], h22);
        return;
    }
    /* 4XNN - skip next if VX != NN */
    CHIP8_MATCH_OP(hex, 4, -1, -1, -1) {
        printf("SKIPIF V%X != %02X\n", hex[1], h22);
        return;
    }
    /* 5XY0 - skip next if VX == VY */
    CHIP8_MATCH_OP(hex, 5, -1, -1, -1) {
        printf("SKIPIF V%X == V%X\n", hex[1], hex[2]);
        return;
    }
    /* 6XNN - set VX to NN */
    CHIP8_MATCH_OP(hex, 6, -1, -1, -1) {
        printf("SET V%X = %02X\n", hex[1], h22);
        return;
    }
    /* 7XNN - VX += NN, carry flag not changed */
    CHIP8_MATCH_OP(hex, 7, -1, -1, -1) {
        printf("V%X += %02X\n", hex[1], h22);
        return;
    }
    /* 8XY0  - VX = VY */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 0) {
        printf("SET V%X = V%X\n", hex[1], hex[2]);
        return;
    }
    /* 8XY1 - VX = VX | VY */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 1) {
        printf("SET V%X = V%X | V%X\n", hex[1], hex[1], hex[2]);
        return;
    }
    /* 8XY2 - VX = VX & VY */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 2) {
        printf("SET V%X = V%X & V%X\n", hex[1], hex[1], hex[2]);
        return;
    }
    /* 8XY3 - VX = VX ^ VY */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 3) {
        printf("SET V%X = V%X ^ V%X\n", hex[1], hex[1], hex[2]);
        return;
    }
    /* 8XY4 - VX += VY, set carry flag */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 4) {
        printf("SET V%X += V%X  (SET CARRY FLAG)\n", hex[1], hex[2]);
        return;
    }
    /* 8XY5 - VX -= VY, set carry flag */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 5) {
        printf("SET V%X -= V%X  (SET CARRY FLAG)\n", hex[1], hex[2]);
        return;
    }
    /* 8XY6 - store least significant bit in of VX in VF, then VX >>= 1,  */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 6) {
        printf("STORE LEAST SIG BIT OF V%X IN VF, SET V%X >>= 1\n", hex[1], hex[1]);
        return;
    }
    /* 8XY7 - VX = VY-VX, set VF to 0 if there is a borrow, 1 if not */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 7) {
        printf("SET V%X = V%X - V%X,  SET VF TO 0 IF BORROW ELSE 1\n", hex[1], hex[2], hex[1]);
        return;
    }
    /* 8XYE - store most significant bit of VX in VF, then VX <<= 1 */
    CHIP8_MATCH_OP(hex, 8, -1, -1, 0xE) {
        printf("STORE MOST SIG BIT OF V%X IN VF, SET V%X <<= 1\n", hex[1], hex[1]);
        return;
    }
    /* 9XY0 - skip next if VX != VY */
    CHIP8_MATCH_OP(hex, 9, -1, -1, 0) {
        printf("SKIP NEXT IF V%X != V%X\n", hex[1], hex[2]);
        return;
    }
    /* ANNN - Sets I to the address NNN. */
    CHIP8_MATCH_OP(hex, 0xA, -1, -1, -1) {
        printf("SET I TO ADDR %03X\n", h3);
        return;
    }
    /* BNNN - Jumps to the address NNN plus V0. */
    CHIP8_MATCH_OP(hex, 0xB, -1, -1, -1) {
        printf("JUMP TO V0 + %03X\n", h3);
        return;
    }
    /* CXNN - VX = rand() & NN */
    CHIP8_MATCH_OP(hex, 0xC, -1, -1, -1) {
        printf("SET V%X = RAND() & %02X\n", hex[1], h22);
        return;
    }
    /* DXYN - draw sprite at (VX, VY), width of 8 pixels and height of N pixels */
    /* Each row of 8 pixels is read starting from memory location I; */
    /* VF is set to 1 if any screen pixels are flipped from set to unset, 0 otherwise */
    CHIP8_MATCH_OP(hex, 0xD, -1, -1, -1) {
        printf("DRAW SPRITE AT (V%X, V%X) HEIGHT %X\n", hex[1], hex[2], hex[3]);
        return;
    }
    /* EX9E - skip next if key stored in VX is pressed */
    CHIP8_MATCH_OP(hex, 0xE, -1, 9, 0xE) {
        printf("SKIP NEXT IF KEY IN V%X PRESSED\n", hex[1]);
        return;
    }
    /* EXA1 - skip next if key stored in VX is not pressed */
    CHIP8_MATCH_OP(hex, 0xE, -1, 0xA, 1) {
        printf("SKIP NEXT IF KEY IN V%X NOT PRESSED\n", hex[1]);
        return;
    }
    /* FX07 - Sets VX to the value of the delay timer. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 0, 7) {
        printf("SET V%X TO DELAY TIMER\n", hex[1]);
        return;
    }
    /* FX0A - wait for a key press (blocking) and store it in VX */
    CHIP8_MATCH_OP(hex, 0xF, -1, 0, 0xA) {
        printf("WAIT FOR KEY, STORE IN V%X\n", hex[1]);
        return;
    }
    /* FX15 - Sets the delay timer to VX.  */
    CHIP8_MATCH_OP(hex, 0xF, -1, 1, 5) {
        printf("SET DELAY TIMER TO V%X\n", hex[1]);
        return;
    }
    /* FX18 - Sets the sound timer to VX. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 1, 8) {
        printf("SET SOUND TIMER TO V%X\n", hex[1]);
        return;
    }
    /* FX1E - Adds VX to I. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 1, 0xE) {
        printf("I += V%X\n", hex[1]);
        return;
    }
    /* FX29 - set I to location for sprite for character in VX */
    CHIP8_MATCH_OP(hex, 0xF, -1, 2, 9) {
        printf("SET I TO SPRITE ADDR IN V%X\n", hex[1]);
        return;
    }
    /* FX33 - Stores the binary-coded decimal representation of VX */
    CHIP8_MATCH_OP(hex, 0xF, -1, 3, 3) {
        printf("STORE BCD OF V%X AT I\n", hex[1]);
        return;
    }
    /* FX55 - Stores V0 to VX (including VX) in memory starting at address I. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 5, 5) {
        printf("STORE V0 TO V%X IN MEMORY AT I\n", hex[1]);
        return;
    }
    /* FX65 - Fills V0 to VX (including VX) with values from memory starting at address I. */
    CHIP8_MATCH_OP(hex, 0xF, -1, 6, 5) {
        printf("FILL V0 TO V%X FROM I\n", hex[1]);
        return;
    }
}
