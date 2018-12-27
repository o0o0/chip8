CC=clang
CFLAGS=-Wall
LDFLAGS=-I./include -lsdl2

chip8: chip8.c
	$(CC) $(CFLAGS) $(LDFLAGS) chip8.c -o chip8
