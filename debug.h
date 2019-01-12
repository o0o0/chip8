#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include "chip8.h"

void print_opcode(uint16_t);
void print_keyboard(struct Chip8State *c8);
void print_screen(struct Chip8State *c8);
void print_stack(struct Chip8State *c8);
void print_registers(struct Chip8State *c8);

#endif
