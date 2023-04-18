# Legends-JRPG
Tactics JRPG in the late SNES style, "Nothing but my blade and shame."

**I admit the code quality is not as good as it can be, however I am proud of the technical aspects, and besides that I am focused on game completion. Code quality was a tradeoff I was willing to make so that I would not spend forever on this!**

## Feature List Summary
This lists many engine features, and is not an exhaustive list.

- Pure **C99** Codebase
- **Software Rendered** optimized with **SIMD** (SSE2 only) and a **multithreaded job queue**
- **Bloom Postprocessing** done through a box filter blur and alpha blending
- **Custom scripting language implementation**
- **Breadth-first-search** based pathfinding for combat falling back to **Bresenham** line tracing if trivial
- Custom **backwards compatible binary formats**, and a basic tape archive implementation
- **Turn Based combat with turn order based on priority**
- **Level Editor**, **World Map Editor**, and **Development Scripting Console**
- **Persistent save state system** with **delta compression** to reduce memory usage
- 2.5D World Map done through perspective transform
- UI with **controller support** and **mouse and keyboard support**
- **Extensively animated UI** for a polished UX experience
- Inventory and Shop system with **item category filtering**
- **Multiple party members** that can participate in combat and flock towards the player in the overworld
- **Rebindable controls** through a configuration file with **controller** and **mouse support**
- Minimal dependencies (only SDL2 and SDL2_mixer, and stb)
- **Heavily scriptable and customizable** dialogue system allowing for arbitrary game script to be executed, and markup
- Custom **cutscene system** through the scripting system
- Targets multiple platforms, and **confirmed working** on Windows and HTML5

## Technical Description

This game is made in C99, and as of now it's not complete, and is in active development. 

It is intended to be a fully fledged JRPG with a few acts (or one large act), and is fully self-developed with all the intention being that all art (exempting the font), and other assets are created by me.

It's a **software rendered engine** working within a **mainly fixed memory footprint** and has
a **custom lisp based scripting language**. The engine is mostly independent and uses
a few libraries (SDL, stb are the only notable dependencies), with the vast majority
of the codebase being independent from any platform.

The engine uses bloom as a postprocessing effect through a basic box-filter, and due to
how expensive this effect is to compute on a CPU, the software renderer is **multithreaded** with the
help of a **custom job queue**.

The game also has built-in development tools such as a **level editor** and a **world map editor**, which output
custom binary files that the game can quickly serialize. These formats are also backwards compatible, for any version
of the engine. The editors are designed to be somewhat comfortable to paint-in and utilize the mouse extensively, and simple menus through context keys (TAB, CTRL, and SHIFT).

The game also features a **save system**, which performs delta compression over the game state. The engine will only save
any deviations from a "source" state. This minimizes the amount of storage space a single savefile will require. The engine
supports flags to save a subset of an object's fields if necessary, however this is not used currently.

**Save files are also backwards compatible.**

It's heavily inspired by games of the late SNES era, and some tactics RPGs. It aims to
be a "modernized" version of a game of the time.

As it is ultimately still a "modern" game it features modern amenities such as **arbitrary joystick/gamepad support** and
supports **rebindable controls**.

Also unlike many games of its genre (such as any RPG Maker game), it is **resolution independent** and not restricted to any aspect ratio. The UI is confirmed for functionality and usability on other aspect ratios and resolution scales. However it is noted that the game is _best experienced with a 4:3 aspect ratio!_

As I'm a college student and a solo developer it will presumably take a long time before any game comes out,
and most definitely might suffer lots of cutting room floor due to time constraints (of not wanting to work on one project
forever.)

## Media
![Latest screenshot](./scr.png)

## Development / Compiling

### Requirements / Dependencies
- SDL2, SDL2_Mixer: These are the "platform" layer libraries, and are not bundled in the repository.
- A _Standard C Linux development environment_: On Windows you should use MinGW-w64 or MSYS2.
- **For Web Deployment**: _Emscripten_ is required to build WebASM targets for this game.
- **For Itch.IO Deployment**: _Butler_ is required to deploy to itch.io... Which should be impossible unless you are me or I got hacked.

These requirements should be quick to setup, or basically on any Linux system by default as I intended this project to be minimal in terms of
setup. This is so that I could quickly move to another machine and get straight to work without having to wait for setup time.

**Assets are currently not here! They are private for now, and I may link it to this as a submodule at some point.**

### Makefile Targets
You should check the makefile specifically for targets, however here are the most common targets for building.
```
all
  Performs all of the below.

clean
  Deletes all build/auxiliary files.

package-all-builds
  This is used to compile every target, and upload to itch.io. Very convenient for rapid deployment.

package-web
  This will build and upload a WebASM build to itch.io. This requires Emscripten.

package-windows-build
  This is specifically only used to build and upload a release Windows build to itch.io.

build-run-tree 
  This creates the 'run-tree' which is the intended directory structure used for distribution. It will build
  the game in release mode, and also build the custom bigfile archiver tool.
  
  It will then archive all the game assets into a single bigfile so as to be loaded by the game's virtual file system
  implementation.
  
run / run-debug
  This will compile, and run the game from the source directory. This is for testing release performance/behavior.
  It is intended to be used from within an IDE style environment.

  The debug variant will do the same, but with debug symbols.
  
run-from-runtree / run-debug-from-runtree
  This will compile, and run the game from the 'run-tree'. This is useful for confirming the archive system for the game
  works as intended, as this is what is being released.
  
web-experimental
  This is the web version of the engine. Technically the web version is always experimental, as it is unknown how stable
  this engine would run on the web. It works fine enough that I'm comfortable with a demo.
```

## Playing/Running

When I am comfortable with releasing a content demo the game will be avaliable through _releases_, and on itch.io.
