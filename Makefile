CC=clang
CFLAGS=-Wall
LDFLAGS=-I./include -lsdl2

chip8: main.c sdlctx.o debug.o chip8.o
	$(CC) $(CFLAGS) $(LDFLAGS) chip8.o debug.o sdlctx.o main.c -o chip8
# $(CC) $(CFLAGS) $(LDFLAGS) chip8.c -o chip8

sdlctx.o: sdlctx.h chip8.h

debug.o: debug.h chip8.h sdlctx.h

chip8.o: chip8.c chip8.h debug.h sdlctx.h
