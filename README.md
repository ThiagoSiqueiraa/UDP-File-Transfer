# UDP-File-Transfer
Transfer of files using UDP protocol.


tracker.c -> Responsável por informar ao cliente (peer) quais são os clientes (peers) que possuem determinado arquivo a ser baixado, age como um banco de dados.

peer.c -> Executa o download de um determinado arquivo ponto-a-ponto (p2p).

Uso:

1) Execute o tracker.c
2) Execute o peer.c e escolha a opção desejada
  [l] - para listar os peers que possuem um arquivo pelo nome.
      -> se existir alguém com arquivo, você irá de qual deseja baixar.
  [a] - para informar ao tracker que você possui tal arquivo.

