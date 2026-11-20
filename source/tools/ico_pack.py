"""Empacota PNGs quadrados num .ico (entradas em PNG, aceitas pelo Windows Vista em diante).
Uso: python -I tools/ico_pack.py SAIDA.ico A.png [B.png ...]   (o lado de cada imagem vem do cabeçalho do PNG)"""
import struct, sys

out, ins = sys.argv[1], sys.argv[2:]
imgs = []
for p in ins:
    d = open(p, 'rb').read()
    assert d[:8] == bytes([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]), p + ': não é PNG'
    w, h = struct.unpack_from('>II', d, 16)
    assert w == h and w <= 256, p
    imgs.append((w, d))
head = struct.pack('<HHH', 0, 1, len(imgs))
off = 6 + 16 * len(imgs)
dirs = b''
for w, d in imgs:
    dirs += struct.pack('<BBBBHHII', w & 255, w & 255, 0, 0, 1, 32, len(d), off)
    off += len(d)
open(out, 'wb').write(head + dirs + b''.join(d for _, d in imgs))
print(out, len(imgs), 'imagens', off, 'bytes')
