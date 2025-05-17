#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"

// Definição das credenciais Wi-Fi
extern char *WIFI_SSID;
extern char *WIFI_PASSWORD;

typedef struct
{
    char ssid[32];
    int8_t signal_quality;
    uint8_t channel;
    char ip_address[16];
} wifi_info_t;

// Funções para inicializar e gerenciar a conexão Wi-Fi
int wifi_init(void);
void wifi_poll(void);
void wifi_disconnect(void);
wifi_info_t wifi_get_info(void);

#endif // WIFI_H
