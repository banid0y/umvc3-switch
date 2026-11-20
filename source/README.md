# Installer source

This folder holds the complete source of `Install.exe`. Nothing in the installer is hidden: what it does is
what these files do. Comments and messages in the code are in Portuguese (the UI strings are in both languages).

## What the installer does, step by step

1. Checks the SHA-256 of your `umvc3.exe` (only the supported Steam build goes through).
2. Offers to download Steamless 3.1.0.5 from its official GitHub release, checks the zip's SHA-256 and runs it on a
   **copy** of the executable. This is the only network access, and it asks first. If a `steamless` folder sits next
   to the installer, or the game folder already has `umvc3.exe.unpacked.exe`, nothing is downloaded.
3. Extracts the game's Direct3D 9 shader bytecode from your own files, translates it to GLSL (`sm3_glsl.c`),
   compiles it with devkitPro's `uam`, and checks every compiled shader against a table of expected hashes
   (`dksh_esperado.bin`) before packing `dksh.pak`.
4. If you chose the Community Edition: checks the four plugins by SHA-256 and relocates them to the addresses the
   port expects.
5. Copies the game data, the Switch button icons, a default `config.ini` and the `.nro` files to the destination.

It writes only to the destination you pick and to a temporary folder (`%TEMP%\umvc3-instalador-<pid>`, removed on
exit). It does not ask for administrator rights, does not touch the registry and never changes the game folder.

## Files

| File | What it is |
|---|---|
| `installer/instalador.c` | Window, Steamless step, install flow, command-line mode |
| `installer/preparador.c` | Engine: SHA-256, inflate, shader extraction/compilation/packing, CE plugins, copies |
| `installer/sm3_glsl.c`, `.h` | Direct3D 9 (SM3) bytecode to GLSL translator |
| `installer/instalador.rc`, `.manifest` | Embedded resources (icon, version info, `uam.exe`, button icons, default config, hash table) |
| `installer/dksh_esperado.bin` | Expected size and FNV-1a hash of each compiled shader (hashes only) |
| `installer/sdcard/`, `config.ini`, `instalador.ico`, `ferramentas/LICENCA-uam.txt` | The resources the `.rc` embeds |
| `installer/build.sh` | Build script |
| `tools/` | Python scripts that generated the hash table, the `.ico` and the d-pad texture patch |

The only embedded file not stored here is `uam.exe`, the deko3d shader compiler. It comes from
[devkitPro](https://devkitpro.org/) (package `uam`, `tools/bin/uam.exe`); `build.sh` copies it from there.

## Building

Needs a MinGW-w64 toolchain (for example [llvm-mingw](https://github.com/mstorsjo/llvm-mingw)) on `PATH` and `uam.exe`.

```bash
cd source/installer
UAM=/c/devkitPro/msys64/opt/devkitpro/tools/bin/uam.exe bash build.sh
```

The result is `Install.exe` in the same folder.

## Command-line mode

The installer also runs without the window:

```
Install.exe --sim --jogo "GAME FOLDER" --saida "DESTINATION" [--ce "CE FOLDER"] [--exe "UNPACKED EXE"] [--sem-dados] [--lang en|pt]
```

`--sem-dados` skips the big data copy (builds only the shaders, the executable and the small files).

## Linux

There is no native Linux build yet: the code uses the Win32 API directly. Running `Install.exe` under Wine, with
the command-line mode above, has not been tested. The steps above describe everything the installer produces, so
the process can also be reproduced by a script (devkitPro ships `uam` for Linux too).
