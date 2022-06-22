# Assume this only works on windows.

CC=gcc
CFLAGS=-Wall -std=c99
CLIBS=`pkg-config --libs --cflags sdl2`
SOURCE_FILE_MODULES= main.c

.PHONY: all clean
all: game.exe game-debug.exe

game.exe: $(wildcard *.c *.h)
	$(CC) $(SOURCE_FILE_MODULES) -o $@ $(CFLAGS) $(CLIBS) -O2
game-debug.exe: $(wildcard *.c *.h)
	$(CC) $(SOURCE_FILE_MODULES) -o $@ $(CFLAGS) $(CLIBS) -ggdb3
run: game.exe
	./game.exe
run-debug: game-debug.exe
	./game-debug.exe
clean:
	@rm game.exe
	@rm game-debug.exe
