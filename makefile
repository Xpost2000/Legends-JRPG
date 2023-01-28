# Assume this only works on windows.
# TODO make this use the SDL distribution inside of the repository.

CC=gcc
CFLAGS=-Werror -Wno-unused -Wno-unused-but-set-variable -Wall -std=c99
CLIBS=-Dmain=SDL_main -lmingw32 -L./dependencies/x86-64/lib/ -L./dependencies/x86-64/bin/ -I./dependencies/ -I./dependencies/x86-64/include/ -lOpenGL32 -lglew32 -lSDL2main -lSDL2 -lSDL2_mixer -msse4
ITCHPROJECT=xpost2000/untitled-project

SOURCE_FILE_MODULES= main.c
EMCC=emcc

.PHONY: package-all-builds package-windows-build all build-run-tree clean gen-asm gen-asm-debug cloc run run-debug run-from-runtree run-debug-from-runtree package-web web-experimental
all: package-all-builds build-run-tree packer.exe depack.exe game.exe game-debug.exe

data.bigfile: pack.exe
	./pack $@ worldmaps areas dlg res scenes shops
pack.exe: bigfilemaker/bigfile_packer.c bigfilemaker/bigfile_def.c
	$(CC) bigfilemaker/bigfile_packer.c -o $@ -O2
gamescript_metagen.exe: gamescript_metagen.c
	$(CC) gamescript_metagen.c -g -w -o $@
depack.exe: bigfilemaker/depacker.c bigfilemaker/bigfile_unpacker.c bigfilemaker/bigfile_def.c
	$(CC) bigfilemaker/depacker.c -o $@ -O2
game.exe: clean data.bigfile gamescript_metagen.exe $(wildcard *.c *.h)
	./gamescript_metagen.exe
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR  -DRELEASE -o $@ $(CFLAGS) $(CLIBS) -m64 -O2 -mwindows
game-debug.exe: gamescript_metagen.exe $(wildcard *.c *.h)
	./gamescript_metagen.exe
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR -o $@ $(CFLAGS) $(CLIBS) -m64 -ggdb3
gamex86.exe: metagen.exe $(wildcard *.c *.h)
	./gamescript_metagen.exe
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR  -DRELEASE -o $@ $(CFLAGS) $(CLIBS) -m32 -O2 -mwindows
gamex86-debug.exe: ./gamescript_metagen.exe $(wildcard *.c *.h)
	./gamescript_metagen.exe
	$(CC) $(SOURCE_FILE_MODULES) -DUSE_EDITOR -o $@ $(CFLAGS) $(CLIBS) -m32 -ggdb3
web-build/index.html: ./gamescript_metagen.exe $(wildcard *.c *.h) shell_minimal.html
	./gamescript_metagen.exe
	-mkdir web-build/
	$(EMCC) main.c -O2 -lSDL2_mixer -lSDL2 -s USE_SDL_MIXER=2 -s USE_SDL=2 -s USE_WEBGL2=1 -I./dependencies/ --shell-file shell_minimal.html  -o web-build/game.html -s INITIAL_MEMORY=128MB --preload-file res --preload-file areas --preload-file shops --preload-file dlg --preload-file scenes --preload-file worldmaps
	mv web-build/game.html web-build/index.html
package-web: web-build/index.html
	cd web-build && zip gamewebpackage.zip index.html game.data game.js game.wasm && butler push gamewebpackage.zip $(ITCHPROJECT):wasm
build-run-tree: clean game-debug.exe game.exe data.bigfile
	-mkdir run-tree
	cp game-debug.exe game.exe data.bigfile run-tree/
web-experimental: game.html
run: game.exe
	./game.exe
run-debug: game-debug.exe
	./game-debug.exe
run-from-runtree: build-run-tree
	./run-tree/game.exe
run-debug-from-runtree: build-run-tree
	./run-tree/game-debug.exe
package-windows-build: build-run-tree
	butler push run-tree $(ITCHPROJECT):windows-64bit
gen-asm-debug: $(wildcard *.c *.h)
	-mkdir asm
	$(CC) $(SOURCE_FILE_MODULES) $(CFLAGS) $(CLIBS) -S -masm=intel -ggdb3 -fverbose-asm -o asm/debug.s
gen-asm: $(wildcard *.c *.h)
	-mkdir asm
	$(CC) $(SOURCE_FILE_MODULES) $(CFLAGS) $(CLIBS) -S -masm=intel -O2 -fverbose-asm -o asm/release.s
cloc:
	cloc --by-file *.c bigfilemaker/*.c
clean:
	-rm data.bigfile
	-rm game.exe
	-rm gamescript_metagen.exe
	-rm game.js
	-rm game.html
	-rm game.wasm
	-rm game.data
	-rm game-debug.exe
	-rm pack.exe
	-rm run-tree/*
	-rm web-build/*
	-rm depack.exe
package-all-builds: package-windows-build package-web
