#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdint.h>
#include "lwip/tcp.h"

// Inicializa o servidor web
void webserver_init(void);

// Função para leitura da temperatura interna
float temp_read(void);

// Tratamento do request do usuário
void user_request(struct tcp_pcb *tpcb, char **request);

#endif // WEBSERVER_H
