Transfer of files using UDP protocol. tracker.c -> Responsável por informar ao cliente (peer) quais são os clientes (peers) que possuem determinado arquivo a ser baixado, age como um banco de dados. peer.c -> Executa o download de um determinado arquivo ponto-a-ponto (p2p).

Uso:

Execute o tracker.c
Execute o peer.c e escolha a opção desejada [l] - para listar os peers que possuem um arquivo pelo nome. -> se existir alguém com arquivo, você irá dizer de qual peer deseja baixar. -> aguarde finalização da transferencia para digitar nova opção [a] - para informar ao tracker que você possui tal arquivo. -> basta inserir o nome do arquivo, o arquivo deve estar na mesma pasta em que o cliente (em execução) -> o arquivo é inserido automaticamente ao tracker se você fizer download dele.
Para ambas operações por-favor informar o nome do arquivo + extensão Ex: Led Zeppelin - Stairway To Heaven (Lyrics) HD.mp3 filename.ext

São possiveis executar vários peer, mas ATENÇÃO [1] em importante.

IMPORTANTE - [1] NÃO TENTAR BAIXAR O MESMO ARQUIVO QUANDO UM USUÁRIO JÁ ESTIVER TRANSFERINDO OU BAIXANDO [2] O PROGRAMA SÓ RODA EM SISTEMA OPERACIONAL UNIX (LINUX) POR CAUSA DO PTHREAD [3] PARA MELHORES RESULTADOS EXECUTAR CLIENTES EM DIRETORIOS DIFERENTES.

COMPILAÇÃO: peer -> gcc peer.c -o peer -pthread tracker -> gcc tracker.c -o tracker -pthread

EXECUÇÃO peer -> ./peer tracker -> ./tracker
