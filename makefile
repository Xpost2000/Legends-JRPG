# Assume this only works on windows.
# TODO make this use the SDL distribution inside of the repository.

CC=gcc
CFLAGS=-Werror -Wno-unused -Wno-unused-but-set-variable -Wall -std=c99
CLIBS=-Dmain=SDL_main -lmingw32 -L./dependencies/x86-64/lib/ -L./dependencies/x86-64/bin/ -I./dependencies/ -I./dependencies/x86-64/include/ -lSDL2main -lSDL2 -lSDL2_mixer -msse4

SOURCE_FILE_MODULES= main.c
EMCC=emcc

.PHONY: all clean gen-asm gen-asm-debug cloc
all: packer.exe depack.exe game.exe game-debug.exe

data.bigfile: pack.exe
	./pack $@ areas dlg res scenes shops
pack.exe: bigfilemaker/bigfile_packer.c bigfilemaker/bigfile_def.c
	$(CC) bigfilemaker/bigfile_packer.c -o $@ -O2
metagen.exe: metagen.c
	$(CC) metagen.c -g -w -o $@
depack.exe: bigfilemaker/depacker.c bigfilemaker/bigfile_unpacker.c bigfilemaker/bigfile_def.c
	$(CC) bigfilemaker/depacker.c -o $@ -O2
game.exe: data.bigfile metagen.exe $(wildcard *.c *.h)
	./metagen.exe
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR  -DRELEASE -o $@ $(CFLAGS) $(CLIBS) -m64 -O2 -mwindows
game-debug.exe: data.bigfile metagen.exe $(wildcard *.c *.h)
	./metagen.exe
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR -o $@ $(CFLAGS) $(CLIBS) -m64 -ggdb3
gamex86.exe: metagen.exe $(wildcard *.c *.h)
	./metagen.exe
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR  -DRELEASE -o $@ $(CFLAGS) $(CLIBS) -m32 -O2 -mwindows
gamex86-debug.exe: metagen.exe $(wildcard *.c *.h)
	./metagen.exe
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR -o $@ $(CFLAGS) $(CLIBS) -m32 -ggdb3
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
cloc:
	cloc *.c bigfilemaker/*.c
clean:
	-rm data.bigfile
	-rm game.exe
	-rm game.js
	-rm game.html
	-rm game.wasm
	-rm game.data
	-rm game-debug.exe
