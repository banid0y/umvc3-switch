# Código-fonte do instalador / Installer source

- `instalador/instalador.c`: a janela, o passo do Steamless e o fluxo da instalação.
- `instalador/preparador.c`: o motor (conferência do executável, extração e compilação dos shaders, correção das
  palavras de escalonamento, pacote `dksh.pak`, plugins da CE, cópias).
- `instalador/sm3_glsl.c/.h`: tradutor de bytecode Direct3D 9 (SM3) para GLSL.
- `instalador/instalador.rc`, `instalador.manifest`: recursos embutidos (ícone, versão, `uam.exe`, ícones de botão,
  configuração padrão, tabela de conferência `dksh_esperado.bin`).
- `ferramentas/`: scripts Python que geram a tabela de conferência, o `.ico` e a textura do direcional.

Compilar (llvm-mingw): `windres instalador.rc -O coff -o instalador.res` e
`x86_64-w64-mingw32-gcc -O2 -municode -mwindows -static instalador.c sm3_glsl.c instalador.res -lshell32 -lole32 -lurlmon -lcomctl32 -lgdi32 -o Install.exe`,
numa pasta com `ferramentas/uam.exe`, `ferramentas/LICENCA-uam.txt`, `sdcard/umvc3/texturas/`, `config.ini`,
`instalador.ico` e `dksh_esperado.bin` (ver os caminhos em `instalador.rc`).
