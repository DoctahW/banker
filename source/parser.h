#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdio.h>
#include "erro.h"

typedef enum { CMD_RQ, CMD_RL, CMD_ESTRELA } TipoCmd;

typedef struct {
    TipoCmd tipo;
    int cliente;
} Comando;

typedef struct {
    Comando *cmd;
    int *vals;
    int n;
} ListaComandos;

// Em erro, cada leitor libera o que alocou, deixa a saida NULL/zerada e escreve
// em erro "linha N: ..." (o main prefixa o nome do arquivo).
ErroParse le_recursos(int argc, char **argv, int **total_out, int *n_rec_out,
                      char *erro, size_t erro_sz);
ErroParse le_clientes(FILE *arquivo, int n_rec, int **maximo_out, int *n_cli_out,
                      char *erro, size_t erro_sz);
ErroParse le_comandos(FILE *arquivo, int n_rec, int n_cli, ListaComandos *out,
                      char *erro, size_t erro_sz);
void      libera_comandos(ListaComandos *lista);

#endif
