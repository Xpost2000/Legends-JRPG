# Assume this only works on windows.

CC=gcc
CFLAGS=-Wall
CLIBS=`pkg-config --libs --cflags sdl2`


.PHONY: all clean
all: game.exe game-debug.exe

game.exe: $(wildcard *.c *.h)
	$(CC) main.c -o $@ $(CFLAGS) $(CLIBS) -O2
game-debug.exe: $(wildcard *.c *.h)
	$(CC) main.c -o $@ $(CFLAGS) $(CLIBS) -ggdb3
clean:
	@rm game.exe
	@rm game-debug.exe
