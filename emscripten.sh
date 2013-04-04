#!/bin/bash
EMSCRIPTEN=~/src/emscripten/
$EMSCRIPTEN/em++ -DNDEBUG --minify 1 -O2 \
  -s DEAD_FUNCTIONS="['_gethostbyaddr','_SDL_SemPost','_SDL_SemTryWait','_SDL_SemWait','_SDL_CreateSemaphore']" \
  -s ASM_JS=1 \
  -s TOTAL_MEMORY=67108864 \
  -Wall -std=c++11 -I./enet/include/ emscripten_blob.cpp ./lib/libz.bc \
  -o $1.html \
--preload-file "data/" \
--preload-file "packages/" \
--preload-file "config.cfg" \
--preload-file "autoexec.cfg"

