// Preparador do UMvC3 no Switch (Windows). A partir da cópia do jogo do próprio usuário, monta o que vai para o
// cartão SD: confere o executável, compila os shaders para o deko3d (uam), prepara os plugins da Community Edition
// e copia os dados. Nada do jogo é distribuído com esta ferramenta.
//
//   Preparador.exe [--jogo PASTA] [--exe ARQUIVO] [--ce PASTA] [--saida PASTA] [--sem-dados] [--sim]
//
// --exe: o umvc3.exe.unpacked.exe quando ele não está na pasta do jogo.
//
// Sem argumentos ele pergunta. --sem-dados não copia nativePCx64 nem os dados da CE (para quem já os tem no cartão).
// --sim aceita as perguntas de confirmação (uso automático).
//
// Compilar: x86_64-w64-mingw32-gcc -O2 -municode -static -I../../src/gfx preparador.c ../../src/gfx/sm3_glsl.c -o Preparador.exe
// Ao lado do executável: ferramentas\uam.exe (compilador de shaders do devkitPro) e a pasta sdcard\ do pacote.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "sm3_glsl.h"

#define EXE_SHA "946ca80925b0c65ebb49c958e0c052bde37bb96a271eb2a26cb8eec5b99afc50"      // umvc3.exe.unpacked.exe suportado
#define EXE_STEAM_SHA "b2f2fabe01949f1130f1cc42d31709a2a8cbed9c524330b5fdee71583bd21e7e"   // umvc3.exe da Steam (com SteamStub)
static const struct { const wchar_t *name; uint64_t base; const char *sha; } CE[4] = {
    { L"CloneEngine.asi", 0x17F000000ull, "855a3bd1a743fa47e1bcb8d09cbff37f2c24dcec899e54b43c631209f295b131" },
    { L"ColorExpansion.asi", 0x17F100000ull, "a178f560c2b770fffb7cabdf20b8116b6e4c7828566b20888155602c681d50a6" },
    { L"mag_patch.asi", 0x17F200000ull, "483349264c970c6ead153e102602c81ef565790b07559522df995173c12da305" },
    { L"mod_enable_damage_counter.asi", 0x17F300000ull, "f6198c76a13a418d3b35ef23a2333f68fe8d34d409670856f8642ad40f14b224" },
};

static int g_yes;
static int g_en;   // 1 = textos em inglês (instalador: idioma do Windows ou --lang)
#define T(pt, en) (g_en ? (en) : (pt))
static void (*g_say_hook)(const char *);   // instalador com janela: recebe cada texto de andamento
static HWND g_owner;                        // janela dona das caixas de mensagem (NULL no modo de linha de comando)
static volatile int g_sh_done;              // shaders já compilados (para a barra de progresso)
// Conferência dos shaders (tools/dksh_expected.py): em 2026-11-19 uma instalação saiu com 1 byte diferente num shader
// (o uam emitiu stall 7 em vez de 8 numa instrução; 640 recompilações desse shader não repetiram). Com a tabela, cada
// shader compilado é comparado com o do pacote testado no console e recompilado se sair diferente. NULL = sem conferir.
static const uint8_t *g_exp; static size_t g_exp_n; static int g_sh_retry, g_sh_diff, g_sh_unknown;
static void say(const char *fmt, ...)
{
    char b[1400]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    fputs(b, stdout); fflush(stdout); if (g_say_hook) g_say_hook(b);
}
static void die(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); printf(T("\nERRO: ", "\nERROR: ")); vprintf(fmt, ap); va_end(ap);
#ifdef PREPARADOR_SEM_MAIN
    if (!g_yes) { char m[1500]; wchar_t w[1600]; va_start(ap, fmt); vsnprintf(m, sizeof m, fmt, ap); va_end(ap);
        MultiByteToWideChar(CP_UTF8, 0, m, -1, w, 1600); fflush(stdout);
        MessageBoxW(g_owner, w, g_en ? L"UMvC3 on Switch" : L"UMvC3 no Switch", MB_OK | MB_ICONERROR | MB_SETFOREGROUND); exit(1); }
#endif
    printf(T("\n\nNada foi alterado na pasta do jogo. Pressione Enter para fechar.\n", "\n\nNothing was changed in the game folder. Press Enter to close.\n")); fflush(stdout);
    if (!g_yes) getwchar();
    exit(1);
}

// ---- arquivos ----
static uint8_t *rd(const wchar_t *p, size_t *n)
{
    FILE *f = _wfopen(p, L"rb"); if (!f) return NULL;
    _fseeki64(f, 0, SEEK_END); long long sz = _ftelli64(f); _fseeki64(f, 0, SEEK_SET);
    uint8_t *b = malloc((size_t)sz + 16); if (!b) { fclose(f); return NULL; }
    if (fread(b, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); free(b); return NULL; }
    fclose(f); *n = (size_t)sz; return b;
}
static int wr(const wchar_t *p, const void *b, size_t n)
{
    FILE *f = _wfopen(p, L"wb"); if (!f) return 0;
    int ok = fwrite(b, 1, n, f) == n; fclose(f); return ok;
}
static int exists(const wchar_t *p) { return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES; }
static void mkdirs(const wchar_t *p)
{
    wchar_t t[1024]; wcsncpy(t, p, 1023); t[1023] = 0;
    for (wchar_t *q = t + 3; *q; q++) if (*q == L'\\' || *q == L'/') { *q = 0; CreateDirectoryW(t, NULL); *q = L'\\'; }
    CreateDirectoryW(t, NULL);
}
static void join(wchar_t *o, const wchar_t *a, const wchar_t *b) { swprintf(o, 1024, L"%ls\\%ls", a, b); }

// ---- SHA-256 ----
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };
#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
static void sha_block(uint32_t *h, const uint8_t *p)
{
    uint32_t w[64], a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int i = 0; i < 16; i++) w[i] = (uint32_t)p[i * 4] << 24 | (uint32_t)p[i * 4 + 1] << 16 | (uint32_t)p[i * 4 + 2] << 8 | p[i * 4 + 3];
    for (int i = 16; i < 64; i++) { uint32_t s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3), s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10); w[i] = w[i - 16] + s0 + w[i - 7] + s1; }
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = hh + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i], t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}
static void sha256(const uint8_t *d, size_t n, char *hex)
{
    uint32_t h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    size_t i = 0; for (; i + 64 <= n; i += 64) sha_block(h, d + i);
    uint8_t t[128]; size_t r = n - i; memset(t, 0, sizeof t); memcpy(t, d + i, r); t[r] = 0x80;
    size_t tl = r + 9 <= 64 ? 64 : 128; uint64_t bits = (uint64_t)n * 8;
    for (int k = 0; k < 8; k++) t[tl - 1 - k] = (uint8_t)(bits >> (8 * k));
    sha_block(h, t); if (tl == 128) sha_block(h, t + 64);
    for (int k = 0; k < 8; k++) sprintf(hex + k * 8, "%08x", h[k]);
}

// ---- inflate (zlib/deflate), decodificação canônica bit a bit: os arquivos são pequenos ----
typedef struct { const uint8_t *in; size_t inlen, inpos; uint32_t bitbuf; int bitcnt; uint8_t *out; size_t outlen, outpos; int err; } zs;
static int zbits(zs *s, int n)
{
    uint32_t v = s->bitbuf;
    while (s->bitcnt < n) { if (s->inpos >= s->inlen) { s->err = 1; return 0; } v |= (uint32_t)s->in[s->inpos++] << s->bitcnt; s->bitcnt += 8; }
    s->bitbuf = v >> n; s->bitcnt -= n; return (int)(v & ((1u << n) - 1));
}
typedef struct { uint16_t count[16], symbol[288]; } zhuff;
static void zbuild(zhuff *h, const uint8_t *len, int n)
{
    uint16_t offs[16]; memset(h->count, 0, sizeof h->count);
    for (int i = 0; i < n; i++) h->count[len[i]]++;
    h->count[0] = 0; offs[1] = 0; for (int i = 1; i < 15; i++) offs[i + 1] = (uint16_t)(offs[i] + h->count[i]);
    for (int i = 0; i < n; i++) if (len[i]) h->symbol[offs[len[i]]++] = (uint16_t)i;
}
static int zdecode(zs *s, const zhuff *h)
{
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= 15; len++) {
        code |= zbits(s, 1); if (s->err) return -1;
        int count = h->count[len];
        if (code - count < first) return h->symbol[index + (code - first)];
        index += count; first += count; first <<= 1; code <<= 1;
    }
    s->err = 1; return -1;
}
static int zcodes(zs *s, const zhuff *lc, const zhuff *dc)
{
    static const uint16_t lens[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
    static const uint16_t lext[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
    static const uint16_t dists[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
    static const uint16_t dext[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
    for (;;) {
        int sym = zdecode(s, lc); if (sym < 0) return 0;
        if (sym < 256) { if (s->outpos >= s->outlen) return 0; s->out[s->outpos++] = (uint8_t)sym; }
        else if (sym == 256) return 1;
        else {
            sym -= 257; if (sym >= 29) return 0;
            int len = lens[sym] + zbits(s, lext[sym]); int ds = zdecode(s, dc); if (ds < 0 || ds >= 30) return 0;
            size_t dist = (size_t)dists[ds] + (size_t)zbits(s, dext[ds]);
            if (s->err || dist > s->outpos || s->outpos + (size_t)len > s->outlen) return 0;
            for (int i = 0; i < len; i++) { s->out[s->outpos] = s->out[s->outpos - dist]; s->outpos++; }
        }
    }
}
static int zinflate(const uint8_t *in, size_t inlen, uint8_t *out, size_t outlen)
{
    zs s = { in, inlen, 2, 0, 0, out, outlen, 0, 0 };   // pula os 2 bytes do cabeçalho zlib
    if (inlen < 2) return 0;
    for (;;) {
        int last = zbits(&s, 1), type = zbits(&s, 2); if (s.err) return 0;
        if (type == 0) {
            s.bitbuf = 0; s.bitcnt = 0; if (s.inpos + 4 > s.inlen) return 0;
            size_t len = s.in[s.inpos] | (size_t)s.in[s.inpos + 1] << 8; s.inpos += 4;
            if (s.inpos + len > s.inlen || s.outpos + len > s.outlen) return 0;
            memcpy(s.out + s.outpos, s.in + s.inpos, len); s.inpos += len; s.outpos += len;
        } else if (type == 1) {
            static zhuff lc, dc; uint8_t l[288]; int i;
            for (i = 0; i < 144; i++) l[i] = 8; for (; i < 256; i++) l[i] = 9; for (; i < 280; i++) l[i] = 7; for (; i < 288; i++) l[i] = 8;
            zbuild(&lc, l, 288); for (i = 0; i < 30; i++) l[i] = 5; zbuild(&dc, l, 30);
            if (!zcodes(&s, &lc, &dc)) return 0;
        } else if (type == 2) {
            static const uint8_t order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
            int nlen = zbits(&s, 5) + 257, ndist = zbits(&s, 5) + 1, ncode = zbits(&s, 4) + 4; uint8_t l[320]; zhuff cl, lc, dc;
            if (s.err || nlen > 286 || ndist > 30) return 0;
            memset(l, 0, sizeof l); for (int i = 0; i < ncode; i++) l[order[i]] = (uint8_t)zbits(&s, 3);
            zbuild(&cl, l, 19);
            int idx = 0; memset(l, 0, sizeof l);
            while (idx < nlen + ndist) {
                int sym = zdecode(&s, &cl); if (sym < 0) return 0;
                if (sym < 16) l[idx++] = (uint8_t)sym;
                else { int rep, v = 0;
                    if (sym == 16) { if (!idx) return 0; v = l[idx - 1]; rep = 3 + zbits(&s, 2); }
                    else if (sym == 17) rep = 3 + zbits(&s, 3); else rep = 11 + zbits(&s, 7);
                    if (idx + rep > nlen + ndist) return 0;
                    while (rep--) l[idx++] = (uint8_t)v; }
            }
            zbuild(&lc, l, nlen); zbuild(&dc, l + nlen, ndist);
            if (!zcodes(&s, &lc, &dc)) return 0;
        } else return 0;
        if (s.err) return 0;
        if (last) break;
    }
    return s.outpos == outlen;
}

// ---- shaders: extração (igual a tools/shader_extract.py) ----
typedef struct { int kind; uint64_t h; const uint8_t *p; size_t n; uint8_t *dksh; size_t dn; } shader;
static shader *g_sh; static int g_nsh, g_capsh;
static uint32_t ld32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint64_t ld64(const uint8_t *p) { return ld32(p) | (uint64_t)ld32(p + 4) << 32; }
static void st32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void st64(uint8_t *p, uint64_t v) { st32(p, (uint32_t)v); st32(p + 4, (uint32_t)(v >> 32)); }
static size_t sh_walk(const uint8_t *d, size_t n, size_t o)
{
    size_t p = o + 4; int ins = 0;
    while (p + 4 <= n) {
        uint32_t t = ld32(d + p);
        if (t == 0x0000FFFFu) return ins >= 2 ? p + 4 : 0;
        uint32_t op = t & 0xFFFF, ln;
        if (op == 0xFFFE) ln = (t >> 16) & 0x7FFF;
        else { if (t & 0x80000000u) return 0; if (op > 0x60 && op != 0xFFFD) return 0; ln = (t >> 24) & 0xF; }
        p += 4 + 4 * (size_t)ln; ins++;
        if (p - o > (1u << 20)) return 0;
    }
    return 0;
}
static int sh_cmp(const void *a, const void *b)
{
    const shader *x = a, *y = b;
    if (x->kind != y->kind) return x->kind - y->kind;
    return x->h < y->h ? -1 : x->h > y->h;
}
static void sh_extract(const uint8_t *d, size_t n)
{
    for (int kind = 0; kind < 2; kind++) {
        const uint8_t magic[4] = { 0x00, 0x03, kind ? 0xff : 0xfe, 0xff };
        size_t k = 0;
        while (k + 4 <= n) {
            if (memcmp(d + k, magic, 4)) { k++; continue; }
            size_t e = sh_walk(d, n, k);
            if (!e) { k += 4; continue; }
            uint64_t h = 1469598103934665603ull;   // semente do port (não é a FNV padrão)
            for (size_t i = k; i < e; i++) { h ^= d[i]; h *= 0x100000001b3ull; }
            int dup = 0; for (int i = 0; i < g_nsh; i++) if (g_sh[i].kind == kind && g_sh[i].h == h) { dup = 1; break; }
            if (!dup) {
                if (g_nsh == g_capsh) { g_capsh = g_capsh ? g_capsh * 2 : 8192; g_sh = realloc(g_sh, (size_t)g_capsh * sizeof *g_sh); }
                g_sh[g_nsh++] = (shader){ kind, h, d + k, e - k, NULL, 0 };
            }
            k = e;
        }
    }
}

// ---- shaders: palavras de escalonamento (igual a tools/dksh_sched_mod.py, modo pred6+noyield) ----
// O código que o uam gera roda certo no emulador e errado na GPU real sem isto: desliga o dual-issue (yield = 0,
// stall 0 -> 1) e garante stall >= 6 depois de cada instrução predicada.
static int sched_fix(uint8_t *b, size_t n)
{
    if (n < 24) return 0;
    uint32_t magic = ld32(b), cs = ld32(b + 8), code = ld32(b + 12), po = ld32(b + 16), np = ld32(b + 20);
    if (magic != 0x48534B44u || np != 1 || (size_t)po + 8 > n || (size_t)cs + code > n || (size_t)cs + 8 > n) return 0;
    if (memcmp(b + cs, "lol nvid", 8) || ld32(b + po + 4) != 0x30) return 0;
    for (size_t o = (size_t)cs + 0x80; o + 7 < (size_t)cs + code; o += 32) {
        if (o + 32 > n) break;
        int any = 0; for (int i = 0; i < 32; i++) any |= b[o + i];
        if (!any) continue;
        uint64_t w = ld64(b + o), nw = 0;
        for (int k = 0; k < 3; k++) {
            uint64_t f = (w >> (21 * k)) & 0x1fffff, ins = ld64(b + o + 8 + 8 * (size_t)k);
            if ((f & 0x10) || !(f & 0xf)) f = (f & ~0x1full) | 1;
            if (((ins >> 16) & 0xf) != 7 && (f & 0xf) < 6) f = (f & ~0xfull) | 6;
            nw |= f << (21 * k);
        }
        st64(b + o, nw);
    }
    return 1;
}

// ---- shaders: tradução + uam em paralelo ----
static wchar_t g_tmp[1024], g_uam[1024];
static HANDLE uam_start(int i)
{
    wchar_t g[1024], d[1024], cmd[3000];
    swprintf(g, 1024, L"%ls\\%d.glsl", g_tmp, i); swprintf(d, 1024, L"%ls\\%d.dksh", g_tmp, i);
    static uint32_t tok[1 << 16]; static char out[1 << 18]; sm3_info info;
    size_t nt = g_sh[i].n / 4; if (nt > (1 << 16)) nt = 1 << 16;
    memcpy(tok, g_sh[i].p, nt * 4);
    size_t k = sm3_to_glsl(tok, nt, out, sizeof out, &info);
    if (!k) die(T("a tradução do shader %s_%016llx falhou: %s", "translating shader %s_%016llx failed: %s"), g_sh[i].kind ? "ps" : "vs", (unsigned long long)g_sh[i].h, info.err);
    if (!wr(g, out, k)) die(T("não consegui gravar na pasta temporária", "could not write to the temporary folder"));
    swprintf(cmd, 3000, L"\"%ls\" -s %ls -o \"%ls\" \"%ls\"", g_uam, g_sh[i].kind ? L"frag" : L"vert", d, g);
    STARTUPINFOW si; PROCESS_INFORMATION pi; memset(&si, 0, sizeof si); si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES; si.hStdInput = si.hStdOutput = si.hStdError = INVALID_HANDLE_VALUE;
    if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) die(T("não consegui executar o uam.exe (%lu)", "could not run uam.exe (%lu)"), GetLastError());
    CloseHandle(pi.hThread); return pi.hProcess;
}
static uint64_t fnv64(const uint8_t *b, size_t n) { uint64_t h = 0xcbf29ce484222325ull; for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 0x100000001b3ull; } return h; }
// 1 = igual ao do pacote testado; 0 = diferente; -1 = shader fora da tabela (ou sem tabela)
static int exp_check(int i)
{
    static int fake = -2;   // teste: UMVC3_PREP_FAKEDIFF=N faz o shader de índice N "sair diferente" uma vez
    if (fake == -2) { const char *e = getenv("UMVC3_PREP_FAKEDIFF"); fake = e ? atoi(e) : -1; }
    if (!g_exp || g_exp_n < 40 || memcmp(g_exp, "DKEX", 4)) return -1;
    uint32_t n = ld32(g_exp + 4), lo = 0, hi = n; if (40 + 24 * (size_t)n > g_exp_n) return -1;
    while (lo < hi) {
        uint32_t m = (lo + hi) / 2; const uint8_t *e = g_exp + 40 + 24 * (size_t)m; uint64_t h = ld64(e); int k = (int)ld32(e + 8);
        if (k < g_sh[i].kind || (k == g_sh[i].kind && h < g_sh[i].h)) lo = m + 1; else hi = m;
    }
    if (lo >= n) return -1;
    const uint8_t *e = g_exp + 40 + 24 * (size_t)lo;
    if (ld64(e) != g_sh[i].h || (int)ld32(e + 8) != g_sh[i].kind) return -1;
    if (i == fake) { fake = -1; return 0; }
    return ld32(e + 12) == g_sh[i].dn && ld64(e + 16) == fnv64(g_sh[i].dksh, g_sh[i].dn);
}
static void uam_finish(int i)
{
    wchar_t g[1024], d[1024]; swprintf(g, 1024, L"%ls\\%d.glsl", g_tmp, i); swprintf(d, 1024, L"%ls\\%d.dksh", g_tmp, i);
    for (int tries = 0; ; tries++) {
        g_sh[i].dksh = rd(d, &g_sh[i].dn);
        if (!g_sh[i].dksh || !sched_fix(g_sh[i].dksh, g_sh[i].dn)) die(T("o uam não compilou o shader %s_%016llx", "uam did not compile shader %s_%016llx"), g_sh[i].kind ? "ps" : "vs", (unsigned long long)g_sh[i].h);
        int c = exp_check(i);
        if (c != 0) { if (c < 0 && g_exp) g_sh_unknown++; break; }
        if (tries >= 3) { g_sh_diff++; break; }   // continua diferente depois de 3 recompilações: fica como saiu e entra na conta
        g_sh_retry++; free(g_sh[i].dksh); g_sh[i].dksh = NULL; DeleteFileW(d);
        HANDLE h = uam_start(i); WaitForSingleObject(h, INFINITE); CloseHandle(h);
    }
    DeleteFileW(g); DeleteFileW(d);
}
static void shaders_build(const wchar_t *arc, const wchar_t *outpak)
{
    size_t n; uint8_t *d = rd(arc, &n); if (!d) die(T("não consegui ler os shaders do jogo (sa\\DX9_64\\root.arc)", "could not read the game shaders (sa\\DX9_64\\root.arc)"));
    if (n >= 8 && !memcmp(d, "ARC\0", 4)) {   // arquivo do MT Framework: entradas de 0x50 bytes, dados em zlib
        unsigned cnt = d[6] | d[7] << 8; size_t tot = 0;
        for (unsigned i = 0; i < cnt; i++) tot += ld32(d + 8 + 0x50 * i + 72) & 0x1fffffffu;
        uint8_t *all = malloc(tot + 16); size_t pos = 0;
        for (unsigned i = 0; i < cnt; i++) {
            const uint8_t *e = d + 8 + 0x50 * i + 68; uint32_t csz = ld32(e), usz = ld32(e + 4) & 0x1fffffffu, off = ld32(e + 8);
            if ((size_t)off + csz > n || !zinflate(d + off, csz, all + pos, usz)) die(T("o arquivo de shaders do jogo está corrompido (entrada %u)", "the game shader archive is corrupted (entry %u)"), i);
            pos += usz;
        }
        d = all; n = pos;
    }
    sh_extract(d, n);
    qsort(g_sh, (size_t)g_nsh, sizeof *g_sh, sh_cmp);
    say(T("  %d shaders encontrados na sua cópia do jogo\n", "  %d shaders found in your copy of the game\n"), g_nsh);
    if (g_nsh < 1000) die(T("poucos shaders encontrados: a pasta do jogo parece incompleta", "too few shaders found: the game folder looks incomplete"));
    sm3_target = 2;
    SYSTEM_INFO si; GetSystemInfo(&si); int jobs = (int)si.dwNumberOfProcessors; if (jobs > 16) jobs = 16; if (jobs < 1) jobs = 1;
    HANDLE hs[16]; int id[16], nrun = 0, next = 0, done = 0; DWORD t0 = GetTickCount();
    while (done < g_nsh) {
        while (nrun < jobs && next < g_nsh) { hs[nrun] = uam_start(next); id[nrun] = next; nrun++; next++; }
        DWORD r = WaitForMultipleObjects((DWORD)nrun, hs, FALSE, INFINITE);
        if (r >= WAIT_OBJECT_0 + (DWORD)nrun) die(T("espera pelo uam falhou", "waiting for uam failed"));
        int k = (int)(r - WAIT_OBJECT_0); CloseHandle(hs[k]); uam_finish(id[k]);
        hs[k] = hs[nrun - 1]; id[k] = id[nrun - 1]; nrun--; done++; g_sh_done = done;
        if (done % 50 == 0 || done == g_nsh) say(T("\r  compilando shaders: %d de %d (%lu s)   ", "\r  compiling shaders: %d of %d (%lu s)   "), done, g_nsh, (GetTickCount() - t0) / 1000);
    }
    say("\n");
    // pacote "DKP2" (igual a tools/dksh_pack.py, opções = 1: faixa de profundidade 0 a 1)
    size_t off = (16 + 24 * (size_t)g_nsh + 15) & ~(size_t)15, total = off;
    for (int i = 0; i < g_nsh; i++) total += (g_sh[i].dn + 15) & ~(size_t)15;
    uint8_t *pk = calloc(total, 1); st32(pk, 0x32504B44u); st32(pk + 4, (uint32_t)g_nsh); st32(pk + 8, 1);
    for (int i = 0; i < g_nsh; i++) {
        uint8_t *e = pk + 16 + 24 * (size_t)i; st64(e, g_sh[i].h); st32(e + 8, (uint32_t)g_sh[i].kind); st32(e + 12, (uint32_t)off); st32(e + 16, (uint32_t)g_sh[i].dn);
        memcpy(pk + off, g_sh[i].dksh, g_sh[i].dn); off += (g_sh[i].dn + 15) & ~(size_t)15;
    }
    if (!wr(outpak, pk, total)) die(T("não consegui gravar o pacote de shaders no destino", "could not write the shader pack to the destination"));
    say(T("  pacote de shaders gravado (%.1f MB)\n", "  shader pack written (%.1f MB)\n"), (double)total / 1048576.0);
    if (g_exp && g_exp_n >= 40) {
        char hex[80], ref[80]; sha256(pk, total, hex); for (int k = 0; k < 32; k++) sprintf(ref + 2 * k, "%02x", g_exp[8 + k]);
        if (!strcmp(hex, ref)) say(T("  pacote conferido: idêntico ao testado no console (%d shader(s) recompilado(s))\n", "  pack verified: identical to the one tested on console (%d shader(s) recompiled)\n"), g_sh_retry);
        else say(T("  ATENÇÃO: o pacote de shaders saiu diferente do testado (%d diferentes, %d fora da tabela)\n", "  WARNING: the shader pack differs from the tested one (%d differ, %d not in the table)\n"), g_sh_diff, g_sh_unknown);
    }
}

// ---- plugins da CE: rebase (tools/pe_rebase.py) e remapeamento do CloneEngine (tools/ce_remap.py) ----
static size_t pe_off(const uint8_t *d, size_t o, int ns, uint32_t rva, int *ok)
{
    for (int i = 0; i < ns; i++) { const uint8_t *s = d + o + 40 * (size_t)i + 8; uint32_t va = ld32(s + 4), rs = ld32(s + 8), ro = ld32(s + 12); if (rva >= va && rva < va + rs) { *ok = 1; return (size_t)ro + rva - va; } }
    *ok = 0; return 0;
}
static void ce_plugin(const wchar_t *src, const wchar_t *dst, uint64_t base, int remap)
{
    size_t n; uint8_t *d = rd(src, &n); if (!d) die(T("não consegui ler um plugin da CE", "could not read a CE plugin"));
    uint32_t pe = ld32(d + 0x3c); if (pe + 264 > n || memcmp(d + pe, "PE\0\0", 4) || (d[pe + 24] | d[pe + 25] << 8) != 0x20b) die(T("plugin da CE em formato inesperado", "CE plugin in an unexpected format"));
    int ns = d[pe + 6] | d[pe + 7] << 8; size_t opt = pe + 24, o = opt + (size_t)(d[pe + 20] | d[pe + 21] << 8);
    uint64_t old = ld64(d + opt + 24), delta = base - old; uint32_t rrva = ld32(d + opt + 112 + 40), rsize = ld32(d + opt + 112 + 44);
    int ok; size_t p = pe_off(d, o, ns, rrva, &ok), end = p + rsize; if (!ok || end > n) die(T("plugin da CE sem tabela de relocação", "CE plugin without a relocation table"));
    while (p + 8 <= end) {
        uint32_t page = ld32(d + p), bsz = ld32(d + p + 4); if (!bsz) break;
        for (uint32_t k = 0; k < (bsz - 8) / 2; k++) {
            unsigned e = d[p + 8 + 2 * k] | d[p + 9 + 2 * k] << 8, t = e >> 12, ro = e & 0xFFF;
            if (!t) continue; if (t != 10) die(T("plugin da CE com relocação de tipo %u", "CE plugin with relocation type %u"), t);
            size_t fo = pe_off(d, o, ns, page + ro, &ok); if (!ok) continue;
            st64(d + fo, ld64(d + fo) + delta);
        }
        p += bsz;
    }
    st64(d + opt + 24, base);
    if (remap) {   // CloneEngine: 12 endereços fixos pedidos ao VirtualAlloc vão para a faixa dele na memória do port
        static const struct { uint64_t c; int nblk; } M[12] = { { 0x150000000ull, 1 }, { 0x160000000ull, 1 }, { 0x190000000ull, 1 }, { 0x1A0000000ull, 1 }, { 0x1B0000000ull, 1 }, { 0x200000000ull, 1 },
            { 0x210000000ull, 2 }, { 0x220000000ull, 1 }, { 0x230000000ull, 1 }, { 0x240000000ull, 1 }, { 0x250000000ull, 1 }, { 0x260000000ull, 1 } };
        uint64_t to[12], blk = 0x17F310000ull; int hit[12] = { 0 };
        for (int i = 0; i < 12; i++) { to[i] = blk; blk += (uint64_t)M[i].nblk * 0x10000; }
        const uint8_t *s = d + o + 8; uint32_t vs = ld32(s), rs = ld32(s + 8), ro = ld32(s + 12), len = vs < rs ? vs : rs;
        for (size_t i = ro; i + 10 < (size_t)ro + len; i++)
            if ((d[i] == 0x48 || d[i] == 0x49) && d[i + 1] >= 0xB8 && d[i + 1] <= 0xBF) { uint64_t v = ld64(d + i + 2); for (int m = 0; m < 12; m++) if (v == M[m].c) { st64(d + i + 2, to[m]); hit[m]++; } }
        for (int m = 0; m < 12; m++) if (!hit[m]) die(T("o CloneEngine desta CE não é o da versão suportada", "the CloneEngine of this CE is not the supported version"));
    }
    if (!wr(dst, d, n)) die(T("não consegui gravar um plugin da CE no destino", "could not write a CE plugin to the destination"));
    free(d);
}

// ---- cópia de pastas com progresso ----
static unsigned long long g_copied, g_total; static DWORD g_tick;
static void tree(const wchar_t *src, const wchar_t *dst, int count_only)
{
    wchar_t pat[1024], a[1024], b[1024]; join(pat, src, L"*"); WIN32_FIND_DATAW fd; HANDLE h = FindFirstFileW(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    if (!count_only) mkdirs(dst);
    do {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        join(a, src, fd.cFileName); join(b, dst, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { tree(a, b, count_only); continue; }
        unsigned long long sz = ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        if (count_only) { g_total += sz; continue; }
        if (!_wcsicmp(fd.cFileName, L"config.ini") && exists(b)) continue;   // nunca troca a configuração que o usuário já tem
        WIN32_FILE_ATTRIBUTE_DATA da; int same = GetFileAttributesExW(b, GetFileExInfoStandard, &da) && da.nFileSizeLow == fd.nFileSizeLow && da.nFileSizeHigh == fd.nFileSizeHigh
            && CompareFileTime(&da.ftLastWriteTime, &fd.ftLastWriteTime) == 0;
        if (!same && !CopyFileW(a, b, FALSE)) die(T("falha ao copiar um arquivo para o destino (erro %lu). O cartão está cheio ou protegido?", "failed to copy a file to the destination (error %lu). Is the card full or write-protected?"), GetLastError());
        g_copied += sz;
        if (GetTickCount() - g_tick > 500) { g_tick = GetTickCount(); say(T("\r  copiando: %.2f de %.2f GB   ", "\r  copying: %.2f of %.2f GB   "), (double)g_copied / 1073741824.0, (double)g_total / 1073741824.0); }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}
static void copy_tree(const char *what, const wchar_t *src, const wchar_t *dst)
{
    g_copied = g_total = 0; tree(src, dst, 1);
    say("%s (%.2f GB)\n", what, (double)g_total / 1073741824.0);
    tree(src, dst, 0); say(T("\r  copiando: %.2f de %.2f GB   \n", "\r  copying: %.2f of %.2f GB   \n"), (double)g_copied / 1073741824.0, (double)g_total / 1073741824.0);
}
static void copy_file(const wchar_t *a, const wchar_t *b) { if (!CopyFileW(a, b, FALSE)) die(T("falha ao copiar um arquivo para o destino (erro %lu)", "failed to copy a file to the destination (error %lu)"), GetLastError()); }

// ---- conversa ----
static void ask(const char *q, wchar_t *out)
{
    say("%s\n> ", q);
    wchar_t ln[1024]; if (!fgetws(ln, 1024, stdin)) ln[0] = 0;
    wchar_t *s = ln; while (*s == L' ' || *s == L'"') s++;
    size_t l = wcslen(s); while (l && (s[l - 1] == L'\n' || s[l - 1] == L'\r' || s[l - 1] == L' ' || s[l - 1] == L'"' || s[l - 1] == L'\\')) s[--l] = 0;
    wcscpy(out, s);
}
static int confirm(const char *q)
{
    if (g_yes) return 1;
    wchar_t a[1024]; char t[600]; snprintf(t, sizeof t, "%s (s/n)", q); ask(t, a); return a[0] == L's' || a[0] == L'S';
}
static void sha_file(const wchar_t *p, char *hex) { size_t n; uint8_t *d = rd(p, &n); if (!d) { hex[0] = 0; return; } sha256(d, n, hex); free(d); }

#ifndef PREPARADOR_SEM_MAIN
int wmain(int argc, wchar_t **argv)
{
    SetConsoleOutputCP(CP_UTF8);
    wchar_t game[1024] = L"", ce[1024] = L"", out[1024] = L"", exe_arg[1024] = L"", self[1024], p[1024], q[1024]; int nodata = 0, ce_given = 0;
    for (int i = 1; i < argc; i++) {
        if (!wcscmp(argv[i], L"--jogo") && i + 1 < argc) wcscpy(game, argv[++i]);
        else if (!wcscmp(argv[i], L"--ce") && i + 1 < argc) { wcscpy(ce, argv[++i]); ce_given = 1; }
        else if (!wcscmp(argv[i], L"--saida") && i + 1 < argc) wcscpy(out, argv[++i]);
        else if (!wcscmp(argv[i], L"--exe") && i + 1 < argc) wcscpy(exe_arg, argv[++i]);
        else if (!wcscmp(argv[i], L"--sem-dados")) nodata = 1;
        else if (!wcscmp(argv[i], L"--sim")) g_yes = 1;
    }
    GetModuleFileNameW(NULL, self, 1024); { wchar_t *s = wcsrchr(self, L'\\'); if (s) *s = 0; }
    join(g_uam, self, L"ferramentas\\uam.exe");
    say("Preparador do UMvC3 no Switch\n=============================\n\n");
    if (!exists(g_uam)) die("não achei ferramentas\\uam.exe ao lado do Preparador. Extraia o pacote inteiro antes de rodar.");

    // 1. pasta do jogo
    if (!game[0]) { const wchar_t *def = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\ULTIMATE MARVEL VS. CAPCOM 3"; join(p, def, L"nativePCx64"); if (exists(p)) { wcscpy(game, def); say("Jogo encontrado em:\n  %ls\n\n", game); } }
    if (!game[0]) ask("Onde está o jogo? Arraste a pasta \"ULTIMATE MARVEL VS. CAPCOM 3\" para esta janela e aperte Enter.", game);
    join(p, game, L"nativePCx64\\sa\\DX9_64\\root.arc");
    if (!exists(p)) die("essa pasta não parece ser a do jogo (falta nativePCx64\\sa\\DX9_64\\root.arc).");

    // 2. executável sem a proteção
    say("[1/5] Conferindo o executável\n");
    wchar_t exe[1024]; char hex[80]; join(exe, game, L"umvc3.exe.unpacked.exe"); if (exe_arg[0]) wcscpy(exe, exe_arg);
    if (!exists(exe)) {
        join(p, game, L"umvc3.exe"); sha_file(p, hex);
        if (!strcmp(hex, EXE_STEAM_SHA)) die("o seu umvc3.exe é da versão certa, mas falta tirar a proteção dele.\nRode o Steamless no umvc3.exe (passo 2 do guia de instalação): ele cria umvc3.exe.unpacked.exe na pasta do jogo.\nDepois rode o Preparador de novo.");
        die("não achei umvc3.exe.unpacked.exe na pasta do jogo.\nRode o Steamless no umvc3.exe (passo 2 do guia de instalação) e rode o Preparador de novo.");
    }
    sha_file(exe, hex);
    if (strcmp(hex, EXE_SHA)) {
        say("  ATENÇÃO: o seu umvc3.exe.unpacked.exe é diferente do que este port espera.\n  O jogo pode não abrir. Causas comuns: versão do jogo desatualizada, ou outra versão do Steamless.\n");
        if (!confirm("  Continuar mesmo assim?")) die("interrompido a seu pedido.");
    } else say("  executável da versão suportada\n");

    // 3. Community Edition (opcional)
    int with_ce = 0;
    if (!ce_given && !g_yes) ask("\nPasta da Community Edition (\"UMvC3 Community Edition Build\"). Arraste-a para cá, ou só aperte Enter para o jogo original.", ce);
    if (ce[0]) {
        join(p, ce, L"Characters.ini"); join(q, ce, L"nativePCx64");
        if (!exists(p) || !exists(q)) die("essa pasta não parece ser a da Community Edition (falta Characters.ini ou nativePCx64).");
        with_ce = 1;
        for (int i = 0; i < 4; i++) { join(p, ce, CE[i].name); sha_file(p, hex);
            if (strcmp(hex, CE[i].sha)) { say("  A sua Community Edition é de uma versão diferente da suportada por este port (%ls).\n  Vou preparar só o jogo original.\n", CE[i].name); with_ce = 0; break; } }
        if (with_ce) say("  Community Edition da versão suportada\n");
    }

    // 4. destino
    if (!out[0]) ask("\nOnde gravar? Arraste o cartão SD (por exemplo E:) ou uma pasta do PC e aperte Enter.", out);
    if (!out[0]) die("nenhum destino informado.");
    wchar_t um[1024]; join(um, out, L"umvc3"); mkdirs(um);
    if (!exists(um)) die("não consegui criar a pasta umvc3 no destino.");

    // 5. shaders
    say("\n[2/5] Compilando os shaders do jogo para o Switch (alguns minutos)\n");
    { wchar_t t[512]; GetTempPathW(512, t); swprintf(g_tmp, 1024, L"%lsumvc3-prep-%lu", t, GetCurrentProcessId()); mkdirs(g_tmp); }
    join(p, game, L"nativePCx64\\sa\\DX9_64\\root.arc"); join(q, um, L"dksh.pak");
    shaders_build(p, q); RemoveDirectoryW(g_tmp);

    // 6. executável, aplicativos e ícones
    say("[3/5] Copiando o executável, os aplicativos e os ícones\n");
    join(q, um, L"umvc3.exe.unpacked.exe"); copy_file(exe, q);
    join(p, self, L"sdcard"); if (exists(p)) { g_copied = g_total = 0; tree(p, out, 1); tree(p, out, 0); } else say("  (pasta sdcard do pacote não encontrada ao lado do Preparador: copie os .nro para a pasta switch do cartão)\n");

    // 7. CE
    if (with_ce) {
        say("[4/5] Preparando os plugins da Community Edition\n");
        wchar_t cd[1024]; join(cd, um, L"ce"); mkdirs(cd);
        for (int i = 0; i < 4; i++) { join(p, ce, CE[i].name); join(q, cd, CE[i].name); ce_plugin(p, q, CE[i].base, i == 0); }
        join(cd, um, L"ce-data"); mkdirs(cd);
        join(p, ce, L"Characters.ini"); join(q, cd, L"Characters.ini"); copy_file(p, q);
        join(p, ce, L"ColorExpansion.ini"); join(q, cd, L"ColorExpansion.ini"); if (exists(p)) copy_file(p, q);
    } else say("[4/5] Community Edition: não pedida\n");

    // 8. dados
    if (nodata) say("[5/5] Dados do jogo: não copiados (--sem-dados)\n");
    else {
        say("[5/5] ");
        join(p, game, L"nativePCx64"); join(q, um, L"nativePCx64"); copy_tree("Copiando os dados do jogo", p, q);
        if (with_ce) { join(p, ce, L"nativePCx64"); join(q, um, L"ce-data\\nativePCx64"); copy_tree("      Copiando os dados da Community Edition", p, q); }
    }
    say("\nPronto. Em %ls:\n  switch\\   os aplicativos (umvc3.nro%s)\n  umvc3\\    os arquivos do jogo preparados\n", out, with_ce ? " e umvc3-ce.nro" : "");
    say("\nSe o destino não foi o cartão, copie essas duas pastas para a raiz do cartão SD.\nNo Switch: segure R ao abrir um jogo para entrar no Homebrew Menu e escolha \"(BASE) ULTIMATE MARVEL VS CAPCOM 3\"%s.\n\nPressione Enter para fechar.\n", with_ce ? " ou \"(CE) ULTIMATE MARVEL VS CAPCOM 3\"" : "");
    if (!g_yes) getwchar();
    return 0;
}
#endif
