"""Tabela de conferência dos shaders compilados, para o instalador (fix do "uam variou": 2026-11-19).
Lê um dksh.pak de referência (o testado no console) e grava, para cada shader, o FNV-1a de 64 bits do .dksh final
(depois da correção das palavras de sched) e o tamanho. O instalador compara cada shader que compila com esta tabela e
recompila o que sair diferente; no fim confere o SHA-256 do pacote inteiro.
Só há hashes aqui: nada do conteúdo dos shaders nem do jogo.
Uso: python -I tools/dksh_expected.py REFERENCIA.pak SAIDA.bin
Formato de SAIDA.bin: "DKEX", quantidade (u32), SHA-256 do pacote (32 bytes), depois entradas de 24 bytes em ordem de
(tipo, hash): hash do shader (u64), tipo (u32: 0 = vs, 1 = ps), tamanho (u32), FNV-1a 64 do conteúdo (u64)."""
import hashlib, struct, sys

src, dst = sys.argv[1], sys.argv[2]
d = open(src, 'rb').read()
magic, n, flags, _ = struct.unpack_from('<IIII', d, 0)
assert magic == 0x32504B44, 'não é um pacote DKP2'


def fnv(b):
    h = 0xcbf29ce484222325
    for x in b:
        h = ((h ^ x) * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    return h


out = [b'DKEX', struct.pack('<I', n), hashlib.sha256(d).digest()]
prev = None
for i in range(n):
    h, kind, off, size, _ = struct.unpack_from('<QIIII', d, 16 + 24 * i)
    assert prev is None or (kind, h) > prev, 'entradas fora de ordem'
    prev = (kind, h)
    out.append(struct.pack('<QIIQ', h, kind, size, fnv(d[off:off + size])))
open(dst, 'wb').write(b''.join(out))
print(dst, n, 'shaders;', 'SHA-256 do pacote', hashlib.sha256(d).hexdigest())
