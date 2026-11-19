"""Gera a troca PARCIAL (fix96) da barra da opção selecionada do menu principal (textura 512x64 BM_HQ, hash
c50d09005c484509): o disco do direcional do Xbox vira os quatro botões direcionais do Joy-Con, o da esquerda em
vermelho (como a seta vermelha do original). Nada do jogo entra no arquivo: a faixa regravada (52x52 pixels a partir
de (368, 8)) é desenhada aqui — barra branca com borda preta e os quatro botões.
Uso: python -I tools/dpad_bar_patch.py SAIDA.bin [PREVIA.bmp]
Formato "TXRP": magia, tamanho total do nível 0 da textura alvo, bloco x, bloco y, blocos de largura, blocos de altura
(4 x u16), depois os blocos DXT5 da faixa, linha a linha."""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tex_encode_hq, mt_tex

X0, Y0, W, H = 368, 8, 52, 52
CX, CY, OFF, RAD = 394.0, 32.0, 10.5, 6.6
SS = 4


def bar(y):   # (cinza, opacidade) da barra na linha y da textura
    if y == 12 or y == 55: return 0, 12
    if y == 13 or 50 <= y <= 54: return 0, 255
    if 14 <= y <= 49: return 255, 255
    return 0, 0


def main():
    out = bytearray(W * H * 4)
    btn = [(-OFF, 0, (18, 0, 230)), (OFF, 0, (45, 45, 45)), (0, -OFF, (45, 45, 45)), (0, OFF, (45, 45, 45))]   # BGR
    for y in range(H):
        for x in range(W):
            g, a = bar(Y0 + y)
            b_, g_, r_ = float(g), float(g), float(g)
            for dx, dy, col in btn:
                cov = 0
                for sy in range(SS):
                    for sx in range(SS):
                        px = X0 + x + (sx + 0.5) / SS - (CX + dx); py = Y0 + y + (sy + 0.5) / SS - (CY + dy)
                        if px * px + py * py <= RAD * RAD: cov += 1
                if cov:
                    t = cov / (SS * SS)
                    b_ = b_ * (1 - t) + col[0] * t; g_ = g_ * (1 - t) + col[1] * t; r_ = r_ * (1 - t) + col[2] * t
            o = (y * W + x) * 4
            out[o] = int(b_ + 0.5); out[o + 1] = int(g_ + 0.5); out[o + 2] = int(r_ + 0.5); out[o + 3] = a
    blocks = tex_encode_hq.encode(out, W, H)
    assert len(blocks) == (W // 4) * (H // 4) * 16
    hd = b'TXRP' + struct.pack('<IHHHH', 512 * 64, X0 // 4, Y0 // 4, W // 4, H // 4)
    open(sys.argv[1], 'wb').write(hd + bytes(blocks))
    if len(sys.argv) > 2: mt_tex.bmp_write(sys.argv[2], W, H, out)
    print('ok', len(hd) + len(blocks), 'bytes')


if __name__ == '__main__':
    main()
