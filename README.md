# Legends-JRPG
Tactics JRPG in the late SNES style, "Nothing but my blade and shame."

**I admit the code quality is not as good as it can be, however I am focusing on completing the game as a whole, and that is a sacrifice I am making.**

This game is made in C99, and as of now it's not complete, and is in active development.

It is intended to be a fully fledged JRPG with a few acts (or one large act), and is fully self-developed with all the intention being that all art (exempting the font), and other assets are created by me.

It's a **software rendered engine** working within a **mainly fixed memory footprint** and has
a **custom lisp based scripting language**. The engine is mostly independent and uses
a few libraries (SDL, stb are the only notable dependencies), with the vast majority
of the codebase being independent from any platform.

The engine uses bloom as a postprocessing effect through a basic box-filter, and due to
how expensive this effect is to compute on a CPU, the software renderer is **multithreaded** with the
help of a **custom job queue**.

The game also has built-in development tools such as a **level editor** and a **world map editor**.

It's heavily inspired by games of the late SNES era, and some tactics RPGs. It aims to
be essentially inspired by a game of the era, but is distinctly not of the era.

As I'm a college student and a solo developer it will presumably take a long time before any game comes out,
and most definitely might suffer lots of cutting room floor due to time constraints (of not wanting to work on one project
forever.)

## Latest Screenshot
![Latest screenshot](./scr.png)
