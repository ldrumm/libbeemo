libbeemo
========

procedural realtime audio for games

[![Build Status](https://travis-ci.org/ldrumm/libbeemo.svg?branch=master)](https://travis-ci.org/ldrumm/libbeemo)

libbeemo is intended as a way to bring the game's environment to the composer, by providing a powerful graph-based DSP interface with nodes programmable in Lua.
Each programmable DSP node can be given access to in-game variables such as health, progress, number of enemies, etc so that these values may inform the production or processing of sound.

There are many features missing (time makes fools of us all), but the library is usable at the moment.  The API is in a constant state of flux, though, so will break until things mature.

Build Instructions
==================

### Hard Build Dependencies
    - A C99 compiler (GCC, or clang is recommended)
    - liblua5.2
    - SCons (which in-turn requires Python2.7)

For Debian-based systems like Ubuntu, the following should get you everything you need to build:

    sudo apt-get install build-essential scons valgrind portaudio19-dev libjack-dev ladspa-sdk liblua5.2-dev libsndfile1-dev lua5.2

Then run `scons tests` from the root directory to build the shared library, static library, and build and run the tests.

For comprehensive build instructions (including Win32 build instructions), see `docs/building.md`

Examples
========
Take a look at the code in `tests/` and in particular `tests/interactive`
