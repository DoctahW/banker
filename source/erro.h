#ifndef ERRO_H
#define ERRO_H

// mesmo padrão de erro do ProcessFlow e Scheduler
typedef enum ErroParse {
    OK=0,
    ERR_NAO_NUMERICO,
    ERR_NEGATIVO,
    ERR_OVERFLOW,
    ERR_SEM_CLIENTES,
    ERR_N_RECURSOS,
    ERR_CAMPO_VAZIO,
    ERR_CMD_DESCONHECIDO,
    ERR_CLIENTE_INVALIDO,
    ERR_ESTRELA_ARGS,
    ERR_LEITURA,
    ERR_MEMORIA
} ErroParse;

const char *erro_msg(ErroParse e);

#endif
