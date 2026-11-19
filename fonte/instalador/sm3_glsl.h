// Tradutor de bytecode Direct3D 9 (vs_3_0 / ps_3_0) para GLSL 3.30 core.
// Usado pela camada D3D9 do port (Switch: mesa/nouveau; PC: validação com glslang).
//
// Convenções do GLSL gerado (a camada D3D9 precisa seguir):
//   VS: uniform vec4 vc[256]; ivec4 vi[16]; bool vb[16]; atributos "in vec4 a_<USO><n>" (ligar por nome);
//       saídas "out vec4 x_<USO><n>"; POSITION0 -> gl_Position com z' = 2z - w e
//       gl_Position.xy += dx_fixup.zw * gl_Position.w (meio pixel / inversão, como o wined3d).
//   PS: uniform vec4 pc[224]; ivec4 pi[16]; bool pb[16]; entradas "in vec4 x_<USO><n>";
//       samplers "uniform samplerXX ps_s<n>" (VS: "vs_s<n>"); saídas oC<n> em layout(location=n).
#ifndef SM3_GLSL_H
#define SM3_GLSL_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int is_ps;              // 1 = pixel shader
    int ok;                 // 1 = traduzido
    char err[160];          // motivo da falha
    uint32_t samplers;      // máscara de samplers usados
    uint32_t inputs_used;   // bits por registrador v#
    uint32_t in_mask;       // registradores de entrada v# declarados
    uint8_t in_usage[32], in_idx[32];   // semântica de cada v# declarado (D3DDECLUSAGE, índice)
    int n_const;            // constantes float que o shader lê do array uniforme (maior índice + 1; o array
                            // inteiro se há endereçamento relativo): tamanho declarado e quanto enviar
} sm3_info;

// Traduz `ntok` palavras de `tok` (começando no token de versão) para GLSL em `out` (capacidade `cap`).
// Retorna o tamanho escrito (sem o NUL) ou 0 em falha (motivo em info->err).
size_t sm3_to_glsl(const uint32_t *tok, size_t ntok, char *out, size_t cap, sm3_info *info);

// Alvo da tradução: 0 = GLSL 3.30 (OpenGL: uniformes soltos, ligação por nome); 1 = GLSL 4.60 para o compilador
// do deko3d (uam): uniformes num bloco std140 no binding 0 do estágio, samplers com binding = número do sampler,
// entradas do VS com location = número do registrador v#, variáveis VS->PS com location fixa por semântica
// (sm3_varying_location).
extern int sm3_target;
int sm3_varying_location(int usage, int idx);   // -1 = sem lugar

#endif
