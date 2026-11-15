#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

#define DELIM " \t\r\n"

// para_pos_int da atvd3, mas aceita zero (customer.txt tem "9,0,2")
static ErroParse para_nao_neg_int(const char *texto, int *saida){
    char *fim;
    errno = 0;
    long valor = strtol(texto, &fim, 10);

    if (fim == texto){
        return ERR_NAO_NUMERICO;
    }

    if (*fim != '\0') {
        return ERR_NAO_NUMERICO;
    }

    if (errno == ERANGE || valor > INT_MAX){
        return ERR_OVERFLOW;
    }

    if (valor < 0){
        return ERR_NEGATIVO;
    }

    *saida = (int)valor;
    return OK;
}

// igual a da atvd1/atvd3, so com '\r' nos delimitadores (arquivo vindo do Windows).
// Para em quant - 1 sem avisar que sobrou palavra: quem chama dimensiona com folga.
static int dividir_palavras(char *linha, char **palavras, int quant) {
    if (quant <= 0) {
        return 0;
    }
    char *saveptr;
    char *token = strtok_r(linha, DELIM, &saveptr);
    int n = 0;

    while (token != NULL && n < quant - 1) {
        palavras[n] = token;
        n++;
        token = strtok_r(NULL, DELIM, &saveptr);
    }

    palavras[n] = NULL;
    return n;
}

static int eh_espaco(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// tira espaco, \r e \n das duas pontas, no proprio buffer
static char *aparar(char *texto) {
    while (eh_espaco(*texto)) {
        texto++;
    }
    char *fim = texto + strlen(texto);
    while (fim > texto && eh_espaco(fim[-1])) {
        fim--;
    }
    *fim = '\0';
    return texto;
}

// devolve o bloco com espaco para mais um item de tam_item bytes (a capacidade
// dobra); NULL se faltar memoria, e nesse caso o bloco antigo continua valido
static void *cresce(void *bloco, int *capacidade, int usados, size_t tam_item) {
    if (usados < *capacidade) {
        return bloco;
    }
    if (*capacidade > INT_MAX / 2) {
        return NULL;
    }
    int nova = *capacidade ? *capacidade * 2 : 8;
    if ((size_t)nova > SIZE_MAX / tam_item) {
        return NULL;
    }
    void *novo = realloc(bloco, (size_t)nova * tam_item);
    if (novo == NULL) {
        return NULL;
    }
    *capacidade = nova;
    return novo;
}

// getline devolve -1 tanto no fim do arquivo quanto em falha; separa os dois.
// Ler um diretorio cai em ferror; falta de memoria nao marca nem eof nem erro.
static ErroParse fim_de_leitura(FILE *arquivo) {
    if (feof(arquivo)) {
        return OK;
    }
    if (ferror(arquivo)) {
        return ERR_LEITURA;
    }
    return ERR_MEMORIA;
}

// "7,5,3" -> destino[0..n_rec-1]. strtok_r fundiria ",," num separador so,
// entao corta campo a campo: campo vazio pega "7,,3", ",7,3" e "7,5,3,".
static ErroParse le_linha_cliente(char *conteudo, int n_rec, int *destino, int nlinha,
                                  char *erro, size_t erro_sz) {
    int n_campos = 0;
    char *inicio = conteudo;

    while (inicio != NULL) {
        char *virgula = strchr(inicio, ',');
        if (virgula != NULL) {
            *virgula = '\0';
        }
        char *campo = aparar(inicio);
        n_campos++;

        if (*campo == '\0') {
            snprintf(erro, erro_sz, "linha %d, recurso %d: %s",
                     nlinha, n_campos, erro_msg(ERR_CAMPO_VAZIO));
            return ERR_CAMPO_VAZIO;
        }

        int valor;
        ErroParse resultado = para_nao_neg_int(campo, &valor);
        if (resultado != OK) {
            snprintf(erro, erro_sz, "linha %d, recurso %d ('%s'): %s",
                     nlinha, n_campos, campo, erro_msg(resultado));
            return resultado;
        }
        if (n_campos <= n_rec) {
            destino[n_campos - 1] = valor;
        }

        inicio = (virgula != NULL) ? virgula + 1 : NULL;
    }

    if (n_campos != n_rec) {
        snprintf(erro, erro_sz, "linha %d: %s (%d na linha, %d na linha de comando)",
                 nlinha, erro_msg(ERR_N_RECURSOS), n_campos, n_rec);
        return ERR_N_RECURSOS;
    }
    return OK;
}

// palavras de "RQ c r1..rn" / "RL c r1..rn" / "*" -> comando + valores[0..n_rec-1]
static ErroParse le_linha_comando(char **palavras, int n_palavras, int n_rec, int n_cli,
                                  Comando *comando, int *valores, int nlinha,
                                  char *erro, size_t erro_sz) {
    if (strcmp(palavras[0], "RQ") == 0) {
        comando->tipo = CMD_RQ;
    } else if (strcmp(palavras[0], "RL") == 0) {
        comando->tipo = CMD_RL;
    } else if (strcmp(palavras[0], "*") == 0) {
        comando->tipo = CMD_ESTRELA;
    } else {
        snprintf(erro, erro_sz, "linha %d ('%s'): %s",
                 nlinha, palavras[0], erro_msg(ERR_CMD_DESCONHECIDO));
        return ERR_CMD_DESCONHECIDO;
    }

    if (comando->tipo == CMD_ESTRELA) {
        if (n_palavras != 1) {
            snprintf(erro, erro_sz, "linha %d: %s", nlinha, erro_msg(ERR_ESTRELA_ARGS));
            return ERR_ESTRELA_ARGS;
        }
        comando->cliente = -1;
        memset(valores, 0, (size_t)n_rec * sizeof *valores);
        return OK;
    }

    if (n_palavras != n_rec + 2) {
        int no_comando = n_palavras >= 2 ? n_palavras - 2 : 0;
        snprintf(erro, erro_sz, "linha %d: %s (%d%s no comando, %d na linha de comando)",
                 nlinha, erro_msg(ERR_N_RECURSOS), no_comando,
                 n_palavras > n_rec + 2 ? " ou mais" : "", n_rec);
        return ERR_N_RECURSOS;
    }

    ErroParse resultado = para_nao_neg_int(palavras[1], &comando->cliente);
    if (resultado != OK) {
        snprintf(erro, erro_sz, "linha %d, cliente ('%s'): %s",
                 nlinha, palavras[1], erro_msg(resultado));
        return resultado;
    }
    if (comando->cliente >= n_cli) {
        snprintf(erro, erro_sz, "linha %d, cliente %d: %s (customer.txt tem %d, de 0 a %d)",
                 nlinha, comando->cliente, erro_msg(ERR_CLIENTE_INVALIDO), n_cli, n_cli - 1);
        return ERR_CLIENTE_INVALIDO;
    }

    for (int j = 0; j < n_rec; j++) {
        resultado = para_nao_neg_int(palavras[j + 2], &valores[j]);
        if (resultado != OK) {
            snprintf(erro, erro_sz, "linha %d, recurso %d ('%s'): %s",
                     nlinha, j + 1, palavras[j + 2], erro_msg(resultado));
            return resultado;
        }
    }
    return OK;
}

ErroParse le_recursos(int argc, char **argv, int **total_out, int *n_rec_out,
                      char *erro, size_t erro_sz) {
    *total_out = NULL;
    *n_rec_out = 0;

    int n_rec = argc - 1;
    int *total = malloc((size_t)n_rec * sizeof *total);
    if (total == NULL) {
        snprintf(erro, erro_sz, "%s", erro_msg(ERR_MEMORIA));
        return ERR_MEMORIA;
    }

    for (int i = 0; i < n_rec; i++) {
        ErroParse resultado = para_nao_neg_int(argv[i + 1], &total[i]);
        if (resultado != OK) {
            snprintf(erro, erro_sz, "argumento %d ('%s'): %s",
                     i + 1, argv[i + 1], erro_msg(resultado));
            free(total);
            return resultado;
        }
    }

    *total_out = total;
    *n_rec_out = n_rec;
    return OK;
}

ErroParse le_clientes(FILE *arquivo, int n_rec, int **maximo_out, int *n_cli_out,
                      char *erro, size_t erro_sz) {
    *maximo_out = NULL;
    *n_cli_out = 0;

    char *linha = NULL;
    size_t linha_cap = 0;
    int *maximo = NULL;
    int capacidade = 0;
    int n_cli = 0;
    int nlinha = 0;
    ErroParse resultado = OK;

    while (resultado == OK && getline(&linha, &linha_cap, arquivo) != -1) {
        nlinha++;

        char *conteudo = aparar(linha);
        if (*conteudo == '\0') {
            continue;
        }

        int *maior = cresce(maximo, &capacidade, n_cli, (size_t)n_rec * sizeof *maximo);
        if (maior == NULL) {
            resultado = ERR_MEMORIA;
            snprintf(erro, erro_sz, "linha %d: %s", nlinha, erro_msg(resultado));
            break;
        }
        maximo = maior;

        int *destino = maximo + (size_t)n_cli * (size_t)n_rec;
        resultado = le_linha_cliente(conteudo, n_rec, destino, nlinha, erro, erro_sz);
        if (resultado == OK) {
            n_cli++;   // so conta a linha depois de valida
        }
    }

    if (resultado == OK) {
        resultado = fim_de_leitura(arquivo);
        if (resultado != OK) {
            snprintf(erro, erro_sz, "linha %d: %s", nlinha + 1, erro_msg(resultado));
        }
    }

    if (resultado == OK && n_cli == 0) {
        resultado = ERR_SEM_CLIENTES;
        snprintf(erro, erro_sz, "%s", erro_msg(resultado));
    }

    free(linha);
    if (resultado != OK) {
        free(maximo);
        return resultado;
    }
    *maximo_out = maximo;
    *n_cli_out = n_cli;
    return OK;
}

ErroParse le_comandos(FILE *arquivo, int n_rec, int n_cli, ListaComandos *out,
                      char *erro, size_t erro_sz) {
    out->cmd = NULL;
    out->vals = NULL;
    out->n = 0;

    // n_rec + 2 palavras validas ("RQ c r1..rn") + 1 para notar palavra sobrando + NULL
    int quant = n_rec + 4;
    char **palavras = malloc((size_t)quant * sizeof *palavras);
    if (palavras == NULL) {
        snprintf(erro, erro_sz, "%s", erro_msg(ERR_MEMORIA));
        return ERR_MEMORIA;
    }

    char *linha = NULL;
    size_t linha_cap = 0;
    Comando *comandos = NULL;
    int *valores = NULL;
    int cap_comandos = 0;
    int cap_valores = 0;
    int n_cmd = 0;
    int nlinha = 0;
    ErroParse resultado = OK;

    while (resultado == OK && getline(&linha, &linha_cap, arquivo) != -1) {
        nlinha++;

        int n_palavras = dividir_palavras(linha, palavras, quant);
        if (n_palavras == 0) {
            continue;
        }

        Comando *mais_comandos = cresce(comandos, &cap_comandos, n_cmd, sizeof *comandos);
        if (mais_comandos != NULL) {
            comandos = mais_comandos;
        }
        int *mais_valores = cresce(valores, &cap_valores, n_cmd, (size_t)n_rec * sizeof *valores);
        if (mais_valores != NULL) {
            valores = mais_valores;
        }
        if (mais_comandos == NULL || mais_valores == NULL) {
            resultado = ERR_MEMORIA;
            snprintf(erro, erro_sz, "linha %d: %s", nlinha, erro_msg(resultado));
            break;
        }

        int *destino = valores + (size_t)n_cmd * (size_t)n_rec;
        resultado = le_linha_comando(palavras, n_palavras, n_rec, n_cli,
                                     &comandos[n_cmd], destino, nlinha, erro, erro_sz);
        if (resultado == OK) {
            n_cmd++;
        }
    }

    if (resultado == OK) {
        resultado = fim_de_leitura(arquivo);
        if (resultado != OK) {
            snprintf(erro, erro_sz, "linha %d: %s", nlinha + 1, erro_msg(resultado));
        }
    }

    free(linha);
    free(palavras);
    if (resultado != OK) {
        free(comandos);
        free(valores);
        return resultado;
    }
    out->cmd = comandos;
    out->vals = valores;
    out->n = n_cmd;
    return OK;
}

void libera_comandos(ListaComandos *lista) {
    free(lista->cmd);
    free(lista->vals);
    lista->cmd = NULL;
    lista->vals = NULL;
    lista->n = 0;
}
