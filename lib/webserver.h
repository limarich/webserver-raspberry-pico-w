#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdint.h>
#include "lwip/tcp.h"

// Variável global para controle do alarme
extern volatile bool is_alarm_enabled;
extern volatile float dht_temperature;
extern volatile float internal_temperature;
extern volatile float dht_humidity;
extern volatile bool green_led_state;
extern volatile bool blue_led_state;
extern volatile bool red_led_state;

// Inicializa o servidor web
void webserver_init(void);

// Função para leitura da temperatura interna
float temp_read(void);

// Tratamento do request do usuário
void user_request(struct tcp_pcb *tpcb, char **request);

#endif // WEBSERVER_H
