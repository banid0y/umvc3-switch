# Relatar lentidão, travamentos e erros

"Está lento" ou "não funciona" não dá para corrigir sem o arquivo de log do port. Até agora o port só foi medido num
Switch OLED, então o seu log é o único jeito de ver o que acontece no seu console. Leva uns cinco minutos.

## Antes de relatar

- Abra o Homebrew Menu **segurando R ao abrir um jogo**, não pelo Álbum.
- Se você usa sys-clk ou outro módulo de clock, confira se ele não está baixando os clocks (veja
  [PROBLEMAS.md](PROBLEMAS.md)).
- Confira se está na versão mais recente ([ATUALIZAR.md](ATUALIZAR.md)).

## 1. Reproduza o problema

1. Opcional, para lentidão: crie no cartão SD um arquivo de texto `umvc3/ovl_fps.txt` contendo só `1`. Ele mostra o
   contador de quadros na tela.
2. Abra o jogo e vá até onde o problema acontece. Para lentidão, fique ali **dois minutos ou mais** (uma luta
   inteira, por exemplo): o log grava números de desempenho a cada poucos segundos.
3. Feche o jogo. **Não abra de novo** antes de copiar o log: cada abertura começa um log novo.

## 2. Copie o log do cartão SD

O arquivo é **`umvc3/host-nx.log`**. Copie para o PC ou celular do mesmo jeito que instalou o jogo: leitor de
cartão, FTP ou MTP (DBI).

Se você já reabriu o jogo, a sessão que interessa é um dos três logs antigos guardados ao lado,
`umvc3/host-nx-NN.log`: envie todos.

Depois de um travamento com tela de erro do sistema, uma foto dessa tela também ajuda.

## 3. Envie

Abra um relato aqui: **https://github.com/banid0y/umvc3-switch/issues/new/choose**

- **Performance / lag report** para lentidão e engasgos.
- **Crash or bug** para o resto (fecha, congela, imagem errada, controles).

Preencha o formulário (pode escrever em português) e arraste o arquivo de log para a caixa "Log file" (compacte em
zip se o GitHub recusar). É preciso uma conta gratuita do GitHub. Sem conta? Suba o log em qualquer site de arquivos
ou de texto e poste o link onde você relatou o problema, com o modelo do console, portátil ou dock, e qual
aplicativo (BASE ou CE).

Um relato por problema, por favor; se alguém já relatou a mesma coisa, acrescente o seu log àquele relato.

## O que tem no log

Mensagens do port e números de desempenho (tempos de quadro, carga da GPU e da CPU, resolução da cena, tempos de
leitura do cartão SD), em português. Não tem conta, número de série nem outro dado pessoal, e o port não envia nada
sozinho: o arquivo só sai do cartão se você enviar.
