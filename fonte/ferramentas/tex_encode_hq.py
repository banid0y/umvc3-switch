"""Codifica uma imagem BGRA crua (LARGURA x ALTURA) no DXT5 "BM_HQ" do jogo: bloco alfa = luminância Y, cor = (R = Cr,
G = opacidade, B = Cb), com croma neutro em 123 (medido no atlas original de botões).
Uso: python -I tools/tex_encode_hq.py ENTRADA.bgra LARGURA ALTURA SAIDA.bin
Para a troca de texturas por arquivo (fix82): SAIDA.bin vai em sdmc:/umvc3/texturas/<hash>.bin."""
import struct, sys

def enc_alpha(v):
    a0, a1 = max(v), min(v)
    if a0 == a1: return bytes([a0, a1, 0, 0, 0, 0, 0, 0])
    pal = [a0, a1] + [((7 - i) * a0 + i * a1) // 7 for i in range(1, 7)]
    bits = 0
    for j, x in enumerate(v):
        k = min(range(8), key=lambda q: abs(pal[q] - x)); bits |= k << (3 * j)
    return bytes([a0, a1]) + bits.to_bytes(6, 'little')

def q565(r, g, b): return ((r * 31 + 127) // 255 << 11) | ((g * 63 + 127) // 255 << 5) | ((b * 31 + 127) // 255)
def d565(v):
    r = (v >> 11) & 31; g = (v >> 5) & 63; b = v & 31
    return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))

def enc_color(px):   # px = 16 tuplas (cr, alfa, cb)
    # extremos = o par de pixels do bloco mais distantes entre si (a caixa min/max por canal criava cores que nao
    # existem no bloco: branco amarelado ao lado de vermelho, visto na barra do menu em 2026-11-19)
    u = list(set(px)); lo = hi = u[0]; best = -1
    for i in range(len(u)):
        for j in range(i + 1, len(u)):
            d = sum((u[i][c] - u[j][c]) ** 2 for c in range(3))
            if d > best: best = d; lo, hi = u[i], u[j]
    c0, c1 = q565(*hi), q565(*lo)
    if c0 < c1: c0, c1 = c1, c0
    p0, p1 = d565(c0), d565(c1)
    pal = [p0, p1, tuple((2 * a + b) // 3 for a, b in zip(p0, p1)), tuple((a + 2 * b) // 3 for a, b in zip(p0, p1))]
    bits = 0
    for j, p in enumerate(px):
        k = min(range(4), key=lambda q: sum((pal[q][c] - p[c]) ** 2 for c in range(3))); bits |= k << (2 * j)
    return struct.pack('<HHI', c0, c1, bits)

def encode(bgra, w, h):
    out = bytearray()
    for by in range(0, h, 4):
        for bx in range(0, w, 4):
            ys = []; cs = []
            for j in range(16):
                x = min(bx + (j & 3), w - 1); y = min(by + (j >> 2), h - 1); o = (y * w + x) * 4
                b, g, r, a = bgra[o], bgra[o + 1], bgra[o + 2], bgra[o + 3]
                if a == 0: yy, cb, cr = 0, 123, 123
                else:
                    yy = int(0.299 * r + 0.587 * g + 0.114 * b + 0.5)
                    cb = max(0, min(255, int(123 - 0.168736 * r - 0.331264 * g + 0.5 * b + 0.5)))
                    cr = max(0, min(255, int(123 + 0.5 * r - 0.418688 * g - 0.081312 * b + 0.5)))
                ys.append(yy); cs.append((cr, a, cb))
            out += enc_alpha(ys) + enc_color(cs)
    return bytes(out)

if __name__ == '__main__':
    src, w, h, dst = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    d = open(src, 'rb').read(); assert len(d) == w * h * 4
    o = encode(d, w, h); open(dst, 'wb').write(o); print(dst, len(o), 'bytes')
