# Opções do port

A maioria das pessoas não precisa mexer aqui. Cada opção é um arquivo de texto de uma linha em `umvc3/` no cartão;
apagar o arquivo volta ao padrão.

| Arquivo | Conteúdo | Efeito |
|---|---|---|
| `dk_rscale.txt` | `auto` (padrão), ou `70` a `100` | Resolução da cena 3D em % (o HUD fica sempre em 720p). Automática: o port escolhe entre 100, 90, 80 e 70 conforme a carga da GPU |
| `ovl_fps.txt` | `0` ou `1` | Medidor de quadros na tela |
| `menu_jp.txt` | `0` | Volta os menus ao padrão do Xbox (botão de baixo confirma) |
| `joyhold.txt` | `h` | Deixa o sistema girar o Joy-Con deitado, em vez do port |
| `versus_applet.txt` | `0` | Não abre a tela de controles do sistema ao entrar em VERSUS |
| `apm.txt` | `0` | Não pede o perfil oficial de desempenho do modo portátil |

A qualidade das texturas e dos efeitos fica no `config.ini` do próprio jogo, em
`umvc3/userdata/user/CAPCOM/ULTIMATE MARVEL VS. CAPCOM 3/` (ou `userdata-ce/` na versão com CE).
