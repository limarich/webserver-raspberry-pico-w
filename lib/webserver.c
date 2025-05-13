#include "webserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/adc.h"
#include "pico/stdlib.h"

// Definição dos pinos dos LEDs
#define LED_BLUE_PIN 12
#define LED_GREEN_PIN 11
#define LED_RED_PIN 13
#define LED_PIN CYW43_WL_GPIO_LED_PIN

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

float internal_temperature = 0, dht_temperature = 0, dht_humidity = 0;

int alarm_active = 0;
int light_mode_active = 0;
int led_manual_state = 0;

// Inicializa o servidor web
void webserver_init(void)
{
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return;
    }

    // Vincula o servidor à porta 80 (HTTP padrão)
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return;
    }

    // Coloca o servidor em modo de escuta
    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);

    printf("Servidor ouvindo na porta 80\n");
}

static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    printf("Dados enviados com sucesso!\n");
    tcp_shutdown(tpcb, 1, 1);
    return ERR_OK;
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    printf("Conexão TCP aceita\n");
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Leitura da temperatura interna
float temp_read(void)
{
    adc_select_input(4);
    uint16_t raw_value = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
    return temperature;
}

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Aloca o request na memória dinâmica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento do request - Controle dos LEDs
    user_request(tpcb, &request);

    // Leitura da temperatura interna
    float internal_temperature = temp_read();

    // Cria a resposta HTML
    char html[4096];

    float dht_temperature = 0, dht_humidity = 0;

    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<title>Casa Inteligente</title>\n"
             "<style>\n"
             "body { background-color: #f0f4f8; font-family: Arial, sans-serif; text-align: center; margin-top: 50px; color: #333; }\n"
             "h1 { font-size: 48px; color: #444; }\n"
             ".section { margin-bottom: 30px; padding: 20px; background-color: #ffffff; border-radius: 15px; box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1); display: inline-block; width: 80%; max-width: 500px; }\n"
             "button { background-color: #007bff; color: #fff; font-size: 20px; margin: 10px; padding: 15px 30px; border: none; border-radius: 10px; cursor: pointer; }\n"
             "button.alarm { background-color: #dc3545; }\n"
             "button.active { background-color: #28a745; }\n"
             ".data { font-size: 24px; margin-top: 15px; color: #555; }\n"
             ".label { font-weight: bold; margin-right: 10px; }\n"
             "</style>\n"
             "</head>\n"
             "<body>\n"
             "<h1>Casa Inteligente</h1>\n"

             "<div class=\"section\">\n"
             "<h2>Raspberry Pico W</h2>\n"
             "<p><span class=\"label\">Temperatura Interna:</span><span id=\"internalTemp\">%.2f</span> °C</p>\n"
             "</div>\n"

             "<div class=\"section\">\n"
             "<h2>Sensores de Ambiente</h2>\n"
             "<p><span class=\"label\">Temperatura:</span><span id=\"dhtTemp\">%.2f</span> °C</p>\n"
             "<p><span class=\"label\">Umidade:</span><span id=\"dhtHum\">%.2f</span> %%</p>\n"
             "</div>\n"

             "<div class=\"section\">\n"
             "<h2>Controle de Alarme</h2>\n"
             "<form action=\"/toggleAlarm\"><button id=\"alarmButton\" class=\"alarm\">Ativar Alarme</button></form>\n"
             "</div>\n"

             "<div class=\"section\">\n"
             "<h2>Controle de Luz</h2>\n"
             "<form action=\"/toggleLightMode\"><button id=\"lightModeButton\">Modo Automático</button></form>\n"
             "</div>\n"

             "</body>\n"
             "</html>\n",
             internal_temperature, dht_temperature, dht_humidity);

    // Envia a resposta
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    // Libera a memória alocada
    free(request);
    pbuf_free(p);

    return ERR_OK;
}

int send_http_response(struct tcp_pcb *tpcb, const char *response)
{
    err_t err = tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK)
    {
        printf("Erro ao enviar resposta: %d\n", err);
        tcp_abort(tpcb);
        return ERR_ABRT;
    }

    tcp_output(tpcb);
    return ERR_OK;
}

// Tratamento do request do usuário
void user_request(struct tcp_pcb *tpcb, char **request)
{
    if (strstr(*request, "GET /blue_on") != NULL)
    {
        gpio_put(LED_BLUE_PIN, 1);
    }
    else if (strstr(*request, "GET /blue_off") != NULL)
    {
        gpio_put(LED_BLUE_PIN, 0);
    }
    else if (strstr(*request, "GET /green_on") != NULL)
    {
        gpio_put(LED_GREEN_PIN, 1);
    }
    else if (strstr(*request, "GET /green_off") != NULL)
    {
        gpio_put(LED_GREEN_PIN, 0);
    }
    else if (strstr(*request, "GET /red_on") != NULL)
    {
        gpio_put(LED_RED_PIN, 1);
    }
    else if (strstr(*request, "GET /red_off") != NULL)
    {
        gpio_put(LED_RED_PIN, 0);
    }
    else if (strstr(*request, "GET /sensorData") != NULL)
    {
        float internal_temperature = temp_read();
        float dht_temperature = 25.0;
        float dht_humidity = 60.0;

        char json_response[256];
        snprintf(json_response, sizeof(json_response),
                 "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
                 "{\"internal_temperature\": %.2f, \"dht_temperature\": %.2f, \"dht_humidity\": %.2f}\r\n",
                 internal_temperature, dht_temperature, dht_humidity);

        send_http_response(tpcb, json_response);
        printf("Dados dos sensores enviados\n");
    }
};