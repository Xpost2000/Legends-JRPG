@echo off

@rem Grr need better emscripten setup
@rem anyways here's the one liner to compile the whole game
@rem cause I use unity builds

@rem edit this line for emscripten path
echo This should be done on Windows cmd.exe
echo F:/emsdk/emsdk activate latest use this first
emcc main.c -O2 -dRELEASE -dUSE_EDITOR -lSDL2_mixer -lSDL2 -s USE_SDL_MIXER=2 -s USE_SDL=2 -s USE_WEBGL2=1 -I./dependencies/ -o game.html -s INITIAL_MEMORY=256MB --preload-file res --preload-file areas --preload-file shops --preload-file dlg --preload-file scenes
