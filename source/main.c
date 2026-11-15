#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

#define ARQ_CLIENTES "customer.txt"
#define ARQ_COMANDOS "commands.txt"

static bool carrega_recursos(int argc, char **argv, int **total, int *n_rec) {
    char msg[256];
    ErroParse resultado = le_recursos(argc, argv, total, n_rec, msg, sizeof msg);
    if (resultado != OK) {
        fprintf(stderr, "Erro: %s\n", msg);
        return false;
    }
    return true;
}

static bool carrega_clientes(int n_rec, int **maximo, int *n_cli) {
    FILE *arquivo = fopen(ARQ_CLIENTES, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s': %s\n", ARQ_CLIENTES, strerror(errno));
        return false;
    }

    char msg[256];
    ErroParse resultado = le_clientes(arquivo, n_rec, maximo, n_cli, msg, sizeof msg);
    fclose(arquivo);
    if (resultado != OK) {
        fprintf(stderr, "Erro: %s: %s\n", ARQ_CLIENTES, msg);
        return false;
    }
    return true;
}

static bool carrega_comandos(int n_rec, int n_cli, ListaComandos *comandos) {
    FILE *arquivo = fopen(ARQ_COMANDOS, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s': %s\n", ARQ_COMANDOS, strerror(errno));
        return false;
    }

    char msg[256];
    ErroParse resultado = le_comandos(arquivo, n_rec, n_cli, comandos, msg, sizeof msg);
    fclose(arquivo);
    if (resultado != OK) {
        fprintf(stderr, "Erro: %s: %s\n", ARQ_COMANDOS, msg);
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <instancias do recurso 0> [instancias do recurso 1 ...]\n"
                        "Exemplo: %s 10 5 7\n", argv[0], argv[0]);
        return 1;
    }

    int *total = NULL;
    int *maximo = NULL;
    int n_rec = 0;
    int n_cli = 0;
    ListaComandos comandos = {0};

    // valida tudo antes de executar qualquer comando
    bool ok = carrega_recursos(argc, argv, &total, &n_rec)
           && carrega_clientes(n_rec, &maximo, &n_cli)
           && carrega_comandos(n_rec, n_cli, &comandos);

    libera_comandos(&comandos);
    free(maximo);
    free(total);
    return ok ? 0 : 1;
}
