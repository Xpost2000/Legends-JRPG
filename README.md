# Legends-JRPG
Tactics JRPG in the late SNES style, "Nothing but my blade and shame."

This game is made in C99 sort of on and off, and as of now it's not complete.

Ideally I'm supposed to have a demo by the end of next month...

It's a software rendered engine working within a fixed memory footprint and has
a custom lisp based scripting language. The engine is mostly independent and uses
a few libraries (SDL, stb are the only notable dependencies), with the vast majority
of the codebase being independent from any platform.

As it's a software renderer with postprocessing it's kind of slow without multithreading
and so this engine is best run with a multi-core processor for playable performance at
higher resolutions (or even the base resolution.)

It's heavily inspired by games of the late SNES era, and some tactics RPGs. It aims to
be essentially inspired by a game of the era, but is distinctly not of the era.

## Latest Screenshot
![Latest screenshot](./scr.png)
"In development!"
