#!/bin/sh
# tests/f1_check.sh -- checagem rapida da F1 (argv + customer.txt + commands.txt)
# e da F2/F3 (decisoes do banqueiro + result.txt byte a byte).
# Adaptado do f1_check.sh/f5_check.sh da atvd3, com auxilio de IA (secao Uso de IA).
# Os nomes de entrada sao fixos, entao cada caso roda num diretorio temporario
# proprio: nada no projeto e sobrescrito.
set -u

cd "$(dirname "$0")/.." || exit 1
make || { echo "make FALHOU"; exit 1; }

raiz=$(pwd)
banker="$raiz/banker"
exemplo="$raiz/tests/vetores/exemplo"
tmp=$(mktemp -d)
trap 'chmod -R u+rwx "$tmp" 2>/dev/null; rm -rf "$tmp"' EXIT
ok=0
falha=0

CLIENTES='7,5,3\n3,2,2\n9,0,2\n2,2,2\n4,3,3\n'
COMANDOS='RQ 0 0 2 1\n*\n'

# monta $tmp/caso com customer.txt e commands.txt (printf interpreta \n e \r)
preparar() {
    rm -rf "$tmp/caso"
    mkdir "$tmp/caso"
    printf "$1" > "$tmp/caso/customer.txt"
    printf "$2" > "$tmp/caso/commands.txt"
}

# erro: exit != 0, stderr nao vazio, stdout vazio, nenhum result.txt criado
esperar_erro() {
    desc=$1
    shift
    saida=$(cd "$tmp/caso" && "$banker" "$@" 2>"$tmp/err"); rc=$?
    if [ "$rc" -ne 0 ] && [ -s "$tmp/err" ] && [ -z "$saida" ] && [ ! -e "$tmp/caso/result.txt" ]; then
        echo "ok     $desc  ->  $(head -1 "$tmp/err")"; ok=$((ok + 1))
    else
        echo "FALHA  $desc  (rc=$rc, stdout='$saida')"; falha=$((falha + 1))
    fi
}

# valido: exit 0, stdout e stderr vazios
esperar_ok() {
    desc=$1
    shift
    saida=$(cd "$tmp/caso" && "$banker" "$@" 2>"$tmp/err"); rc=$?
    if [ "$rc" -eq 0 ] && [ -z "$saida" ] && [ ! -s "$tmp/err" ]; then
        echo "ok     $desc"; ok=$((ok + 1))
    else
        echo "FALHA  $desc  (rc=$rc, stdout='$saida', err='$(head -1 "$tmp/err")')"
        falha=$((falha + 1))
    fi
}

# F2/F3: exit 0, stdout/stderr vazios e result.txt igual (cmp) ao arquivo $2.
# Na falha mostra o diff com cat -A ('$' = fim de linha, deixa o espaco visivel).
esperar_resultado() {
    desc=$1
    esperado=$2
    shift 2
    saida=$(cd "$tmp/caso" && "$banker" "$@" 2>"$tmp/err"); rc=$?
    if [ "$rc" -eq 0 ] && [ -z "$saida" ] && [ ! -s "$tmp/err" ] \
       && cmp -s "$tmp/caso/result.txt" "$esperado"; then
        echo "ok     $desc"; ok=$((ok + 1))
    else
        echo "FALHA  $desc  (rc=$rc, err='$(head -1 "$tmp/err")')"
        if [ -e "$tmp/caso/result.txt" ]; then
            cat -A "$esperado" > "$tmp/esperado_A"
            cat -A "$tmp/caso/result.txt" > "$tmp/obtido_A"
            echo "       diff esperado obtido:"
            diff "$tmp/esperado_A" "$tmp/obtido_A" | sed 's/^/         /'
        fi
        falha=$((falha + 1))
    fi
}

# grava o result.txt esperado em $tmp/esperado (printf interpreta \n)
esperado() {
    printf "$1" > "$tmp/esperado"
}

CAB='MAXIMUM   |   ALLOCATION  |   NEED\n'

echo "== argumentos"
preparar "$CLIENTES" "$COMANDOS"
esperar_erro "#1 sem argumentos"
esperar_erro "#2 nao numerico"            10 x 7
esperar_erro "#2 negativo"                10 -1 7
esperar_erro "#2 overflow"                99999999999 5 7
esperar_erro "#7 menos argumentos"        10 5
esperar_erro "#7 mais argumentos"         10 5 7 1

echo "== customer.txt"
preparar "$CLIENTES" "$COMANDOS"; rm "$tmp/caso/customer.txt"
esperar_erro "#3 inexistente"             10 5 7
preparar "$CLIENTES" "$COMANDOS"; chmod 000 "$tmp/caso/customer.txt"
esperar_erro "#4 sem permissao"           10 5 7
preparar "$CLIENTES" "$COMANDOS"; rm "$tmp/caso/customer.txt"; mkdir "$tmp/caso/customer.txt"
esperar_erro "#19 e um diretorio"         10 5 7
preparar "" "$COMANDOS";                  esperar_erro "#6 vazio"              10 5 7
preparar '\n   \n' "$COMANDOS";           esperar_erro "#6 so linhas em branco" 10 5 7
preparar '7,a,3\n' "$COMANDOS";           esperar_erro "#8 nao numerico"       10 5 7
preparar '3,2,2\n7,-1,3\n' "$COMANDOS";   esperar_erro "#8 negativo na linha 2" 10 5 7
preparar '7,,3\n' "$COMANDOS";            esperar_erro "#8 7,,3"               10 5 7
preparar ',7,3\n' "$COMANDOS";            esperar_erro "#8 ,7,3"               10 5 7
preparar '7,5,3,\n' "$COMANDOS";          esperar_erro "#8 7,5,3,"             10 5 7
preparar '7 5 3\n' "$COMANDOS";           esperar_erro "#8 espaco no lugar de virgula" 10 5 7

echo "== commands.txt"
preparar "$CLIENTES" "$COMANDOS"; rm "$tmp/caso/commands.txt"
esperar_erro "#5 inexistente"             10 5 7
preparar "$CLIENTES" 'RQ 0 1 1\n';        esperar_erro "#9 recurso a menos"    10 5 7
preparar "$CLIENTES" 'RQ 0 1 1 1 9\n';    esperar_erro "#9 recurso a mais"     10 5 7
preparar "$CLIENTES" 'RQ\n';              esperar_erro "#9 RQ sozinho"         10 5 7
preparar "$CLIENTES" 'XX 0 1 1 1\n';      esperar_erro "#10 desconhecido"      10 5 7
preparar "$CLIENTES" 'rq 0 1 1 1\n';      esperar_erro "#10 minusculo"         10 5 7
preparar "$CLIENTES" 'RQ 0 0 2 1\nRQ 5 1 1 1\n'; esperar_erro "#11 cliente 5 (linha 2)" 10 5 7
preparar "$CLIENTES" 'RQ -1 1 1 1\n';     esperar_erro "#11 cliente -1"        10 5 7
preparar "$CLIENTES" '* 1\n';             esperar_erro "#12 * com argumento"   10 5 7
preparar "$CLIENTES" 'RL 0 1 x 1\n';      esperar_erro "#13 nao numerico"      10 5 7
preparar "$CLIENTES" 'RQ 0 1 -2 1\n';     esperar_erro "#13 negativo"          10 5 7

echo "== entradas validas"
rm -rf "$tmp/caso"; mkdir "$tmp/caso"; cp "$exemplo/customer.txt" "$exemplo/commands.txt" "$tmp/caso"
esperar_ok "#17 entrada oficial"          10 5 7
preparar "$CLIENTES" "$COMANDOS";         esperar_ok "#18 quebra de linha no fim" 10 5 7
preparar '7,5,3\r\n3,2,2\r\n' 'RQ 1 0 2 1\r\n*\r\n'; esperar_ok "#16 CRLF"   10 5 7
preparar '\n7, 5 ,3\n\n3,2,2\n\n' '\nRQ 0 0 2 1\n\n*\n'; esperar_ok "linhas em branco e espacos" 10 5 7
preparar "$CLIENTES" '';                  esperar_ok "commands.txt vazio"     10 5 7
preparar "$CLIENTES" "$COMANDOS";         esperar_ok "instancias zero"        0 0 0
preparar "$(seq 1 2000 | paste -sd, -)\n" "RQ 0 $(seq 1 2000 | tr '\n' ' ')\n"
esperar_ok "#14 2000 recursos (linha longa)" $(seq 1 2000)

echo "== F2/F3: banqueiro + result.txt"
rm -rf "$tmp/caso"; mkdir "$tmp/caso"; cp "$exemplo/customer.txt" "$exemplo/commands.txt" "$tmp/caso"
esperar_resultado "#17 exemplo oficial = result_esperado.txt" "$exemplo/result_esperado.txt" 10 5 7

# need = maximum - allocation: o 2o pedido cabe no maximum (3 3), mas nao no need (1 1)
preparar '3,3\n' 'RQ 0 2 2\nRQ 0 2 2\n*\n'
esperado "Allocate to customer 0 the resources 2 2\nThe customer 0 request 2 2 was denied because exceed its maximum allocation\n${CAB}3 3     |   2 2       |   1 1 \nAVAILABLE 3 3 \n"
esperar_resultado "F2 req <= maximum mas > need -> exceed" "$tmp/esperado" 5 5

# unsafe tem que desfazer o pedido: o * mostra cliente 1 sem nada alocado
preparar '3,3\n3,3\n' 'RQ 0 2 2\nRQ 1 2 2\n*\n'
esperado "Allocate to customer 0 the resources 2 2\nThe customer 1 request 2 2 was denied because result in an unsafe state\n${CAB}3 3     |   2 2       |   1 1 \n3 3     |   0 0       |   3 3 \nAVAILABLE 2 2 \n"
esperar_resultado "F2 unsafe + rollback" "$tmp/esperado" 4 4

# seguro so com 2a passada (C1 termina antes de C0) e need == work exato
preparar '2,2\n1,1\n' 'RQ 1 1 1\n*\n'
esperado "Allocate to customer 1 the resources 1 1\n${CAB}2 2     |   0 0       |   2 2 \n1 1     |   1 1       |   0 0 \nAVAILABLE 1 1 \n"
esperar_resultado "F2 2a passada e need == work" "$tmp/esperado" 2 2

# RL maior que o alocado e negado (D.4); RL valido devolve need
preparar '3,3\n' 'RQ 0 2 2\nRL 0 3 0\nRL 0 1 2\n*\n'
esperado "Allocate to customer 0 the resources 2 2\nThe customer 0 release 3 0 was denied because exceed its allocation\nRelease from customer 0 the resources 1 2\n${CAB}3 3     |   1 0       |   2 3 \nAVAILABLE 4 5 \n"
esperar_resultado "F2 release (D.4 + need devolvido)" "$tmp/esperado" 5 5

# ordem maximo -> disponivel: 1o excede os dois (vale o maximo), 2o so o disponivel
preparar '1,1\n' 'RQ 0 2 2\nRQ 0 1 1\n'
esperado "The customer 0 request 2 2 was denied because exceed its maximum allocation\nThe resources 0 0 are not enough to customer 0 request 1 1 \n"
esperar_resultado "F2 ordem maximo -> disponivel" "$tmp/esperado" 0 0

# 1 recurso, numero de 2 digitos: separadores fixos (D.3)
preparar '12\n' 'RQ 0 10\n*\n'
esperado "Allocate to customer 0 the resources 10\n${CAB}12     |   10       |   2 \nAVAILABLE 10 \n"
esperar_resultado "F3 1 recurso, 2 digitos" "$tmp/esperado" 20

echo
echo "resultado: $ok ok, $falha falha"
[ "$falha" -eq 0 ]
