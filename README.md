Zombies Are Coming
===

Zombies Are Coming is a zombie survival FPS made for the 7DFPS 2012 challenge.

Building
===

Install dependencies:

- CMake
- SDL
- GL
- GLEW
- IL
- ILU
- ILUT
- OpenAL
- alut
- chipmunk

Installing dependencies on Debian/Ubuntu:

$ apt-get install build-essential cmake libsdl1.2-dev libgl1-mesa-dev libglew-dev libdevil-dev libopenal-dev libalut-dev chipmunk-dev

Build:

$ cmake .
$ make

Run:

$ ./zac

Building for Windows:
===

First you will have to acquire all dependency binaries.

Install mingw32 cross-compiler:

$ apt-get install mingw32

Build:

$ cmake -DCMAKE_TOOLCHAIN_FILE=Toolchain-win32.cmake .
$ make

Playing
===

Controls:

- WASD: move
- E: loot bodies and containers/barricade doors
- Space: pull doors
- 1-3: select weapon
- ESC: pause

Explore the buildings for map and ammo.







