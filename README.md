# banker

Algoritmo do Banqueiro escrito em C para a disciplina Infraestrutura de Software
(00016) da CESAR School. O programa recebe pela linha de comando quantas instâncias
existem de cada recurso, lê o máximo de cada cliente em `customer.txt`, executa os
pedidos (`RQ`), liberações (`RL`) e impressões de estado (`*`) de `commands.txt` e
grava uma linha de resultado por comando em `result.txt`. Um pedido só é atendido
se não passar da necessidade do cliente, couber no disponível e deixar o sistema em
estado seguro.

O número de clientes e o de recursos **não são fixos**: o de recursos vem da
quantidade de argumentos e o de clientes, das linhas de `customer.txt`. As matrizes
são alocadas com `malloc` como blocos contíguos `n_cli·n_rec`. As entradas inteiras
(`argv`, `customer.txt` e `commands.txt`) são validadas antes de executar o primeiro
comando, e `result.txt` só é aberto depois disso: entrada inválida nunca cria nem
trunca um `result.txt`.

## Arquivos

```
.
├── banker.c
├── evidencias.log
├── Makefile
├── banker
├── jems2.pdf
└── README.md
```

- **`banker.c`**: o programa inteiro, num arquivo só, dividido em seções na ordem de
  uso (sem `.h` nem protótipos; tudo `static` exceto `main`):
  - **erros**: `enum ErroParse` e `erro_msg()`, o mesmo padrão das atividades 1 e 3.
  - **parser**: `para_nao_neg_int` (`strtol` com `errno`, `endptr` e limite de
    `int`), `le_recursos` (argv), `le_clientes` (`customer.txt`) e `le_comandos`
    (`commands.txt`). As linhas são lidas com `getline`, então não há limite de
    tamanho de linha nem de número de recursos. Os comandos são lidos todos para um
    vetor antes de executar o primeiro.
  - **banco**: `struct Banco` (`available`, `allocation`, `need`, `maximum` e o
    rascunho `work`/`finish` do teste de segurança, alocado uma vez só).
    - `banco_request(banco, i, req)` checa nesta ordem: `req ≤ need[i]` →
      `req ≤ available` → estado seguro. Para testar a segurança, aloca o pedido
      e, se o estado ficar inseguro, desfaz a alocação.
    - `banco_seguro(banco)` simula os clientes terminando: procura um cliente com
      `need ≤ work`, soma a alocação dele em `work` e repete até uma passada
      inteira sem progresso. Seguro se todos terminarem.
    - `banco_release(banco, i, rel)` devolve os recursos a `available` e a `need`;
      liberar mais do que o cliente tem alocado é negado.
  - **saída**: `escreve_decisao` (uma linha por `RQ`/`RL`) e `escreve_estado` (a
    tabela do `*`).
  - **main**: `carrega_recursos` → `carrega_clientes` → `carrega_comandos` →
    `banco_cria` → `escreve_resultado`. Libera toda a memória em qualquer caminho.
- **`Makefile`**: compila `banker.c` direto para `banker`.
- **`evidencias.log`**: sessões gravadas com `script` (build, exemplo oficial e
  bateria de testes).

## Compilar

```
make
```

Gera o executável `banker` na raiz do projeto, sem warnings. Para limpar:

```
make clean       # remove banker
make distclean   # clean + remove result.txt
make rebuild     # clean + make
```

## Executar

```
./banker <instâncias do recurso 0> [instâncias do recurso 1 ...]
```

`customer.txt` e `commands.txt` precisam estar no diretório atual; o resultado vai
para `result.txt` no mesmo diretório. Os nomes são fixos.

`customer.txt` tem uma linha por cliente com o máximo de cada recurso separado por
vírgula. O cliente 0 é a primeira linha:

```
7,5,3
3,2,2
9,0,2
2,2,2
4,3,3
```

`commands.txt` tem um comando por linha, separado por espaços: `RQ <cliente>
<recursos...>`, `RL <cliente> <recursos...>` ou `*`:

```
RQ 0 0 2 1
RQ 2 3 0 2
RQ 1 2 0 0
RQ 4 0 0 2
RL 0 0 1 1
RQ 3 2 1 1
*
RQ 4 4 0 0
RQ 1 1 0 2
RQ 0 0 2 0
RQ 2 0 1 0
```

Com o exemplo do enunciado, `./banker 10 5 7` não escreve nada no terminal, sai com
código 0 e grava em `result.txt`:

```
Allocate to customer 0 the resources 0 2 1
Allocate to customer 2 the resources 3 0 2
Allocate to customer 1 the resources 2 0 0
Allocate to customer 4 the resources 0 0 2
Release from customer 0 the resources 0 1 1
Allocate to customer 3 the resources 2 1 1
MAXIMUM   |   ALLOCATION  |   NEED
7 5 3     |   0 1 0       |   7 4 3 
3 2 2     |   2 0 0       |   1 2 2 
9 0 2     |   3 0 2       |   6 0 0 
2 2 2     |   2 1 1       |   0 1 1 
4 3 3     |   0 0 2       |   4 3 1 
AVAILABLE 3 3 2 
The resources 3 3 2 are not enough to customer 4 request 4 0 0 
Allocate to customer 1 the resources 1 0 2
The customer 0 request 0 2 0 was denied because result in an unsafe state
The customer 2 request 0 1 0 was denied because exceed its maximum allocation
```

O arquivo é idêntico byte a byte ao `result.txt` oficial do professor, inclusive o
espaço no fim das linhas da tabela, de `AVAILABLE` e de `not enough`. O PDF do
enunciado mostra `The resources 3 2 2 are not enough ...` nessa linha, mas o
`result.txt` oficial traz `3 3 2`, que é o disponível real (o mesmo do `AVAILABLE`
logo acima); o programa segue o oficial.

Pedido negado, falta de recurso, estado inseguro e liberação maior que o alocado não
são erros: viram uma linha no `result.txt` e a execução continua. Para a liberação
negada, que o enunciado não define, a linha é `The customer <i> release <rel> was
denied because exceed its allocation`.

### Formato da tabela do `*`

O exemplo oficial só tem 3 recursos de um dígito. Para qualquer número de recursos e
de dígitos, cada vetor é impresso como `"%d "` e as colunas são unidas por
separadores fixos (`"    |   "` entre MAXIMUM e ALLOCATION, `"      |   "` entre
ALLOCATION e NEED), que são exatamente os do arquivo oficial. Com mais recursos ou
números maiores, a coluna cresce junto com o vetor em vez de ser cortada ou ficar
desalinhada por largura fixa. Os dois separadores são as constantes `SEP_MAX_ALLOC`
e `SEP_ALLOC_NEED` no `banker.c`.

### Regras de formato da entrada

- Inteiros aceitam 0 e rejeitam negativo, decimal (`4.5`), texto, sinal sozinho,
  campo vazio e valor acima de `INT_MAX`.
- `customer.txt`: sem campo vazio (`7,,3`, `,7,3`, `7,5,3,`); espaço em volta do
  número é aceito (`7, 5 ,3`).
- Os dois arquivos aceitam a última linha sem `\n` (é assim nos oficiais), fim de
  linha `\r\n` (arquivo criado no Windows) e linhas em branco, que são ignoradas.
- `commands.txt` vazio é válido e gera `result.txt` vazio.
- Um máximo maior que o total de instâncias é aceito: esse cliente nunca consegue
  terminar, então todo pedido que passar das duas primeiras checagens vira
  `unsafe state`.

### Erros

Entrada inválida escreve uma mensagem em `stderr`, sai com código 1 e não cria nem
altera `result.txt`. A mensagem aponta o arquivo, a linha, o recurso e o valor:

```
$ ./banker
Uso: ./banker <instancias do recurso 0> [instancias do recurso 1 ...]
Exemplo: ./banker 10 5 7
$ ./banker 10 -1 7
Erro: argumento 2 ('-1'): valor nao pode ser negativo
$ ./banker 10 5
Erro: customer.txt: linha 1: quantidade de recursos diferente da passada na linha de comando (3 na linha, 2 na linha de comando)
$ ./banker 10 5 7          # customer.txt ausente
Erro: nao foi possivel abrir 'customer.txt': No such file or directory
```

Os casos tratados são: número de argumentos diferente do número de recursos em
`customer.txt` ou em `commands.txt`; argumento, máximo ou recurso não numérico,
negativo ou grande demais; arquivo inexistente, sem permissão ou que é um
diretório; `customer.txt` sem nenhum cliente; campo vazio; comando desconhecido;
cliente inexistente; `*` com argumentos; falha ao criar ou escrever `result.txt`
(diretório sem escrita, disco cheio).

## Testar

Teste manual com o exemplo do enunciado, num diretório com o `customer.txt`, o
`commands.txt` e o `result.txt` oficiais (este último renomeado para
`result_esperado.txt`):

```
make && ./banker 10 5 7 && cmp result.txt result_esperado.txt && echo IGUAL
```

No repositório do GitHub (fora do `.tar`, que não leva os arquivos do professor)
está a bateria completa:

```
sh tests/run_tests.sh
```

## Sistema operacional

Implementado e testado em Linux (Omarchy, baseado em Arch, kernel 7.2.15, x86_64)
com GCC 16.2.1 e padrão `c11`. Usa só a biblioteca padrão e `getline`/`strtok_r` do
POSIX 2008 (`-D_POSIX_C_SOURCE=200809L`), sem threads, então deve compilar em
qualquer Linux, Unix ou macOS com um compilador C11.
