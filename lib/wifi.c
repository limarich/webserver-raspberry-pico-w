#include "wifi.h"
#include <stdio.h>

// Ajustar com as credenciais corretas
char *WIFI_SSID = "";
char *WIFI_PASSWORD = "";

int wifi_init(void)
{
    // Inicializa a arquitetura CYW43 (Wi-Fi e Bluetooth)
    if (cyw43_arch_init())
    {
        printf("Erro na inicialização do Wi-Fi\n");
        return -1;
    }

    // Habilita o modo cliente (station mode)
    cyw43_arch_enable_sta_mode();

    // Tenta conectar ao Wi-Fi
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0)
    {
        printf("Tentando conexão...\n");
    }

    printf("Conectado com sucesso! \n");
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    return 0;
}

void wifi_poll(void)
{
    // Mantém o Wi-Fi ativo
    cyw43_arch_poll();
}

void wifi_disconnect(void)
{
    // Desconecta e desliga o Wi-Fi
    cyw43_arch_deinit();
}
