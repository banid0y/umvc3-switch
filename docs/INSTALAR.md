# Instalação passo a passo

Tempo total: 10 a 30 minutos, quase todo ele copiando arquivos para o cartão.

## Antes de começar

Confira que você tem:

- [ ] Switch com Atmosphère e Homebrew Menu funcionando.
- [ ] Ultimate Marvel vs. Capcom 3 instalado pela Steam num PC com Windows 10 ou 11.
- [ ] Cartão SD com espaço livre: 4 GB (jogo original) ou 13 GB (com a Community Edition).
- [ ] Um jeito de pôr arquivos no cartão: leitor de cartão no PC (mais rápido), ou FTP/MTP pelo próprio Switch.
- [ ] Internet no PC na primeira vez (o instalador baixa uma ferramenta pequena, o Steamless).
- [ ] Só para a versão com CE: a pasta "UMvC3 Community Edition Build".

Nada neste processo mexe no firmware, na NAND ou nas partições do cartão: são apenas arquivos nas pastas
`switch/` e `umvc3/` do SD. A pasta do jogo no PC também não é alterada.

## 1. Abrir o instalador

Extraia o pacote numa pasta qualquer e abra **`Install.exe`**. Mantenha a pasta `aplicativos` ao lado dele.

A janela abre em inglês. Para português, escolha **Português** na caixa do canto superior direito.

Se o Windows avisar que o programa é desconhecido, escolha "Mais informações" e "Executar assim mesmo".

## 2. Preencher a janela e instalar

1. **Pasta do jogo:** o instalador já preenche com a pasta padrão da Steam. Se o seu jogo estiver em outro lugar,
   clique em "Procurar..." e escolha a pasta "ULTIMATE MARVEL VS. CAPCOM 3".
2. **Community Edition (opcional):** marque a caixa e escolha a pasta "UMvC3 Community Edition Build". Deixe
   desmarcada para instalar só o jogo original.
3. **Destino:** escolha o cartão SD (se ele estiver no leitor) ou uma pasta do PC.
4. Clique em **Instalar**.
5. **Steamless:** o `umvc3.exe` da Steam vem com uma proteção que o port não consegue ler. Na primeira vez o
   instalador pergunta se pode baixar o Steamless (programa gratuito, de atom0s) da página oficial e aplicá-lo
   numa cópia do seu executável. Responda "Sim".

Depois disso ele trabalha sozinho, mostrando a etapa e uma barra de progresso: confere se o seu jogo é da versão
suportada, compila os shaders para a GPU do Switch (cerca de um minuto) e confere cada um deles, prepara os
plugins da CE e copia os dados do jogo (3,6 GB, mais 8,3 GB com a CE).

No fim, o destino fica assim:

```
switch/umvc3.nro                  (BASE) ULTIMATE MARVEL VS CAPCOM 3: jogo original
switch/umvc3-ce.nro               (CE) ULTIMATE MARVEL VS CAPCOM 3: só se você instalou a CE
umvc3/umvc3.exe.unpacked.exe
umvc3/dksh.pak                    shaders compilados
umvc3/nativePCx64/                dados do jogo
umvc3/texturas/                   ícones de botão do Switch
umvc3/ce/  e  umvc3/ce-data/      só com a Community Edition
```

## 3. Passar para o cartão (se você não instalou direto nele)

Copie as pastas `switch` e `umvc3` para a **raiz** do cartão SD, mesclando com as que já existem.

- **Leitor de cartão:** desligue o Switch, tire o cartão, copie, devolva.
- **FTP:** abra um servidor FTP no Switch (por exemplo o ftpd) e envie com qualquer cliente FTP.
- **MTP (DBI):** conecte o cabo USB, escolha "MTP responder" no DBI e copie pelo Explorador de Arquivos.

## 4. Abrir o jogo

1. No menu do Switch, **segure R** e abra **um jogo qualquer** (não o Álbum). Isso abre o Homebrew Menu com a
   memória inteira do console; pelo Álbum o jogo não cabe.
2. Escolha **(BASE) ULTIMATE MARVEL VS CAPCOM 3** (jogo original) ou **(CE) ULTIMATE MARVEL VS CAPCOM 3** (com a
   Community Edition).
3. A primeira abertura leva cerca de um minuto carregando os shaders.

## Atualizar depois

Para uma versão nova do port, basta trocar os arquivos `.nro` em `switch/`. Os dados do jogo, shaders e saves ficam.
Quando uma versão precisar de shaders novos, as notas da versão avisam: rode o instalador de novo.

## Desinstalar

Apague `switch/umvc3.nro`, `switch/umvc3-ce.nro` e a pasta `umvc3/` do cartão. Os saves estão em
`umvc3/userdata/` e `umvc3/userdata-ce/`; guarde essas pastas se quiser mantê-los.

## Se você já tem o Steamless ou está sem internet

Extraia o Steamless 3.1.0.5 numa pasta chamada `steamless` ao lado do `Install.exe` (com o `Steamless.CLI.exe`
dentro). O instalador usa essa cópia e não baixa nada. Se a pasta do jogo já tiver um `umvc3.exe.unpacked.exe`,
ele usa esse arquivo direto.
