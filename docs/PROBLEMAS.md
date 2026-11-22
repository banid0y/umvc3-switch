# Problemas comuns

| Sintoma | Causa provável | O que fazer |
|---|---|---|
| Fecha logo ao abrir, ou tela preta | Aberto pelo Álbum (pouca memória) | Abrir o Homebrew Menu segurando R ao iniciar um jogo |
| "shader pré-compilado ausente" no log, imagem faltando | `umvc3/dksh.pak` ausente ou incompleto | Rodar o instalador de novo |
| O instalador avisa que o `umvc3.exe` é de outra versão | Versão do jogo diferente da suportada | Atualizar o jogo na Steam e rodar o instalador de novo |
| A versão com CE abre o jogo original | Falta `umvc3/ce/` ou `umvc3/ce-data/Characters.ini`, ou a sua CE não é a 1.2.3b | Rodar o instalador de novo e responder "Sim" para a Community Edition |
| VERSUS não sai da tela de controles | Menos de dois controles pareados | Parear dois controles (ou dois Joy-Con deitados) na tela do sistema |
| Quedas de quadros em lutas pesadas | Limite da GPU no modo portátil | O port reduz sozinho a resolução da cena; ver `docs/OPCOES.md` |
| Travando em tudo, até nos menus ou no treino | Um módulo de clock (sys-clk ou parecido) segurando CPU/GPU abaixo do perfil do port, ou cartão SD lento | Ver "Desempenho, sys-clk e overclock" abaixo |

## Desempenho, sys-clk e overclock

- **Não precisa nem usa overclock.** O port foi desenvolvido e medido com os clocks de fábrica. No modo portátil
  ele pede ao sistema o perfil oficial de desempenho máximo do modo portátil (o mesmo tipo de perfil que alguns
  jogos comerciais usam); isso não é overclock.
- **Módulos de clock podem passar por cima disso.** Se você usa o sys-clk (ou qualquer módulo que define clocks de
  CPU/GPU/memória), confira se não há um perfil baixando os clocks para o Homebrew Menu ou para o jogo pelo qual
  você o abre. Um perfil mais baixo ali deixa tudo travando. Para tirar a dúvida, desligue o módulo e teste de novo.
- **Overclock não foi testado.** Subir os clocks acima dos de fábrica pode ajudar ou causar instabilidade; é por
  sua conta e o port não garante nada nesse caso.
- **Hardware testado.** Até agora o port foi medido em um único Switch OLED. Relatos de outros modelos (V1, V2,
  Lite) são bem-vindos: ligue o contador de FPS (`ovl_fps.txt`, ver `docs/OPCOES.md`) e mande o log como descrito abaixo.
- `umvc3/apm.txt` com `0` faz o port não pedir o perfil de desempenho (ver `docs/OPCOES.md`).

## Como mandar um relatório

Passo a passo: [RELATAR.md](RELATAR.md).

Anexe o arquivo `umvc3/host-nx.log` do cartão (ele é regravado a cada abertura; os três anteriores ficam como
`host-nx-NN.log`). Diga qual das duas versões você abriu, em que tela estava e o que apertou.
