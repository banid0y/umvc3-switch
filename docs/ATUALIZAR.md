# Atualizar para uma versão nova

Atualizar só troca os arquivos do aplicativo. Saves, configurações, dados do jogo e shaders continuam no cartão.

## 1. Baixe os arquivos novos

Baixe este repositório de novo (**Code → Download ZIP**) e extraia, ou baixe só os arquivos da pasta
[`NRO FILES`](../NRO%20FILES) (abra o arquivo no GitHub e use o botão de download):

- `umvc3.nro`: o jogo original.
- `umvc3-ce.nro`: o jogo com a Community Edition.

Basta o que você joga. O [CHANGELOG.md](../CHANGELOG.md) lista o que mudou em cada versão.

## 2. Troque no cartão SD

1. Feche o jogo no Switch.
2. Copie o `.nro` novo por cima do antigo na pasta **`switch`** da raiz do cartão SD, mantendo o mesmo nome de
   arquivo. Use o mesmo caminho da instalação: leitor de cartão, FTP ou MTP (DBI).
3. Abra o jogo como sempre (segure R num jogo qualquer → Homebrew Menu). O número da versão que aparece ao lado do
   nome do aplicativo no Homebrew Menu mostra que a cópia deu certo.

Não apague a pasta `umvc3`: nela estão os dados do jogo, os shaders e os seus saves.

## Quando rodar o instalador de novo

Só quando as notas daquela versão pedirem (por exemplo, shaders novos). Nesse caso abra o `Install.exe` novo e
instale no mesmo destino de antes. Saves e configurações do jogo que já estão no cartão são mantidos.

Por garantia, copie antes `umvc3/userdata` (jogo original) e `umvc3/userdata-ce` (Community Edition) para o PC:
essas duas pastas são os seus saves e configurações.

## Observações

- **Forwarder no menu do console:** ele aponta para o caminho do `.nro`, então continua funcionando depois da troca.
- **Arquivos de opção** que você criou em `umvc3/` (veja [OPCOES.md](OPCOES.md)) continuam valendo.
- **Voltar atrás:** guarde uma cópia do `.nro` antigo; devolvê-lo a `switch/` restaura a versão anterior.
- **Uma Community Edition mais nova** não é atualização do port: só a versão da CE citada no
  [LEIAME](../LEIAME.md) é suportada.
- Piorou depois de atualizar? Abra um relato com o seu arquivo de log:
  [novo issue](https://github.com/banid0y/umvc3-switch/issues/new/choose).
