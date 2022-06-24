# Assume this only works on windows.
# TODO make this use the SDL distribution inside of the repository.

CC=gcc
CFLAGS=-Wall -std=c99
CLIBS=`pkg-config --libs --cflags sdl2` -I./dependencies/
SOURCE_FILE_MODULES= main.c

.PHONY: all clean gen-asm
all: game.exe game-debug.exe

game.exe: $(wildcard *.c *.h)
	$(CC) $(SOURCE_FILE_MODULES) -o $@ $(CFLAGS) $(CLIBS) -O2
game-debug.exe: $(wildcard *.c *.h)
	$(CC) $(SOURCE_FILE_MODULES) -o $@ $(CFLAGS) $(CLIBS) -ggdb3
run: game.exe
	./game.exe
run-debug: game-debug.exe
	./game-debug.exe
gen-asm-debug: $(wildcard *.c *.h)
	-mkdir asm
	$(CC) $(SOURCE_FILE_MODULES) $(CFLAGS) $(CLIBS) -S -masm=intel -ggdb3 -fverbose-asm -o asm/debug.s
gen-asm: $(wildcard *.c *.h)
	-mkdir asm
	$(CC) $(SOURCE_FILE_MODULES) $(CFLAGS) $(CLIBS) -S -masm=intel -O2 -fverbose-asm -o asm/release.s
clean:
	-rm game.exe
	-rm game-debug.exe
