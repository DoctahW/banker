#include "erro.h"

// replica da atvd1 e 3
const char *erro_msg(ErroParse e){
    switch (e) {
        case OK:                   return "sem erro";
        case ERR_NAO_NUMERICO:     return "valor nao e um inteiro valido";
        case ERR_NEGATIVO:         return "valor nao pode ser negativo";
        case ERR_OVERFLOW:         return "valor grande demais";
        case ERR_LINHA_LONGA:      return "linha comprida demais para o buffer de leitura";
        case ERR_SEM_CLIENTES:     return "o arquivo nao define nenhum cliente";
        case ERR_N_RECURSOS:       return "quantidade de recursos diferente da passada na linha de comando";
        case ERR_CAMPO_VAZIO:      return "campo vazio (virgula sobrando ou faltando valor)";
        case ERR_CMD_DESCONHECIDO: return "comando desconhecido (use RQ, RL ou *)";
        case ERR_CLIENTE_INVALIDO: return "cliente inexistente";
        case ERR_ESTRELA_ARGS:     return "o comando '*' nao recebe argumentos";
        case ERR_MEMORIA:          return "memoria insuficiente";
    }
    return "motivo desconhecido";
}
