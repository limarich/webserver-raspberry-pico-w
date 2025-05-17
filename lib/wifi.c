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

wifi_info_t wifi_get_info(void)
{
    wifi_info_t info;
    memset(&info, 0, sizeof(info)); // Zera a estrutura

    // Obtém o SSID da rede conectada
    strncpy(info.ssid, WIFI_SSID, sizeof(info.ssid) - 1);

    // Obtém a qualidade do sinal
    int8_t rssi = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    if (rssi != CYW43_LINK_DOWN)
    {
        // Usa o valor bruto do RSSI em dBm
        info.signal_quality = rssi;
    }
    else
    {
        // Define -100 dBm como sem sinal
        info.signal_quality = -100;
    }

    // Obtém o canal atual (simulado)
    info.channel = 11; // Aqui você pode usar outra função se disponível para pegar o canal real

    // Obtém o endereço IP
    if (netif_default)
    {
        snprintf(info.ip_address, sizeof(info.ip_address), "%s", ipaddr_ntoa(&netif_default->ip_addr));
    }
    else
    {
        strcpy(info.ip_address, "0.0.0.0");
    }

    return info;
}
