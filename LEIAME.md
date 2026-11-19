# UMvC3 no Switch

Port não oficial de **Ultimate Marvel vs. Capcom 3** (versão de PC, Steam) para Nintendo Switch com Atmosphère.

**Você precisa da sua própria cópia do jogo na Steam.** Os dados do jogo não estão aqui: o instalador monta tudo
o que o Switch precisa a partir da sua instalação.

Versão 1.0 (versão de teste). São dois aplicativos, que podem ficar juntos no cartão:

| Nome no Homebrew Menu | Arquivo | O que é |
|---|---|---|
| (BASE) ULTIMATE MARVEL VS CAPCOM 3 | `umvc3.nro` | O jogo original |
| (CE) ULTIMATE MARVEL VS CAPCOM 3 | `umvc3-ce.nro` | O jogo com a Community Edition **1.2.3b** (precisa também da sua cópia dessa CE) |

## O que você precisa

- Um Switch com **Atmosphère** e o **Homebrew Menu**.
- Ultimate Marvel vs. Capcom 3 instalado pela **Steam** num PC com Windows 10 ou 11.
- Espaço no cartão SD: cerca de **4 GB** para o jogo original, **13 GB** com a Community Edition.
- Para a versão com CE: a pasta "UMvC3 Community Edition Build", versão **1.2.3b** (a única suportada).

## Como instalar

1. Baixe este repositório (**Code → Download ZIP**) e extraia.
2. Abra **`Install.exe`** (mantenha a pasta `aplicativos` ao lado dele). Ele abre em inglês; a caixa no canto
   superior direito troca para português.
3. Confira a pasta do jogo, marque a Community Edition se quiser, escolha o destino (o cartão SD ou uma pasta do
   PC) e clique em **Instalar**.
4. Se não instalou direto no cartão, copie as pastas `switch` e `umvc3` para a raiz do cartão SD.
5. No Switch, **segure R ao abrir um jogo qualquer** para entrar no Homebrew Menu e escolha o aplicativo.

Guia completo: [docs/INSTALAR.md](docs/INSTALAR.md) · Controles: [docs/CONTROLES.md](docs/CONTROLES.md) ·
Opções: [docs/OPCOES.md](docs/OPCOES.md) · Problemas: [docs/PROBLEMAS.md](docs/PROBLEMAS.md)

## Bom saber

- O `Install.exe` não tem assinatura digital, então o Windows mostra o aviso de "editor desconhecido": escolha
  "Mais informações" → "Executar assim mesmo".
- O `umvc3.exe` da Steam vem com uma proteção que o port não consegue ler. O instalador pergunta antes de baixar o
  [Steamless](https://github.com/atom0s/Steamless) (de atom0s) da página oficial e aplicá-lo numa **cópia** do seu
  executável. A pasta do jogo nunca é alterada.
- Nada aqui mexe no firmware, na NAND ou nas partições do cartão: só arquivos em `switch/` e `umvc3/` no SD.
- Abra o Homebrew Menu por um jogo (segurando R), não pelo Álbum: pelo Álbum não há memória suficiente.

## Código-fonte

`fonte/` tem o código do instalador e as pequenas ferramentas que geram os dados dele. O instalador leva embutido
o compilador de shaders `uam`, do devkitPro (licença em `docs/LICENCA-uam.txt`).

## Licença

Veja [LICENSE.md](LICENSE.md): você pode baixar, rodar, estudar e modificar para uso próprio o código e os textos
do port; publicar mudanças ou reaproveitá-los em outro lugar exige pedir autorização ao autor antes (abra uma
*issue* para pedir). Notas da versão: [CHANGELOG.md](CHANGELOG.md).

## Créditos

Veja [CREDITS.md](CREDITS.md). Ultimate Marvel vs. Capcom 3 pertence à Capcom e à Marvel. Este projeto não é
afiliado a nenhuma delas e não substitui a compra do jogo.
