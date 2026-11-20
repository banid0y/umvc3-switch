// Instalador do UMvC3 no Switch (Windows): um único .exe COM JANELA (sem janela de comando). Usa o motor do
// preparador.c (conferência do executável, shaders, plugins da CE, cópias) e acrescenta:
//  - a janela: pasta do jogo, Community Edition (opcional), destino, botão Instalar, etapa atual e barra de progresso;
//  - o passo do Steamless automático: baixa o Steamless do endereço oficial (ou usa a pasta steamless\ ao lado do
//    instalador), roda a linha de comando dele numa CÓPIA do umvc3.exe em pasta temporária e confere o resultado.
//    O Steamless não é distribuído nem modificado aqui (licença CC BY-NC-ND 4.0, atom0s); a pasta do jogo não é tocada;
//  - o compilador de shaders (uam), os ícones de botão e a configuração padrão embutidos no .exe (instalador.rc);
//  - os aplicativos: todo .nro da pasta "NRO FILES" ao lado do instalador vai para switch\ no destino;
//  - dois idiomas: abre em inglês; o seletor no canto da janela troca para português na hora; --lang pt|en força um.
//
//   Instalador.exe                       abre a janela
//   Instalador.exe --sim --jogo PASTA --saida PASTA [--ce PASTA] [--exe ARQUIVO] [--sem-dados] [--lang pt|en]
//                                        modo automático, sem janela (testes); o andamento sai na saída padrão
//
// Compilar: lab/instalador_build.sh (windres instalador.rc + gcc -municode -mwindows -static).
#define PREPARADOR_SEM_MAIN
#include "preparador.c"
#include <shlobj.h>
#include <urlmon.h>
#include <commctrl.h>

#define SL_URL L"https://github.com/atom0s/Steamless/releases/download/v3.1.0.5/Steamless.v3.1.0.5.-.by.atom0s.zip"
#define SL_ZIP_SHA "e3e2d22e098ff3fb359b2876aa2bed9596f0501e6ff588cbffae90a76d2dc4f5"
#define TITLE (g_en ? L"UMvC3 on Switch" : L"UMvC3 no Switch")
#define CE_VER "1.2.3b"   // versão da Community Edition suportada
#define STEAM_DEF L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\ULTIMATE MARVEL VS. CAPCOM 3"

// ---- recursos embutidos (instalador.rc) ----
static const void *res_get(int id, size_t *n)
{
    HRSRC r = FindResourceW(NULL, MAKEINTRESOURCEW(id), (LPCWSTR)RT_RCDATA); if (!r) return NULL;
    HGLOBAL g = LoadResource(NULL, r); if (!g) return NULL;
    *n = SizeofResource(NULL, r); return LockResource(g);
}
static const struct { int id; const wchar_t *path; int keep; } SDFILES[] = {
    { 110, L"umvc3\\texturas\\indice.txt", 0 }, { 111, L"umvc3\\texturas\\CREDITOS.txt", 0 },
    { 112, L"umvc3\\texturas\\9befc1f7fc3908ab.bin", 0 }, { 113, L"umvc3\\texturas\\af638d8670f646d5.bin", 0 },
    { 114, L"umvc3\\texturas\\c7987b2262934b67.bin", 0 }, { 115, L"umvc3\\texturas\\c50d09005c484509.bin", 0 },
    { 120, L"umvc3\\userdata\\user\\CAPCOM\\ULTIMATE MARVEL VS. CAPCOM 3\\config.ini", 1 },
    { 120, L"umvc3\\userdata-ce\\user\\CAPCOM\\ULTIMATE MARVEL VS. CAPCOM 3\\config.ini", 1 },   // keep = não troca se já existe
};

// ---- caixas de mensagem e escolha de pasta ----
static void wide(const char *utf8, wchar_t *w, int n) { MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w, n); }
static void box(const char *utf8, UINT flags) { wchar_t w[2000]; wide(utf8, w, 2000); MessageBoxW(g_owner, w, TITLE, flags | MB_SETFOREGROUND); }
static int box_yes(const char *utf8) { wchar_t w[2000]; wide(utf8, w, 2000); return MessageBoxW(g_owner, w, TITLE, MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND) == IDYES; }
static int pick_folder(const char *title_utf8, wchar_t *out)
{
    wchar_t t[600]; wide(title_utf8, t, 600);
    BROWSEINFOW bi; memset(&bi, 0, sizeof bi); bi.hwndOwner = g_owner; bi.lpszTitle = t; bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE id = SHBrowseForFolderW(&bi); if (!id) return 0;
    wchar_t p[MAX_PATH * 2]; int ok = SHGetPathFromIDListW(id, p); CoTaskMemFree(id); if (!ok) return 0;
    size_t l = wcslen(p); while (l > 2 && p[l - 1] == L'\\') p[--l] = 0;   // "E:\" vira "E:"
    wcscpy(out, p); return 1;
}

// ---- processos ----
static int run_wait(const wchar_t *cmdline, const wchar_t *cwd)
{
    wchar_t cmd[3000]; wcsncpy(cmd, cmdline, 2999); cmd[2999] = 0;
    STARTUPINFOW si; PROCESS_INFORMATION pi; memset(&si, 0, sizeof si); si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES; si.hStdInput = si.hStdOutput = si.hStdError = INVALID_HANDLE_VALUE;
    if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, cwd, &si, &pi)) return -1;
    WaitForSingleObject(pi.hProcess, INFINITE); DWORD ec = 1; GetExitCodeProcess(pi.hProcess, &ec);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return (int)ec;
}
static void rmtree(const wchar_t *dir)
{
    wchar_t pat[1024], a[1024]; join(pat, dir, L"*"); WIN32_FIND_DATAW fd; HANDLE h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do { if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
             join(a, dir, fd.cFileName);
             if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) rmtree(a); else { SetFileAttributesW(a, FILE_ATTRIBUTE_NORMAL); DeleteFileW(a); }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryW(dir);
}
// A pasta temporária da execução some também quando o instalador termina com erro (die() chama exit()).
static void cleanup(void) { if (g_tmp[0]) rmtree(g_tmp); }

// ---- Steamless automático ----
// Devolve em exe_out o umvc3.exe.unpacked.exe gerado numa pasta temporária. A pasta do jogo só é lida.
static void steamless_auto(const wchar_t *self, const wchar_t *game, wchar_t *exe_out)
{
    wchar_t cli[1024], dir[1024], zip[1024], work[1024], p[1024], cmd[3000]; char hex[80];
    join(dir, self, L"steamless"); join(cli, dir, L"Steamless.CLI.exe");
    if (!exists(cli)) {   // cópia baixada numa execução anterior
        wchar_t t[512]; GetTempPathW(512, t); swprintf(dir, 1024, L"%lsumvc3-instalador-steamless", t); join(cli, dir, L"Steamless.CLI.exe");
    }
    if (!exists(cli)) {
        if (!g_yes && !box_yes(T("O umvc3.exe da Steam vem com uma proteção (SteamStub) que o port não consegue ler.\n\n"
                                 "Posso baixar agora o Steamless (programa gratuito, de atom0s) da página oficial dele no GitHub e aplicá-lo "
                                 "numa cópia do seu umvc3.exe. A pasta do jogo não é alterada.\n\nBaixar e aplicar?",
                                 "The Steam umvc3.exe is wrapped in a protection layer (SteamStub) that the port cannot read.\n\n"
                                 "I can download Steamless (a free program by atom0s) from its official GitHub page now and apply it "
                                 "to a copy of your umvc3.exe. The game folder is not changed.\n\nDownload and apply?")))
            die(T("sem o Steamless o instalador não tem como continuar.\nSe preferir fazer à mão: rode o Steamless no umvc3.exe e rode o instalador de novo.",
                  "the installer cannot continue without Steamless.\nTo do it by hand: run Steamless on umvc3.exe and run the installer again."));
        say(T("  baixando o Steamless da página oficial...\n", "  downloading Steamless from its official page...\n"));
        mkdirs(dir); join(zip, dir, L"steamless.zip");
        if (URLDownloadToFileW(NULL, SL_URL, zip, 0, NULL) != S_OK)
            die(T("não consegui baixar o Steamless (sem internet?).\nBaixe-o à mão em github.com/atom0s/Steamless/releases, extraia numa pasta \"steamless\" ao lado do instalador e rode de novo.",
                  "could not download Steamless (no internet?).\nDownload it by hand from github.com/atom0s/Steamless/releases, extract it into a \"steamless\" folder next to the installer and run again."));
        sha_file(zip, hex);
        if (strcmp(hex, SL_ZIP_SHA)) { DeleteFileW(zip);
            die(T("o arquivo do Steamless baixado não é o esperado (o download falhou ou a versão mudou).\nBaixe a versão 3.1.0.5 à mão, extraia numa pasta \"steamless\" ao lado do instalador e rode de novo.",
                  "the downloaded Steamless file is not the expected one (the download failed or the version changed).\nDownload version 3.1.0.5 by hand, extract it into a \"steamless\" folder next to the installer and run again.")); }
        wchar_t sys[512]; GetSystemDirectoryW(sys, 512);
        swprintf(cmd, 3000, L"\"%ls\\tar.exe\" -xf \"%ls\" -C \"%ls\"", sys, zip, dir);
        if (run_wait(cmd, NULL) != 0 || !exists(cli)) {
            swprintf(cmd, 3000, L"powershell -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -Force -LiteralPath '%ls' -DestinationPath '%ls'\"", zip, dir);
            if (run_wait(cmd, NULL) != 0 || !exists(cli))
                die(T("não consegui extrair o Steamless.\nExtraia-o à mão numa pasta \"steamless\" ao lado do instalador e rode de novo.",
                      "could not extract Steamless.\nExtract it by hand into a \"steamless\" folder next to the installer and run again."));
        }
    }
    join(work, g_tmp, L"exe"); mkdirs(work);
    join(p, game, L"umvc3.exe"); join(exe_out, work, L"umvc3.exe");
    if (!CopyFileW(p, exe_out, FALSE)) die(T("não consegui copiar o umvc3.exe do jogo para a pasta temporária (erro %lu).", "could not copy the game's umvc3.exe to the temporary folder (error %lu)."), GetLastError());
    say(T("  tirando a proteção de uma cópia do umvc3.exe (Steamless)...\n", "  removing the protection from a copy of umvc3.exe (Steamless)...\n"));
    swprintf(cmd, 3000, L"\"%ls\" umvc3.exe", cli);
    int rc = run_wait(cmd, work);
    join(exe_out, work, L"umvc3.exe.unpacked.exe");
    if (!exists(exe_out)) die(T("o Steamless não gerou o executável sem proteção (código %d).\nEle precisa do .NET Framework 4.5.2 ou mais novo, que já vem no Windows 10 e 11.",
                                "Steamless did not produce the unprotected executable (code %d).\nIt needs .NET Framework 4.5.2 or newer, which ships with Windows 10 and 11."), rc);
}

// ---- a instalação (roda na thread de trabalho da janela, ou direto no modo automático) ----
static wchar_t g_self[1024], g_game[1024], g_ce[1024], g_out[1024], g_exe_arg[1024];
static int g_nodata, g_with_ce; static volatile int g_stage;   // 1..5 = etapa em curso, 9 = terminou

static void install_run(void)
{
    wchar_t p[1024], q[1024], exe[1024], um[1024]; char hex[80];
    const wchar_t *game = g_game, *ce = g_ce, *out = g_out, *self = g_self;
    join(p, game, L"nativePCx64\\sa\\DX9_64\\root.arc");
    if (!exists(p)) die(T("essa pasta não parece ser a do jogo (falta nativePCx64\\sa\\DX9_64\\root.arc).", "that does not look like the game folder (nativePCx64\\sa\\DX9_64\\root.arc is missing)."));

    // 1. executável sem a proteção
    g_stage = 1; say(T("[1/5] Conferindo o executável\n", "[1/5] Checking the executable\n"));
    join(exe, game, L"umvc3.exe.unpacked.exe"); if (g_exe_arg[0]) wcscpy(exe, g_exe_arg);
    if (!exists(exe)) {
        join(p, game, L"umvc3.exe"); if (!exists(p)) die(T("não achei o umvc3.exe na pasta do jogo.", "umvc3.exe not found in the game folder."));
        sha_file(p, hex);
        if (strcmp(hex, EXE_STEAM_SHA)) {
            say(T("  ATENÇÃO: o seu umvc3.exe é de uma versão diferente da que este port espera.\n", "  WARNING: your umvc3.exe is a different version from the one this port expects.\n"));
            if (g_yes || !box_yes(T("O seu umvc3.exe é de uma versão diferente da que este port espera.\nAtualize o jogo na Steam.\n\nSe continuar, o jogo pode não abrir no Switch. Continuar mesmo assim?",
                                    "Your umvc3.exe is a different version from the one this port expects.\nUpdate the game on Steam.\n\nIf you go on, the game may not start on the Switch. Continue anyway?")))
                die(T("versão do jogo não suportada.", "unsupported game version."));
        }
        steamless_auto(self, game, exe);
    }
    sha_file(exe, hex);
    if (strcmp(hex, EXE_SHA)) {
        say(T("  ATENÇÃO: o executável sem proteção é diferente do que este port espera.\n", "  WARNING: the unprotected executable differs from the one this port expects.\n"));
        if (g_yes || !box_yes(T("O executável sem proteção saiu diferente do que este port espera.\n\nO jogo pode não abrir no Switch. Continuar mesmo assim?",
                                "The unprotected executable differs from the one this port expects.\n\nThe game may not start on the Switch. Continue anyway?")))
            die(T("executável não suportado.", "unsupported executable."));
    } else say(T("  executável da versão suportada\n", "  executable is the supported version\n"));

    // Community Edition (opcional)
    g_with_ce = 0;
    if (ce[0]) {
        join(p, ce, L"Characters.ini"); join(q, ce, L"nativePCx64");
        if (!exists(p) || !exists(q)) die(T("a pasta da Community Edition não parece certa (falta Characters.ini ou nativePCx64).", "that does not look like the Community Edition folder (Characters.ini or nativePCx64 is missing)."));
        g_with_ce = 1;
        for (int i = 0; i < 4; i++) { join(p, ce, CE[i].name); sha_file(p, hex);
            if (strcmp(hex, CE[i].sha)) {
                say(T("  A sua Community Edition não é a versão " CE_VER ", a suportada (%ls): só o jogo original será instalado.\n",
                      "  Your Community Edition is not version " CE_VER ", the supported one (%ls): only the original game will be installed.\n"), CE[i].name); g_with_ce = 0;
                if (!g_yes) box(T("A sua Community Edition não é a versão " CE_VER ", a única suportada por este port.\n\nVou instalar só o jogo original.",
                                  "Your Community Edition is not version " CE_VER ", the only one this port supports.\n\nOnly the original game will be installed."), MB_OK | MB_ICONWARNING);
                break; } }
        if (g_with_ce) say(T("  Community Edition da versão suportada (" CE_VER ")\n", "  Community Edition is the supported version (" CE_VER ")\n"));
    }

    join(um, out, L"umvc3"); mkdirs(um);
    if (!exists(um)) die(T("não consegui criar a pasta umvc3 no destino.", "could not create the umvc3 folder at the destination."));

    // 2. shaders
    g_stage = 2; say(T("[2/5] Compilando os shaders do jogo para o Switch (cerca de um minuto)\n", "[2/5] Compiling the game shaders for the Switch (about a minute)\n"));
    join(p, game, L"nativePCx64\\sa\\DX9_64\\root.arc"); join(q, um, L"dksh.pak");
    shaders_build(p, q);

    // 3. executável, aplicativos, ícones e configuração
    g_stage = 3; say(T("[3/5] Copiando o executável, os aplicativos e os ícones\n", "[3/5] Copying the executable, the apps and the icons\n"));
    join(q, um, L"umvc3.exe.unpacked.exe"); copy_file(exe, q);
    for (size_t i = 0; i < sizeof SDFILES / sizeof SDFILES[0]; i++) {
        size_t n; const void *d = res_get(SDFILES[i].id, &n); if (!d) continue;
        join(q, out, SDFILES[i].path); if (SDFILES[i].keep && exists(q)) continue;
        wchar_t dd[1024]; wcscpy(dd, q); { wchar_t *s = wcsrchr(dd, L'\\'); if (s) *s = 0; } mkdirs(dd);
        if (!wr(q, d, n)) die(T("não consegui gravar no destino. O cartão está cheio ou protegido?", "could not write to the destination. Is the card full or write-protected?"));
    }
    int napps = 0;
    { wchar_t ad[1024], pat[1024], sw[1024]; join(ad, self, L"NRO FILES"); if (!exists(ad)) join(ad, self, L"aplicativos"); join(pat, ad, L"*.nro"); join(sw, out, L"switch");
      WIN32_FIND_DATAW fd; HANDLE h = FindFirstFileW(pat, &fd);
      if (h != INVALID_HANDLE_VALUE) { mkdirs(sw);
          do { if (!g_with_ce && !_wcsicmp(fd.cFileName, L"umvc3-ce.nro")) continue;   // sem a CE instalada, a NRO da CE não vai
               join(p, ad, fd.cFileName); join(q, sw, fd.cFileName); copy_file(p, q); napps++; say(T("  aplicativo: %ls\n", "  app: %ls\n"), fd.cFileName);
          } while (FindNextFileW(h, &fd));
          FindClose(h); } }
    join(p, self, L"sdcard"); if (exists(p)) { g_copied = g_total = 0; tree(p, out, 1); tree(p, out, 0); }
    if (!napps) say(T("  (nenhum aplicativo .nro na pasta \"NRO FILES\" ao lado do instalador: copie o .nro para a pasta switch do destino)\n",
                      "  (no .nro app in the \"NRO FILES\" folder next to the installer: copy the .nro into the switch folder at the destination)\n"));

    // 4. CE
    g_stage = 4;
    if (g_with_ce) {
        say(T("[4/5] Preparando os plugins da Community Edition\n", "[4/5] Preparing the Community Edition plugins\n"));
        wchar_t cd[1024]; join(cd, um, L"ce"); mkdirs(cd);
        for (int i = 0; i < 4; i++) { join(p, ce, CE[i].name); join(q, cd, CE[i].name); ce_plugin(p, q, CE[i].base, i == 0); }
        join(cd, um, L"ce-data"); mkdirs(cd);
        join(p, ce, L"Characters.ini"); join(q, cd, L"Characters.ini"); copy_file(p, q);
        join(p, ce, L"ColorExpansion.ini"); join(q, cd, L"ColorExpansion.ini"); if (exists(p)) copy_file(p, q);
    } else say(T("[4/5] Community Edition: não instalada\n", "[4/5] Community Edition: not installed\n"));

    // 5. dados
    g_copied = g_total = 0; g_stage = 5;
    if (g_nodata) say(T("[5/5] Dados do jogo: não copiados (--sem-dados)\n", "[5/5] Game data: not copied (--sem-dados)\n"));
    else {
        join(p, game, L"nativePCx64"); join(q, um, L"nativePCx64"); copy_tree(T("[5/5] Copiando os dados do jogo", "[5/5] Copying the game data"), p, q);
        if (g_with_ce) { join(p, ce, L"nativePCx64"); join(q, um, L"ce-data\\nativePCx64"); copy_tree(T("[5/5] Copiando os dados da Community Edition", "[5/5] Copying the Community Edition data"), p, q); }
    }
    cleanup();
    say(T("\nPronto. Em %ls:\n  switch\\   os aplicativos\n  umvc3\\    os arquivos do jogo preparados\n", "\nDone. In %ls:\n  switch\\   the apps\n  umvc3\\    the prepared game files\n"), out);
    say(T("\nSe o destino não foi o cartão, copie essas duas pastas para a raiz do cartão SD.\nNo Switch: segure R ao abrir um jogo para entrar no Homebrew Menu e escolha \"(BASE) ULTIMATE MARVEL VS CAPCOM 3\"%s.\n",
          "\nIf the destination was not the card, copy those two folders to the root of the SD card.\nOn the Switch: hold R while opening a game to enter the Homebrew Menu and pick \"(BASE) ULTIMATE MARVEL VS CAPCOM 3\"%s.\n"),
        g_with_ce ? T(" ou \"(CE) ULTIMATE MARVEL VS CAPCOM 3\"", " or \"(CE) ULTIMATE MARVEL VS CAPCOM 3\"") : "");
    g_stage = 9;
}

// ---- a janela ----
enum { ID_GAME = 101, ID_GAME_B, ID_CE_CHK, ID_CE, ID_CE_B, ID_OUT, ID_OUT_B, ID_GO, ID_CLOSE, ID_STEP, ID_BAR, ID_STATUS, ID_LANG, ID_SUB, ID_L_GAME, ID_L_OUT };
#define WM_SAY (WM_APP + 0x3A1)
#define WM_DONE (WM_APP + 0x3A2)
#define MSG_MARK ((WPARAM)0x554D5643)   // "UMVC": só as mensagens postadas por este programa valem
static HWND g_wnd; static HFONT g_font, g_font_b, g_font_t; static int g_dpi = 96, g_running, g_finished;
static int S(int v) { return MulDiv(v, g_dpi, 96); }
static HWND ctl(const wchar_t *cls, const char *text, DWORD style, int x, int y, int w, int h, int id, HFONT f)
{
    wchar_t t[400]; wide(text, t, 400);
    HWND c = CreateWindowExW(!wcscmp(cls, L"EDIT") ? WS_EX_CLIENTEDGE : 0, cls, t, WS_CHILD | WS_VISIBLE | style, S(x), S(y), S(w), S(h), g_wnd, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(c, WM_SETFONT, (WPARAM)f, TRUE); return c;
}
static void set_text(int id, const char *utf8) { wchar_t w[1200]; wide(utf8, w, 1200); SetDlgItemTextW(g_wnd, id, w); }
// Todos os textos fixos da janela: chamada ao criar e de novo quando o seletor de idioma muda.
static void apply_texts(void)
{
    SetWindowTextW(g_wnd, g_en ? L"UMvC3 on Switch: installer" : L"UMvC3 no Switch: instalador");
    set_text(ID_SUB, T("Instalação no Nintendo Switch, a partir da sua cópia do jogo na Steam. Versão 1.0.", "Install on the Nintendo Switch, from your own copy of the game on Steam. Version 1.0."));
    set_text(ID_L_GAME, T("Pasta do jogo (Steam)", "Game folder (Steam)"));
    set_text(ID_CE_CHK, T("Instalar também a Community Edition " CE_VER " (pasta \"UMvC3 Community Edition Build\")", "Also install the Community Edition " CE_VER " (\"UMvC3 Community Edition Build\" folder)"));
    set_text(ID_L_OUT, T("Destino: o cartão SD do Switch ou uma pasta do PC", "Destination: the Switch SD card or a folder on the PC"));
    set_text(ID_GAME_B, T("Procurar...", "Browse...")); set_text(ID_CE_B, T("Procurar...", "Browse...")); set_text(ID_OUT_B, T("Procurar...", "Browse..."));
    set_text(ID_GO, T("Instalar", "Install"));
    if (g_finished) {
        set_text(ID_CLOSE, T("Concluir", "Finish")); set_text(ID_STEP, T("Instalação pronta", "Installation finished"));
        set_text(ID_STATUS, T("Se o destino não foi o cartão SD, copie as pastas \"switch\" e \"umvc3\" para a raiz dele.", "If the destination was not the SD card, copy the \"switch\" and \"umvc3\" folders to its root."));
    } else {
        set_text(ID_CLOSE, T("Fechar", "Close"));
        if (!g_running) {
            set_text(ID_STEP, T("Espaço necessário: 4 GB para o jogo original, 13 GB com a Community Edition.", "Space needed: 4 GB for the original game, 13 GB with the Community Edition."));
            set_text(ID_STATUS, T("A pasta do jogo no PC não é alterada.", "The game folder on the PC is not changed."));
        }
    }
}
static void say_hook(const char *b) { if (g_wnd) PostMessageW(g_wnd, WM_SAY, MSG_MARK, (LPARAM)_strdup(b)); }
static DWORD WINAPI worker(LPVOID a) { (void)a; install_run(); PostMessageW(g_wnd, WM_DONE, MSG_MARK, 0); return 0; }
static void enable_form(int on)
{
    static const int ids[] = { ID_GAME, ID_GAME_B, ID_CE_CHK, ID_OUT, ID_OUT_B, ID_GO, ID_LANG };
    for (size_t i = 0; i < sizeof ids / sizeof ids[0]; i++) EnableWindow(GetDlgItem(g_wnd, ids[i]), on);
    int ce = on && IsDlgButtonChecked(g_wnd, ID_CE_CHK) == BST_CHECKED;
    EnableWindow(GetDlgItem(g_wnd, ID_CE), ce); EnableWindow(GetDlgItem(g_wnd, ID_CE_B), ce);
}
static void start_install(void)
{
    GetDlgItemTextW(g_wnd, ID_GAME, g_game, 1024); GetDlgItemTextW(g_wnd, ID_OUT, g_out, 1024); g_ce[0] = 0;
    if (IsDlgButtonChecked(g_wnd, ID_CE_CHK) == BST_CHECKED) GetDlgItemTextW(g_wnd, ID_CE, g_ce, 1024);
    for (wchar_t *s = g_game, *e; s; s = NULL) { e = s + wcslen(s); while (e > s + 2 && e[-1] == L'\\') *--e = 0; }
    { wchar_t *e = g_out + wcslen(g_out); while (e > g_out + 2 && e[-1] == L'\\') *--e = 0; }
    wchar_t p[1024]; join(p, g_game, L"nativePCx64\\sa\\DX9_64\\root.arc");
    if (!g_game[0] || !exists(p)) { box(T("Escolha a pasta do jogo: a \"ULTIMATE MARVEL VS. CAPCOM 3\" da Steam, que tem o umvc3.exe.", "Pick the game folder: the Steam \"ULTIMATE MARVEL VS. CAPCOM 3\" folder, which holds umvc3.exe."), MB_OK | MB_ICONWARNING); return; }
    if (IsDlgButtonChecked(g_wnd, ID_CE_CHK) == BST_CHECKED) { join(p, g_ce, L"Characters.ini");
        if (!g_ce[0] || !exists(p)) { box(T("Escolha a pasta \"UMvC3 Community Edition Build\", ou desmarque a Community Edition.", "Pick the \"UMvC3 Community Edition Build\" folder, or untick the Community Edition."), MB_OK | MB_ICONWARNING); return; } }
    if (!g_out[0]) { box(T("Escolha o destino: o cartão SD do Switch ou uma pasta do PC.", "Pick the destination: the Switch SD card or a folder on the PC."), MB_OK | MB_ICONWARNING); return; }
    g_running = 1; enable_form(0); SetTimer(g_wnd, 1, 150, NULL);
    CloseHandle(CreateThread(NULL, 0, worker, NULL, 0, NULL));
}
static LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_COMMAND: {
        int id = LOWORD(w); wchar_t p[1024];
        if (id == ID_GAME_B && pick_folder(T("Escolha a pasta do jogo (ULTIMATE MARVEL VS. CAPCOM 3)", "Pick the game folder (ULTIMATE MARVEL VS. CAPCOM 3)"), p)) SetDlgItemTextW(h, ID_GAME, p);
        else if (id == ID_CE_B && pick_folder(T("Escolha a pasta \"UMvC3 Community Edition Build\"", "Pick the \"UMvC3 Community Edition Build\" folder"), p)) SetDlgItemTextW(h, ID_CE, p);
        else if (id == ID_OUT_B && pick_folder(T("Escolha o cartão SD ou uma pasta de destino", "Pick the SD card or a destination folder"), p)) SetDlgItemTextW(h, ID_OUT, p);
        else if (id == ID_CE_CHK) enable_form(!g_running && !g_finished);
        else if (id == ID_LANG && HIWORD(w) == CBN_SELCHANGE) { g_en = SendDlgItemMessageW(h, ID_LANG, CB_GETCURSEL, 0, 0) == 0; apply_texts(); }
        else if (id == ID_GO && !g_running && !g_finished) start_install();
        else if (id == ID_CLOSE) SendMessageW(h, WM_CLOSE, 0, 0);
        return 0; }
    case WM_SAY: {   // texto de andamento vindo da thread de trabalho: "[n/5] ..." vira a etapa; o resto, o detalhe
        if (w != MSG_MARK) return 0;
        char *b = (char *)l, *s = b, *last = NULL;
        for (char *t = strtok(s, "\r\n"); t; t = strtok(NULL, "\r\n")) { while (*t == ' ') t++; if (*t) { if (*t == '[') set_text(ID_STEP, t); else last = t; } }
        if (last && g_stage != 9) set_text(ID_STATUS, last);
        free(b); return 0; }
    case WM_TIMER: {
        int pos = 0;
        if (g_stage == 2 && g_nsh > 0) pos = (int)((long long)g_sh_done * 1000 / g_nsh);
        else if (g_stage == 5 && g_total > 0) pos = (int)(g_copied * 1000 / g_total);
        else if (g_stage == 3 || g_stage == 4) pos = 1000;
        SendDlgItemMessageW(h, ID_BAR, PBM_SETPOS, (WPARAM)pos, 0); return 0; }
    case WM_DONE:
        if (w != MSG_MARK || g_finished || !g_running) return 0;   // o fim da instalação vale uma vez só
        KillTimer(h, 1); g_running = 0; g_finished = 1; SendDlgItemMessageW(h, ID_BAR, PBM_SETPOS, 1000, 0);
        apply_texts(); EnableWindow(GetDlgItem(h, ID_LANG), TRUE);
        box(g_with_ce ? T("Instalação pronta.\n\nSe o destino não foi o cartão SD, copie as pastas \"switch\" e \"umvc3\" para a raiz dele.\n\nNo Switch: segure R ao abrir um jogo para entrar no Homebrew Menu e escolha \"(BASE) ULTIMATE MARVEL VS CAPCOM 3\" ou \"(CE) ULTIMATE MARVEL VS CAPCOM 3\".",
                          "Installation finished.\n\nIf the destination was not the SD card, copy the \"switch\" and \"umvc3\" folders to its root.\n\nOn the Switch: hold R while opening a game to enter the Homebrew Menu and pick \"(BASE) ULTIMATE MARVEL VS CAPCOM 3\" or \"(CE) ULTIMATE MARVEL VS CAPCOM 3\".")
                      : T("Instalação pronta.\n\nSe o destino não foi o cartão SD, copie as pastas \"switch\" e \"umvc3\" para a raiz dele.\n\nNo Switch: segure R ao abrir um jogo para entrar no Homebrew Menu e escolha \"(BASE) ULTIMATE MARVEL VS CAPCOM 3\".",
                          "Installation finished.\n\nIf the destination was not the SD card, copy the \"switch\" and \"umvc3\" folders to its root.\n\nOn the Switch: hold R while opening a game to enter the Homebrew Menu and pick \"(BASE) ULTIMATE MARVEL VS CAPCOM 3\"."), MB_OK | MB_ICONINFORMATION);
        return 0;
    case WM_CTLCOLORSTATIC: SetBkMode((HDC)w, TRANSPARENT); return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    case WM_CLOSE:
        if (g_running && !box_yes(T("A instalação está em andamento. Cancelar e fechar?", "The installation is in progress. Cancel and close?"))) return 0;
        if (g_running) exit(1);   // atexit apaga a pasta temporária; o destino fica incompleto e pode ser refeito
        DestroyWindow(h); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
static int gui_run(void)
{
    INITCOMMONCONTROLSEX ic = { sizeof ic, ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES }; InitCommonControlsEx(&ic);
    { HDC dc = GetDC(NULL); g_dpi = GetDeviceCaps(dc, LOGPIXELSY); ReleaseDC(NULL, dc); }
    g_font = CreateFontW(-S(12), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_font_b = CreateFontW(-S(12), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_font_t = CreateFontW(-S(20), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    WNDCLASSEXW wc; memset(&wc, 0, sizeof wc); wc.cbSize = sizeof wc; wc.lpfnWndProc = wndproc; wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW); wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW); wc.lpszClassName = L"UMvC3Instalador";
    wc.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(1)); wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);
    RECT r = { 0, 0, S(560), S(392) }; DWORD st = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX; AdjustWindowRect(&r, st, FALSE);
    int ww = r.right - r.left, wh = r.bottom - r.top;
    g_wnd = CreateWindowExW(0, wc.lpszClassName, L"", st,
                            (GetSystemMetrics(SM_CXSCREEN) - ww) / 2, (GetSystemMetrics(SM_CYSCREEN) - wh) / 2, ww, wh, NULL, NULL, wc.hInstance, NULL);
    g_owner = g_wnd;
    ctl(L"STATIC", "Ultimate Marvel vs. Capcom 3", 0, 16, 14, 400, 30, 0, g_font_t);
    { HWND c = ctl(L"COMBOBOX", "", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 424, 16, 120, 200, ID_LANG, g_font);
      SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)L"English"); SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)L"Português"); SendMessageW(c, CB_SETCURSEL, g_en ? 0 : 1, 0); }
    ctl(L"STATIC", "", 0, 16, 46, 528, 18, ID_SUB, g_font);
    ctl(L"STATIC", "", 0, 16, 80, 400, 18, ID_L_GAME, g_font_b);
    ctl(L"EDIT", "", ES_AUTOHSCROLL | WS_TABSTOP, 16, 100, 426, 24, ID_GAME, g_font);
    ctl(L"BUTTON", "", WS_TABSTOP, 450, 99, 94, 26, ID_GAME_B, g_font);
    ctl(L"BUTTON", "", BS_AUTOCHECKBOX | WS_TABSTOP, 16, 136, 528, 20, ID_CE_CHK, g_font_b);
    ctl(L"EDIT", "", ES_AUTOHSCROLL | WS_TABSTOP, 16, 158, 426, 24, ID_CE, g_font);
    ctl(L"BUTTON", "", WS_TABSTOP, 450, 157, 94, 26, ID_CE_B, g_font);
    ctl(L"STATIC", "", 0, 16, 196, 528, 18, ID_L_OUT, g_font_b);
    ctl(L"EDIT", "", ES_AUTOHSCROLL | WS_TABSTOP, 16, 216, 426, 24, ID_OUT, g_font);
    ctl(L"BUTTON", "", WS_TABSTOP, 450, 215, 94, 26, ID_OUT_B, g_font);
    ctl(L"STATIC", "", 0, 16, 256, 528, 18, ID_STEP, g_font_b);
    ctl(PROGRESS_CLASSW, "", 0, 16, 280, 528, 16, ID_BAR, g_font);
    ctl(L"STATIC", "", 0, 16, 302, 528, 34, ID_STATUS, g_font);
    ctl(L"BUTTON", "", BS_DEFPUSHBUTTON | WS_TABSTOP, 344, 346, 96, 30, ID_GO, g_font_b);
    ctl(L"BUTTON", "", WS_TABSTOP, 448, 346, 96, 30, ID_CLOSE, g_font);
    apply_texts();
    SendDlgItemMessageW(g_wnd, ID_BAR, PBM_SETRANGE32, 0, 1000);
    if (g_game[0]) SetDlgItemTextW(g_wnd, ID_GAME, g_game);
    if (g_ce[0]) { CheckDlgButton(g_wnd, ID_CE_CHK, BST_CHECKED); SetDlgItemTextW(g_wnd, ID_CE, g_ce); }
    if (g_out[0]) SetDlgItemTextW(g_wnd, ID_OUT, g_out);
    enable_form(1); g_say_hook = say_hook;
    ShowWindow(g_wnd, SW_SHOW); UpdateWindow(g_wnd);
    MSG msg; while (GetMessageW(&msg, NULL, 0, 0) > 0) { if (!IsDialogMessageW(g_wnd, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); } }
    return 0;
}

int wmain(int argc, wchar_t **argv)
{
    SetConsoleOutputCP(CP_UTF8); CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    g_en = 1;   // abre em inglês; o seletor da janela (ou --lang pt) troca para português
    for (int i = 1; i < argc; i++) {
        if (!wcscmp(argv[i], L"--jogo") && i + 1 < argc) wcscpy(g_game, argv[++i]);
        else if (!wcscmp(argv[i], L"--ce") && i + 1 < argc) wcscpy(g_ce, argv[++i]);
        else if (!wcscmp(argv[i], L"--saida") && i + 1 < argc) wcscpy(g_out, argv[++i]);
        else if (!wcscmp(argv[i], L"--exe") && i + 1 < argc) wcscpy(g_exe_arg, argv[++i]);
        else if (!wcscmp(argv[i], L"--lang") && i + 1 < argc) { i++; g_en = !(argv[i][0] == L'p' || argv[i][0] == L'P'); }
        else if (!wcscmp(argv[i], L"--sem-dados")) g_nodata = 1;
        else if (!wcscmp(argv[i], L"--sim")) g_yes = 1;
    }
    GetModuleFileNameW(NULL, g_self, 1024); { wchar_t *s = wcsrchr(g_self, L'\\'); if (s) *s = 0; }
    { wchar_t t[512]; GetTempPathW(512, t); swprintf(g_tmp, 1024, L"%lsumvc3-instalador-%lu", t, GetCurrentProcessId()); mkdirs(g_tmp); atexit(cleanup); }

    // compilador de shaders: embutido, ou ferramentas\uam.exe ao lado
    { size_t n; const void *u = res_get(100, &n);
      if (u) { join(g_uam, g_tmp, L"uam.exe"); if (!wr(g_uam, u, n)) die(T("não consegui usar a pasta temporária do Windows.", "could not use the Windows temporary folder.")); }
      else { join(g_uam, g_self, L"ferramentas\\uam.exe"); if (!exists(g_uam)) die(T("não achei o compilador de shaders (ferramentas\\uam.exe).", "shader compiler not found (ferramentas\\uam.exe).")); } }

    { size_t n; const void *e = res_get(130, &n); if (e) { g_exp = e; g_exp_n = n; } }   // tabela de conferência dos shaders
    if (!g_game[0]) { wchar_t p[1024]; join(p, STEAM_DEF, L"nativePCx64"); if (exists(p)) wcscpy(g_game, STEAM_DEF); }
    if (!g_yes) return gui_run();

    // modo automático (testes): sem janela
    say(T("Instalador do UMvC3 no Switch\n=============================\n\n", "UMvC3 on Switch installer\n=========================\n\n"));
    if (!g_game[0]) die(T("pasta do jogo não informada (--jogo).", "game folder not given (--jogo)."));
    if (!g_out[0]) die(T("destino não informado (--saida).", "destination not given (--saida)."));
    install_run();
    return 0;
}
