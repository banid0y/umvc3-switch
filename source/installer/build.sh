#!/bin/bash
# Builds Install.exe from this folder. Needs a MinGW-w64 toolchain on PATH (windres + gcc, e.g. llvm-mingw)
# and devkitPro's uam.exe:  UAM=/path/to/uam.exe bash build.sh
set -e
cd "$(dirname "$0")"
UAM=${UAM:-/c/devkitPro/msys64/opt/devkitpro/tools/bin/uam.exe}
[ -f "$UAM" ] || { echo "uam.exe not found: set UAM=/path/to/uam.exe (devkitPro package 'uam')"; exit 1; }
cp "$UAM" ferramentas/uam.exe
x86_64-w64-mingw32-windres instalador.rc -O coff -o instalador.res
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -municode -mwindows -static instalador.c sm3_glsl.c instalador.res \
    -lshell32 -lole32 -lurlmon -lcomctl32 -lgdi32 -o Install.exe
rm -f instalador.res ferramentas/uam.exe
ls -la Install.exe
