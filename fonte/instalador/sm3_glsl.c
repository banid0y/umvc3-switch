// Tradutor D3D9 SM3 -> GLSL 3.30. Ver sm3_glsl.h para as convenções.
// Cobre os opcodes vistos no corpus do UMvC3 (35) e mais alguns comuns; o resto falha com mensagem.
#include "sm3_glsl.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

enum { R_TEMP = 0, R_INPUT = 1, R_CONST = 2, R_ADDR = 3, R_RASTOUT = 4, R_ATTROUT = 5, R_OUTPUT = 6,
       R_CONSTINT = 7, R_COLOROUT = 8, R_DEPTHOUT = 9, R_SAMPLER = 10, R_CONSTBOOL = 14, R_LOOP = 15,
       R_MISC = 17, R_LABEL = 18, R_PRED = 19 };

enum { OP_NOP = 0, OP_MOV, OP_ADD, OP_SUB, OP_MAD, OP_MUL, OP_RCP, OP_RSQ, OP_DP3, OP_DP4, OP_MIN, OP_MAX,
       OP_SLT, OP_SGE, OP_EXP, OP_LOG, OP_LIT, OP_DST, OP_LRP, OP_FRC, OP_M4X4, OP_M4X3, OP_M3X4, OP_M3X3,
       OP_M3X2, OP_CALL, OP_CALLNZ, OP_LOOP, OP_RET, OP_ENDLOOP, OP_LABEL, OP_DCL, OP_POW, OP_CRS, OP_SGN,
       OP_ABS, OP_NRM, OP_SINCOS, OP_REP, OP_ENDREP, OP_IF, OP_IFC, OP_ELSE, OP_ENDIF, OP_BREAK, OP_BREAKC,
       OP_MOVA, OP_DEFB, OP_DEFI,
       OP_TEXKILL = 65, OP_TEX = 66, OP_DEF = 81, OP_CMP = 88, OP_DP2ADD = 90, OP_DSX = 91, OP_DSY = 92,
       OP_TEXLDD = 93, OP_SETP = 94, OP_TEXLDL = 95, OP_BREAKP = 96, OP_PHASE = 0xFFFD, OP_COMMENT = 0xFFFE,
       OP_END = 0xFFFF };

static const char *USAGE[] = { "POSITION", "BLENDWEIGHT", "BLENDINDICES", "NORMAL", "PSIZE", "TEXCOORD",
                               "TANGENT", "BINORMAL", "TESSFACTOR", "POSITIONT", "COLOR", "FOG", "DEPTH",
                               "SAMPLE" };

typedef struct { char *p; size_t n, cap; int over; } buf;

static void put(buf *b, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    size_t room = b->cap > b->n ? b->cap - b->n : 0;
    int k = vsnprintf(b->p + b->n, room, fmt, ap);
    va_end(ap);
    if (k < 0 || (size_t)k >= room) { b->over = 1; b->n = b->cap ? b->cap - 1 : 0; }
    else b->n += (size_t)k;
}

#define MAXREG 256
typedef struct {
    const uint32_t *t; size_t n;
    int ps;
    sm3_info *info;
    // declarações
    int in_usage[32], in_idx[32], in_decl[32];
    int out_usage[16], out_idx[16], out_decl[16], out_written[16];
    int samp_type[16];
    int def_f[MAXREG]; uint32_t def_fv[MAXREG][4];
    int def_i[16]; int32_t def_iv[16][4];
    int def_b[16]; uint32_t def_bv[16];
    int temps;            // máx r# + 1
    int color_out;        // máscara oC#
    int depth_out, use_vpos, use_vface, use_a0;
    int max_const, rel_const;   // maior c# lido do array uniforme + 1; endereçamento relativo visto
    int rep_depth;
    char err[160];
} ctx;

static int fail(ctx *c, const char *fmt, ...)
{
    if (!c->err[0]) { va_list ap; va_start(ap, fmt); vsnprintf(c->err, sizeof c->err, fmt, ap); va_end(ap); }
    return 0;
}

static int rtype(uint32_t tok) { return (int)(((tok >> 28) & 7) | ((tok >> 8) & 0x18)); }
static int rnum(uint32_t tok) { return (int)(tok & 0x7FF); }

static const char *usage_name(int u) { return (u >= 0 && u < 14) ? USAGE[u] : "UNKNOWN"; }

int sm3_target;
// location das variáveis entre estágios no alvo deko3d: TEXCOORD0-15 -> 0-15, COLOR0-1 -> 16-17, FOG -> 18,
// NORMAL0-1 -> 19-20, TANGENT -> 21, BINORMAL -> 22, POSITION1-2 -> 23-24, BLENDWEIGHT -> 25, BLENDINDICES -> 26,
// DEPTH -> 27, PSIZE/outros -> sem lugar
int sm3_varying_location(int usage, int idx)
{
    switch (usage) {
    case 5: return idx < 16 ? idx : -1;
    case 10: return idx < 2 ? 16 + idx : -1;
    case 11: return idx == 0 ? 18 : -1;
    case 3: return idx < 2 ? 19 + idx : -1;
    case 6: return idx == 0 ? 21 : -1;
    case 7: return idx == 0 ? 22 : -1;
    case 0: return idx >= 1 && idx <= 2 ? 22 + idx : -1;
    case 1: return idx == 0 ? 25 : -1;
    case 2: return idx == 0 ? 26 : -1;
    case 12: return idx == 0 ? 27 : -1;
    }
    return -1;
}

// nome base (vec4) de um registrador-fonte, sem swizzle
static int src_base(ctx *c, uint32_t tok, uint32_t rel, char *o, size_t cap)
{
    int t = rtype(tok), n = rnum(tok);
    int relative = (tok >> 13) & 1;
    switch (t) {
    case R_TEMP:
        if (n + 1 > c->temps) c->temps = n + 1;
        snprintf(o, cap, "r%d", n); return 1;
    case R_INPUT:
        if (n >= 32) return fail(c, "v%d fora do limite", n);
        if (!c->in_decl[n]) return fail(c, "v%d sem dcl", n);
        c->info->inputs_used |= 1u << n;
        snprintf(o, cap, "%s_%s%d", c->ps ? "x" : "a", usage_name(c->in_usage[n]), c->in_idx[n]);
        return 1;
    case R_CONST:
        if (relative) {
            int at = rtype(rel), comp = (rel >> 16) & 3;
            if (at == R_ADDR && !c->ps) { c->use_a0 = 1; c->rel_const = 1; snprintf(o, cap, "vc[dx_a0.%c + %d]", "xyzw"[comp], n); return 1; }
            return fail(c, "endereçamento relativo tipo %d", at);
        }
        if (n < MAXREG && c->def_f[n]) snprintf(o, cap, "lc%d", n);
        else { snprintf(o, cap, "%s[%d]", c->ps ? "pc" : "vc", n); if (n + 1 > c->max_const) c->max_const = n + 1; }
        return 1;
    case R_CONSTINT:
        if (n < 16 && c->def_i[n]) snprintf(o, cap, "vec4(li%d)", n);
        else snprintf(o, cap, "vec4(%s[%d])", c->ps ? "pi" : "vi", n);
        return 1;
    case R_MISC:
        if (!c->ps) return fail(c, "misctype no VS");
        if (n == 0) { c->use_vpos = 1; snprintf(o, cap, "dx_vpos"); return 1; }
        if (n == 1) { c->use_vface = 1; snprintf(o, cap, sm3_target ? "vec4(pi[14].x != 0 ? float(pi[14].x) : (gl_FrontFacing ? 1.0 : -1.0))" : "vec4(gl_FrontFacing ? 1.0 : -1.0)"); return 1; }   // fix65: no deko3d o host informa a face em pi[14].x
        return fail(c, "misctype %d", n);
    case R_ADDR:
        if (c->ps) return fail(c, "registrador t# em ps_3_0");
        c->use_a0 = 1; snprintf(o, cap, "vec4(dx_a0)"); return 1;
    default:
        return fail(c, "registrador-fonte tipo %d", t);
    }
}

// expressão completa de uma fonte (swizzle + modificador)
static int src_expr(ctx *c, uint32_t tok, uint32_t rel, char *o, size_t cap)
{
    char base[96], sw[8] = "";
    if (!src_base(c, tok, rel, base, sizeof base)) return 0;
    uint32_t s = (tok >> 16) & 0xFF;
    if (s != 0xE4) {
        sw[0] = '.';
        for (int i = 0; i < 4; i++) sw[1 + i] = "xyzw"[(s >> (2 * i)) & 3];
        sw[5] = 0;
    }
    char v[128];
    snprintf(v, sizeof v, "%s%s", base, sw);
    switch ((tok >> 24) & 0xF) {
    case 0: snprintf(o, cap, "%s", v); break;
    case 1: snprintf(o, cap, "(-%s)", v); break;
    case 2: snprintf(o, cap, "(%s - 0.5)", v); break;
    case 3: snprintf(o, cap, "(0.5 - %s)", v); break;
    case 4: snprintf(o, cap, "(2.0 * %s - 1.0)", v); break;
    case 5: snprintf(o, cap, "(1.0 - 2.0 * %s)", v); break;
    case 6: snprintf(o, cap, "(1.0 - %s)", v); break;
    case 7: snprintf(o, cap, "(2.0 * %s)", v); break;
    case 8: snprintf(o, cap, "(-2.0 * %s)", v); break;
    case 11: snprintf(o, cap, "abs(%s)", v); break;
    case 12: snprintf(o, cap, "(-abs(%s))", v); break;
    default: return fail(c, "modificador de fonte %u", (tok >> 24) & 0xF);
    }
    return 1;
}

// lê operandos: o[i] = token, r[i] = token relativo (ou 0); retorna nº de tokens consumidos
static int read_ops(ctx *c, size_t at, size_t end, int nops, uint32_t *o, uint32_t *r)
{
    size_t p = at;
    for (int i = 0; i < nops; i++) {
        if (p >= end) return fail(c, "instrução curta em %zu", at), -1;
        o[i] = c->t[p++]; r[i] = 0;
        if ((o[i] >> 13) & 1) {
            if (p >= end) return fail(c, "token relativo ausente"), -1;
            r[i] = c->t[p++];
        }
    }
    return (int)(p - at);
}

// destino: monta "nome" e "máscara"
static int dst_name(ctx *c, uint32_t tok, char *o, size_t cap, int *is_depth)
{
    int t = rtype(tok), n = rnum(tok);
    *is_depth = 0;
    switch (t) {
    case R_TEMP:
        if (n + 1 > c->temps) c->temps = n + 1;
        snprintf(o, cap, "r%d", n); return 1;
    case R_OUTPUT:
        if (c->ps) return fail(c, "o# em ps");
        if (n >= 16) return fail(c, "o%d", n);
        c->out_written[n] = 1;
        snprintf(o, cap, "dx_o%d", n); return 1;
    case R_COLOROUT:
        if (!c->ps || n >= 4) return fail(c, "oC%d", n);
        c->color_out |= 1 << n;
        snprintf(o, cap, "oC%d", n); return 1;
    case R_DEPTHOUT:
        c->depth_out = 1; *is_depth = 1;
        snprintf(o, cap, "gl_FragDepth"); return 1;
    case R_ADDR:
        if (c->ps) return fail(c, "a0 em ps");
        c->use_a0 = 1; snprintf(o, cap, "dx_a0"); return 1;
    default:
        return fail(c, "destino tipo %d", t);
    }
}

static void mask_str(uint32_t tok, char *m)
{
    int k = 0;
    for (int i = 0; i < 4; i++) if (tok & (1u << (16 + i))) m[k++] = "xyzw"[i];
    m[k] = 0;
}

static int emit_assign(ctx *c, buf *b, uint32_t dtok, const char *expr)
{
    char name[48]; int depth;
    if (!dst_name(c, dtok, name, sizeof name, &depth)) return 0;
    if (dtok & (1u << 13)) return fail(c, "destino relativo");
    int sat = (dtok >> 20) & 1;
    if ((dtok >> 24) & 0xF) return fail(c, "shift de destino");
    char e[2300];
    if (sat) snprintf(e, sizeof e, "clamp(%s, 0.0, 1.0)", expr); else snprintf(e, sizeof e, "%s", expr);
    if (depth) { put(b, "    gl_FragDepth = (%s).x;\n", e); return 1; }
    if (rtype(dtok) == R_ADDR) {
        char m[5]; mask_str(dtok, m);
        put(b, "    dx_a0.%s = ivec4(floor((%s) + 0.5)).%s;\n", m, e, m);
        return 1;
    }
    char m[5]; mask_str(dtok, m);
    if (!strcmp(m, "xyzw")) put(b, "    %s = %s;\n", name, e);
    else put(b, "    %s.%s = (%s).%s;\n", name, m, e, m);
    return 1;
}

static const char *cmp_op(int ctl)
{
    switch (ctl) { case 1: return ">"; case 2: return "=="; case 3: return ">="; case 4: return "<";
                   case 5: return "!="; case 6: return "<="; default: return NULL; }
}

static const char *samp_glsl(int ty)
{
    switch (ty) { case 2: return "sampler2D"; case 3: return "samplerCube"; case 4: return "sampler3D"; default: return NULL; }
}

// fix31: 1 = só nos shaders de vértice (padrão: é onde o defeito foi provado, e textureSize num shader de pixel
// custa uma operação de textura por pixel na GPU do console); 2 = em todos; 0 = traduções como antes.
int sm3_texel_eps = 1;
static int tex_coord(ctx *c, int s, const char *src, char *o, size_t cap, int proj)
{
    int ty = c->samp_type[s];
    if (ty == 2) {
        if (proj > 0) snprintf(o, cap, "vec3((%s).xy, (%s).w)", src, src);
        // fix31 (alvo deko3d): 1/64 de texel a mais em toda busca 2D. O jogo busca dados (ossos, tabelas) com filtro
        // POINT em coordenadas exatamente na divisa entre texels; o PC resolve para o texel k, a GPU do Switch às
        // vezes para o vizinho (provado no console com a fix30). 1/64 de texel não muda uma busca filtrada.
        else if (sm3_target && proj == 0 && (sm3_texel_eps == 2 || (sm3_texel_eps == 1 && !c->ps))) snprintf(o, cap, "((%s).xy + dx_e%d)", src, s);
        else snprintf(o, cap, "(%s).xy", src);
    } else if (ty == 3 || ty == 4) {
        if (proj > 0) snprintf(o, cap, "(%s)", src);
        else snprintf(o, cap, "(%s).xyz", src);
    } else return fail(c, "sampler s%d sem dcl", s);
    return 1;
}

static int pass_decl(ctx *c)
{
    size_t i = 1;
    while (i < c->n) {
        uint32_t x = c->t[i];
        int op = (int)(x & 0xFFFF);
        if (op == OP_END) return 1;
        if (op == OP_COMMENT) { i += 1 + ((x >> 16) & 0x7FFF); continue; }
        size_t len = (x >> 24) & 0xF;
        if (i + 1 + len > c->n) return fail(c, "tamanho de instrução inválido");
        const uint32_t *a = c->t + i + 1;
        if (op == OP_DCL) {
            uint32_t u = a[0], d = a[1];
            int t = rtype(d), n = rnum(d);
            if (t == R_SAMPLER) { if (n < 16) c->samp_type[n] = (int)((u >> 27) & 0xF); }
            else if (t == R_INPUT) { if (n < 32) { c->in_decl[n] = 1; c->in_usage[n] = (int)(u & 0x1F); c->in_idx[n] = (int)((u >> 16) & 0xF); } }
            else if (t == R_OUTPUT) { if (n < 16) { c->out_decl[n] = 1; c->out_usage[n] = (int)(u & 0x1F); c->out_idx[n] = (int)((u >> 16) & 0xF); } }
            else if (t == R_MISC) { /* vPos/vFace */ }
            else return fail(c, "dcl de tipo %d", t);
        } else if (op == OP_DEF) {
            int n = rnum(a[0]);
            if (n < MAXREG) { c->def_f[n] = 1; memcpy(c->def_fv[n], a + 1, 16); }
        } else if (op == OP_DEFI) {
            int n = rnum(a[0]);
            if (n < 16) { c->def_i[n] = 1; memcpy(c->def_iv[n], a + 1, 16); }
        } else if (op == OP_DEFB) {
            int n = rnum(a[0]);
            if (n < 16) { c->def_b[n] = 1; c->def_bv[n] = a[1]; }
        }
        i += 1 + len;
    }
    return fail(c, "sem token de fim");
}

static int pass_body(ctx *c, buf *b)
{
    size_t i = 1;
    int depth = 1;
    while (i < c->n) {
        uint32_t x = c->t[i];
        int op = (int)(x & 0xFFFF);
        if (op == OP_END) break;
        if (op == OP_COMMENT) { i += 1 + ((x >> 16) & 0x7FFF); continue; }
        size_t len = (x >> 24) & 0xF;
        size_t at = i + 1, end = at + len;
        int ctl = (int)((x >> 16) & 0xFF);
        if (x & (1u << 28)) return fail(c, "instrução predicada (op %d)", op);
        uint32_t o[6], r[6];
        char s0[256], s1[256], s2[256], e[2100];
        int nsrc = -1;   // nº de fontes para ALU com destino
        switch (op) {
        case OP_NOP: case OP_DCL: case OP_DEF: case OP_DEFI: case OP_DEFB: break;
        case OP_MOV: case OP_RCP: case OP_RSQ: case OP_EXP: case OP_LOG: case OP_FRC: case OP_NRM: case OP_ABS:
        case OP_DSX: case OP_DSY: case OP_SINCOS: case OP_MOVA: nsrc = 1; break;
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_MIN: case OP_MAX: case OP_SLT: case OP_SGE: case OP_DP3:
        case OP_DP4: case OP_POW: case OP_TEX: case OP_TEXLDL: nsrc = 2; break;
        case OP_MAD: case OP_LRP: case OP_CMP: case OP_DP2ADD: nsrc = 3; break;
        case OP_TEXLDD: nsrc = 4; break;
        case OP_TEXKILL: {
            if (read_ops(c, at, end, 1, o, r) < 0) return 0;
            char nm[48]; int dd;
            int t = rtype(o[0]);
            if (t == R_TEMP) { if (!dst_name(c, o[0], nm, sizeof nm, &dd)) return 0; }
            else if (t == R_INPUT) { if (!src_base(c, o[0], 0, nm, sizeof nm)) return 0; }
            else return fail(c, "texkill tipo %d", t);
            char m[5]; mask_str(o[0], m);
            if (!m[0]) strcpy(m, "xyz");
            put(b, "%*sif (any(lessThan(%s.%s, %s(0.0)))) discard;\n", depth * 4, "", nm, m,
                strlen(m) == 1 ? "float" : (strlen(m) == 2 ? "vec2" : (strlen(m) == 3 ? "vec3" : "vec4")));
            break;
        }
        case OP_IFC: case OP_BREAKC: {
            if (read_ops(c, at, end, 2, o, r) < 0) return 0;
            if (!src_expr(c, o[0], r[0], s0, sizeof s0) || !src_expr(c, o[1], r[1], s1, sizeof s1)) return 0;
            const char *cm = cmp_op(ctl);
            if (!cm) return fail(c, "comparação %d", ctl);
            if (op == OP_IFC) { put(b, "%*sif ((%s).x %s (%s).x) {\n", depth * 4, "", s0, cm, s1); depth++; }
            else put(b, "%*sif ((%s).x %s (%s).x) break;\n", depth * 4, "", s0, cm, s1);
            break;
        }
        case OP_IF: {
            if (read_ops(c, at, end, 1, o, r) < 0) return 0;
            int t = rtype(o[0]), n = rnum(o[0]);
            if (t != R_CONSTBOOL) return fail(c, "if com tipo %d", t);
            if (n < 16 && c->def_b[n]) put(b, "%*sif (%s) {\n", depth * 4, "", c->def_bv[n] ? "true" : "false");
            else if (sm3_target) put(b, "%*sif (%s_[%d].x != 0) {\n", depth * 4, "", c->ps ? "pb" : "vb", n);
            else put(b, "%*sif (%s[%d]) {\n", depth * 4, "", c->ps ? "pb" : "vb", n);
            depth++;
            break;
        }
        case OP_ELSE: put(b, "%*s} else {\n", (depth - 1) * 4, ""); break;
        case OP_ENDIF: case OP_ENDREP:
            if (op == OP_ENDREP) c->rep_depth--;
            depth--; put(b, "%*s}\n", depth * 4, ""); break;
        case OP_BREAK: put(b, "%*sbreak;\n", depth * 4, ""); break;
        case OP_REP: {
            if (read_ops(c, at, end, 1, o, r) < 0) return 0;
            int t = rtype(o[0]), n = rnum(o[0]);
            if (t != R_CONSTINT) return fail(c, "rep com tipo %d", t);
            char cnt[48];
            if (n < 16 && c->def_i[n]) snprintf(cnt, sizeof cnt, "li%d.x", n);
            else snprintf(cnt, sizeof cnt, "%s[%d].x", c->ps ? "pi" : "vi", n);
            int k = c->rep_depth++;
            put(b, "%*sfor (int rep%d = 0; rep%d < %s; rep%d++) {\n", depth * 4, "", k, k, cnt, k);
            depth++;
            break;
        }
        default:
            return fail(c, "opcode %d não suportado", op);
        }
        if (nsrc >= 0) {
            if (read_ops(c, at, end, 1 + nsrc, o, r) < 0) return 0;
            if (nsrc >= 1 && op != OP_TEX && op != OP_TEXLDL && op != OP_TEXLDD && !src_expr(c, o[1], r[1], s0, sizeof s0)) return 0;
            if (nsrc >= 2 && op != OP_TEX && op != OP_TEXLDL && op != OP_TEXLDD && !src_expr(c, o[2], r[2], s1, sizeof s1)) return 0;
            if (nsrc >= 3 && op != OP_TEXLDD && !src_expr(c, o[3], r[3], s2, sizeof s2)) return 0;
            switch (op) {
            case OP_MOV: snprintf(e, sizeof e, "%s", s0); break;
            case OP_MOVA: snprintf(e, sizeof e, "%s", s0); break;
            case OP_ADD: snprintf(e, sizeof e, "(%s + %s)", s0, s1); break;
            case OP_SUB: snprintf(e, sizeof e, "(%s - %s)", s0, s1); break;
            case OP_MUL: snprintf(e, sizeof e, "(%s * %s)", s0, s1); break;
            case OP_MAD: snprintf(e, sizeof e, "(%s * %s + %s)", s0, s1, s2); break;
            case OP_MIN: snprintf(e, sizeof e, "min(%s, %s)", s0, s1); break;
            case OP_MAX: snprintf(e, sizeof e, "max(%s, %s)", s0, s1); break;
            case OP_SLT: snprintf(e, sizeof e, "vec4(lessThan(%s, %s))", s0, s1); break;
            case OP_SGE: snprintf(e, sizeof e, "vec4(greaterThanEqual(%s, %s))", s0, s1); break;
            case OP_DP3: snprintf(e, sizeof e, "vec4(dot((%s).xyz, (%s).xyz))", s0, s1); break;
            case OP_DP4: snprintf(e, sizeof e, "vec4(dot(%s, %s))", s0, s1); break;
            case OP_RCP: snprintf(e, sizeof e, "vec4(1.0 / (%s).x)", s0); break;
            case OP_RSQ: snprintf(e, sizeof e, "vec4(inversesqrt(abs((%s).x)))", s0); break;
            case OP_EXP: snprintf(e, sizeof e, "vec4(exp2((%s).x))", s0); break;
            case OP_LOG: snprintf(e, sizeof e, "vec4(log2(abs((%s).x)))", s0); break;
            case OP_POW: snprintf(e, sizeof e, "vec4(pow(abs((%s).x), (%s).x))", s0, s1); break;
            case OP_FRC: snprintf(e, sizeof e, "fract(%s)", s0); break;
            case OP_ABS: snprintf(e, sizeof e, "abs(%s)", s0); break;
            case OP_NRM: snprintf(e, sizeof e, "(%s * inversesqrt(dot((%s).xyz, (%s).xyz)))", s0, s0, s0); break;
            case OP_LRP: snprintf(e, sizeof e, "mix(%s, %s, %s)", s2, s1, s0); break;
            case OP_CMP: snprintf(e, sizeof e, "dx_cmp(%s, %s, %s)", s0, s1, s2); break;
            case OP_DP2ADD: snprintf(e, sizeof e, "vec4(dot((%s).xy, (%s).xy) + (%s).x)", s0, s1, s2); break;
            case OP_DSX: snprintf(e, sizeof e, "dFdx(%s)", s0); break;
            case OP_DSY: snprintf(e, sizeof e, "dFdy(%s)", s0); break;
            case OP_SINCOS: snprintf(e, sizeof e, "vec4(cos((%s).x), sin((%s).x), 0.0, 0.0)", s0, s0); break;
            case OP_TEX: case OP_TEXLDL: case OP_TEXLDD: {
                if (rtype(o[2]) != R_SAMPLER) return fail(c, "texld sem sampler");
                int s = rnum(o[2]);
                if (s >= 16) return fail(c, "s%d", s);
                c->info->samplers |= 1u << s;
                if (!src_expr(c, o[1], r[1], s0, sizeof s0)) return 0;
                int proj = op == OP_TEX && ctl == 1, bias = op == OP_TEX && ctl == 2;
                char co[600];
                if (!tex_coord(c, s, s0, co, sizeof co, proj)) return 0;
                const char *sn = c->ps ? "ps_s" : "vs_s";
                char t[2048];
                if (op == OP_TEXLDL) snprintf(t, sizeof t, "textureLod(%s%d, %s, (%s).w)", sn, s, co, s0);
                else if (op == OP_TEXLDD) {
                    char g1[256], g2[256], d1[600], d2[600];
                    if (!src_expr(c, o[3], r[3], g1, sizeof g1) || !src_expr(c, o[4], r[4], g2, sizeof g2)) return 0;
                    if (!tex_coord(c, s, g1, d1, sizeof d1, -1) || !tex_coord(c, s, g2, d2, sizeof d2, -1)) return 0;   // gradientes: sem o ajuste
                    snprintf(t, sizeof t, "textureGrad(%s%d, %s, %s, %s)", sn, s, co, d1, d2);
                } else if (proj) snprintf(t, sizeof t, "textureProj(%s%d, %s)", sn, s, co);
                else if (bias) snprintf(t, sizeof t, "texture(%s%d, %s, (%s).w)", sn, s, co, s0);
                else snprintf(t, sizeof t, "texture(%s%d, %s)", sn, s, co);
                uint32_t sw = (o[2] >> 16) & 0xFF;
                if (sw != 0xE4) {
                    char swz[6] = "."; for (int k = 0; k < 4; k++) swz[1 + k] = "xyzw"[(sw >> (2 * k)) & 3];
                    swz[5] = 0;
                    snprintf(e, sizeof e, "(%s)%s", t, swz);
                } else snprintf(e, sizeof e, "%s", t);
                break;
            }
            default: return fail(c, "opcode %d", op);
            }
            put(b, "%*s", depth * 4 - 4, "");
            if (!emit_assign(c, b, o[0], e)) return 0;
        }
        if (c->err[0]) return 0;
        i = end;
    }
    if (depth != 1) return fail(c, "blocos desbalanceados");
    return 1;
}

static void put_float4(buf *b, const uint32_t *v)
{
    put(b, "uintBitsToFloat(uvec4(0x%08xu, 0x%08xu, 0x%08xu, 0x%08xu))", v[0], v[1], v[2], v[3]);
}

size_t sm3_to_glsl(const uint32_t *tok, size_t ntok, char *out, size_t cap, sm3_info *info)
{
    static ctx c;
    memset(&c, 0, sizeof c);
    memset(info, 0, sizeof *info);
    c.t = tok; c.n = ntok; c.info = info;
    if (ntok < 2) { snprintf(info->err, sizeof info->err, "curto"); return 0; }
    uint32_t ver = tok[0];
    if ((ver & 0xFFFF0000u) == 0xFFFF0000u) c.ps = 1;
    else if ((ver & 0xFFFF0000u) != 0xFFFE0000u) { snprintf(info->err, sizeof info->err, "versão %08x", ver); return 0; }
    if ((ver & 0xFF00) != 0x0300) { snprintf(info->err, sizeof info->err, "só SM3 (versão %08x)", ver); return 0; }
    info->is_ps = c.ps;

    static char body_mem[1 << 20];
    buf body = { body_mem, 0, sizeof body_mem, 0 };
    if (!pass_decl(&c) || !pass_body(&c, &body)) {
        snprintf(info->err, sizeof info->err, "%s", c.err[0] ? c.err : "erro");
        return 0;
    }
    if (body.over) { snprintf(info->err, sizeof info->err, "corpo grande demais"); return 0; }

    buf b = { out, 0, cap, 0 };
    put(&b, "#version %s\n// traduzido de %s_3_0 pelo sm3_glsl\n", sm3_target ? "460" : "330 core", c.ps ? "ps" : "vs");
    {   // array de constantes do tamanho que o shader usa (menos dados por draw no driver)
        int full = c.ps ? 224 : 256, nc = c.rel_const ? full : c.max_const;
        if (nc < 1) nc = 1;
        if (nc > full) nc = full;
        info->n_const = nc;
        if (sm3_target) {   // bloco std140: fixos primeiro (deslocamentos iguais em todo shader), constantes no fim
            if (c.ps) put(&b, "layout(std140, binding = 0) uniform PSU {\n    vec4 dx_vposfix;\n    ivec4 pi[16];\n    ivec4 pb_[16];\n    vec4 pc[%d];\n};\n#define pb(n) (pb_[n].x != 0)\n", nc);
            else put(&b, "layout(std140, binding = 0) uniform VSU {\n    vec4 dx_fixup;\n    ivec4 vi[16];\n    ivec4 vb_[16];\n    vec4 vc[%d];\n};\n#define vb(n) (vb_[n].x != 0)\n", nc);
        }
        else if (c.ps) put(&b, "uniform vec4 pc[%d];\nuniform ivec4 pi[16];\nuniform bool pb[16];\n", nc);
        else put(&b, "uniform vec4 vc[%d];\nuniform ivec4 vi[16];\nuniform bool vb[16];\nuniform vec4 dx_fixup;\n", nc);
    }
    put(&b, "vec4 dx_cmp(vec4 a, vec4 b, vec4 c) {\n"
            "    return vec4(a.x >= 0.0 ? b.x : c.x, a.y >= 0.0 ? b.y : c.y, a.z >= 0.0 ? b.z : c.z, a.w >= 0.0 ? b.w : c.w);\n}\n");
    for (int s = 0; s < 16; s++) {
        if (!(info->samplers & (1u << s))) continue;
        const char *ty = samp_glsl(c.samp_type[s]);
        if (!ty) { snprintf(info->err, sizeof info->err, "tipo de sampler %d", c.samp_type[s]); return 0; }
        if (sm3_target) put(&b, "layout(binding = %d) uniform %s %s%d;\n", s, ty, c.ps ? "ps_s" : "vs_s", s);
        else put(&b, "uniform %s %s%d;\n", ty, c.ps ? "ps_s" : "vs_s", s);
    }
    for (int n = 0; n < 32; n++) {
        if (!c.in_decl[n]) continue;
        if (sm3_target) {
            int loc = c.ps ? sm3_varying_location(c.in_usage[n], c.in_idx[n]) : n;
            if (loc < 0) { snprintf(info->err, sizeof info->err, "entrada %s%d sem location", usage_name(c.in_usage[n]), c.in_idx[n]); return 0; }
            put(&b, "layout(location = %d) in vec4 %s_%s%d;\n", loc, c.ps ? "x" : "a", usage_name(c.in_usage[n]), c.in_idx[n]);
        }
        else put(&b, "in vec4 %s_%s%d;\n", c.ps ? "x" : "a", usage_name(c.in_usage[n]), c.in_idx[n]);
    }
    if (c.ps) {
        if (c.use_vpos && !sm3_target) put(&b, "uniform vec4 dx_vposfix;\n");
        for (int n = 0; n < 4; n++)
            if (c.color_out & (1 << n)) put(&b, "layout(location = %d) out vec4 oC%d;\n", n, n);
    } else {
        for (int n = 0; n < 16; n++) {
            if (!c.out_decl[n]) continue;
            int u = c.out_usage[n];
            if ((u == 0 && c.out_idx[n] == 0) || u == 4) continue;  // POSITION0 / PSIZE
            if (sm3_target) {
                int loc = sm3_varying_location(u, c.out_idx[n]);
                if (loc < 0) { snprintf(info->err, sizeof info->err, "saída %s%d sem location", usage_name(u), c.out_idx[n]); return 0; }
                put(&b, "layout(location = %d) out vec4 x_%s%d;\n", loc, usage_name(u), c.out_idx[n]);
            }
            else put(&b, "out vec4 x_%s%d;\n", usage_name(u), c.out_idx[n]);
        }
    }
    // fix54: posicao invariante nos VS do alvo deko3d. O jogo desenha o mesmo objeto em passes diferentes com VS
    // diferentes (pre-passe de profundidade x passe final) e conta com a mesma profundidade bit a bit. Sem "invariant" o
    // uam funde mul+add de um jeito em cada shader: no console (20261117s/t) o cabelo da Felicia e o rosto da Chun-Li
    // saiam com triangulos em degraus disputando a profundidade com o passe anterior.
    if (!c.ps && sm3_target) put(&b, "invariant gl_Position;\n");
    put(&b, "void main() {\n");
    if (sm3_target && (sm3_texel_eps == 2 || (sm3_texel_eps == 1 && !c.ps))) for (int n = 0; n < 16; n++) if (c.samp_type[n] == 2)
        put(&b, "    vec2 dx_e%d = vec2(0.015625) / vec2(textureSize(%s%d, 0));\n", n, c.ps ? "ps_s" : "vs_s", n);
    for (int n = 0; n < c.temps; n++) put(&b, "    vec4 r%d = vec4(0.0);\n", n);
    for (int n = 0; n < MAXREG; n++) if (c.def_f[n]) { put(&b, "    vec4 lc%d = ", n); put_float4(&b, c.def_fv[n]); put(&b, ";\n"); }
    for (int n = 0; n < 16; n++) if (c.def_i[n])
        put(&b, "    ivec4 li%d = ivec4(%d, %d, %d, %d);\n", n, c.def_iv[n][0], c.def_iv[n][1], c.def_iv[n][2], c.def_iv[n][3]);
    if (c.use_a0) put(&b, "    ivec4 dx_a0 = ivec4(0);\n");
    if (c.ps && c.use_vpos) put(&b, "    vec4 dx_vpos = vec4(gl_FragCoord.xy * dx_vposfix.xy + dx_vposfix.zw, 0.0, 0.0);\n");
    if (!c.ps) for (int n = 0; n < 16; n++) if (c.out_decl[n] || c.out_written[n]) put(&b, "    vec4 dx_o%d = vec4(0.0);\n", n);
    if (c.ps) for (int n = 0; n < 4; n++) if (c.color_out & (1 << n)) put(&b, "    oC%d = vec4(0.0);\n", n);
    put(&b, "%.*s", (int)body.n, body.p);
    if (!c.ps) {
        for (int n = 0; n < 16; n++) {
            if (!c.out_decl[n]) continue;
            int u = c.out_usage[n];
            if (u == 0 && c.out_idx[n] == 0) {
                // fix33: alvo 2 = deko3d com a faixa de profundidade do Direct3D (0 a 1) no dispositivo
                // (DkDeviceFlags_DepthZeroToOne): o z sai como o jogo calculou. No alvo 1 (faixa -1 a 1) a conversão
                // z*2 - w punha os polígonos de z = 0 ou z = w exatamente no plano de corte e perdia precisão.
                put(&b, "    gl_Position = dx_o%d;\n", n);
                if (sm3_target != 2) put(&b, "    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;\n");
                put(&b, "    gl_Position.y *= dx_fixup.y;\n"
                        "    gl_Position.xy += dx_fixup.zw * gl_Position.w;\n");
            } else if (u == 4) put(&b, "    gl_PointSize = dx_o%d.x;\n", n);
            else put(&b, "    x_%s%d = dx_o%d;\n", usage_name(u), c.out_idx[n], n);
        }
    }
    put(&b, "}\n");
    if (b.over) { snprintf(info->err, sizeof info->err, "saída maior que %zu", cap); return 0; }
    info->in_mask = 0;
    for (int n = 0; n < 32; n++) if (c.in_decl[n]) { info->in_mask |= 1u << n; info->in_usage[n] = (uint8_t)c.in_usage[n]; info->in_idx[n] = (uint8_t)c.in_idx[n]; }
    info->ok = 1;
    return b.n;
}
