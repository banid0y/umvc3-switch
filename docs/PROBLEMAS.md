# Problemas comuns

| Sintoma | Causa provável | O que fazer |
|---|---|---|
| Fecha logo ao abrir, ou tela preta | Aberto pelo Álbum (pouca memória) | Abrir o Homebrew Menu segurando R ao iniciar um jogo |
| "shader pré-compilado ausente" no log, imagem faltando | `umvc3/dksh.pak` ausente ou incompleto | Rodar o instalador de novo |
| O instalador avisa que o `umvc3.exe` é de outra versão | Versão do jogo diferente da suportada | Atualizar o jogo na Steam e rodar o instalador de novo |
| A versão com CE abre o jogo original | Falta `umvc3/ce/` ou `umvc3/ce-data/Characters.ini` | Rodar o instalador de novo e responder "Sim" para a Community Edition |
| VERSUS não sai da tela de controles | Menos de dois controles pareados | Parear dois controles (ou dois Joy-Con deitados) na tela do sistema |
| Quedas de quadros em lutas pesadas | Limite da GPU no modo portátil | O port reduz sozinho a resolução da cena; ver `docs/OPCOES.md` |

## Como mandar um relatório

Anexe o arquivo `umvc3/host-nx.log` do cartão (ele é regravado a cada abertura; os três anteriores ficam como
`host-nx-NN.log`). Diga qual das duas versões você abriu, em que tela estava e o que apertou.
