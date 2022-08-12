# Assume this only works on windows.
# TODO make this use the SDL distribution inside of the repository.

CC=gcc
CFLAGS=-Werror -Wno-unused -Wno-format -Wno-unused-but-set-variable -Wall -std=c99
CLIBS=-Dmain=SDL_main -lmingw32 -L./dependencies/x86-64/lib/ -L./dependencies/x86-64/bin/ -I./dependencies/ -I./dependencies/x86-64/include/ -lSDL2main -lSDL2 -lsoloud_x64 -msse4

SOURCE_FILE_MODULES= main.c
EMCC=emcc

.PHONY: all clean gen-asm
all: game.exe game-debug.exe

game.exe: $(wildcard *.c *.h)
	$(CC) $(SOURCE_FILE_MODULES) -DRELEASE -o $@ $(CFLAGS) $(CLIBS) -O2 -mwindows
game-debug.exe: $(wildcard *.c *.h)
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR -o $@ $(CFLAGS) $(CLIBS) -ggdb3
web-experimental: $(wildcard *.c *.h)
	$(EMCC) $(SOURCE_FILE_MODULES) -DRELEASE -s USE_SDL=2 -s USE_WEBGL2=1 -o game.html $(CFLAGS) $(CLIBS) -s INITIAL_MEMORY=127MB --preload-file res --preload-file scenes --preload-file areas
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
