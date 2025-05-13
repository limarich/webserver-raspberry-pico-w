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

/// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Função de callback ao enviar dados
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);

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
    tcp_close(tpcb); // fecha a conexão após o envio da resposta
    return ERR_OK;
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    printf("Conexão TCP aceita\n");
    tcp_recv(newpcb, tcp_server_recv);   // callback para montar a página html
    tcp_sent(newpcb, tcp_sent_callback); // callback para envio
    return ERR_OK;
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

    // Aloca o request na memória dinâmica para o request e verifica se a alocação foi bem-sucedida
    char *request = (char *)malloc(p->len + 1);
    if (!request)
    {
        printf("Falha ao alocar memória para o request\n");
        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);
        return ERR_MEM;
    }
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento do request - Controle dos LEDs
    user_request(tpcb, &request);

    // Leitura da temperatura interna
    // float internal_temperature = temp_read();

    // Cria a resposta HTML
    char html[1024 * 2];

    // textos dos botões
    const char *blue_led_button_text = blue_led_state ? "Desligar Led Azul" : "Ativar Led Azul";
    const char *green_led_button_text = green_led_state ? "Desligar Led Verde" : "Ativar Led Verde";
    const char *red_led_button_text = red_led_state ? "Desligar Led Vermelho" : "Ativar Led Vermelho";
    const char *alarm_button_text = is_alarm_enabled ? "Desativar Alarme" : "Ativar Alarme";

    // página html
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
             "button.red { background-color: #dc3545; }\n"
             "button.green { background-color: #28a745; }\n"
             "button.purple { background-color: #800080; }\n"
             ".label { font-weight: bold; margin-right: 10px; }\n"
             "</style>\n"
             "</head>\n"
             "<body>\n"
             "<h1>Casa Inteligente</h1>\n"
             "<form action=\"./toggle_blue\"><button >%s</button></form>"
             "<form action=\"./toggle_green\"><button class='green'>%s</button></form>"
             "<form action=\"./toggle_red\"><button class='red'>%s</button></form>"
             "<form action=\"./toogle_alarm\"><button class='purple'>%s</button></form>"
             "<div class=\"section\">\n"
             "<h2>Sensores de Ambiente</h2>\n"
             "<p><span class=\"label\">Temperatura:</span><span id=\"dhtTemp\">%.2f</span> °C</p>\n"
             "<p><span class=\"label\">Umidade:</span><span id=\"dhtHum\">%.2f</span> %%</p>\n"
             "</div>\n"
             "<p><span class=\"label\">Temperatura interna:</span><span id=\"internal_temp\">%.2f</span></p>\n"
             "</body>\n"
             "</html>\n",
             blue_led_button_text, green_led_button_text, red_led_button_text, alarm_button_text, dht_temperature, dht_humidity, internal_temperature);

    // Envia a resposta
    err_t send_err = tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    if (send_err != ERR_OK) // caso ocorra um erro ao gravar a página exibe mensagem, libera memória alocada e aborta a conexão
    {
        printf("Erro ao enviar resposta: %d\n", send_err);
        free(request);
        pbuf_free(p);
        tcp_abort(tpcb);
        return send_err;
    }
    tcp_output(tpcb);

    // Marca os dados como lidos para liberar o buffer do TCP
    tcp_recved(tpcb, p->tot_len);
    free(request);
    pbuf_free(p);

    return ERR_OK;
}

// Tratamento do request do usuário
void user_request(struct tcp_pcb *tpcb, char **request)
{
    // definição das rotas do sistema
    if (strstr(*request, "GET /toggle_blue") != NULL)
    {
        blue_led_state = !blue_led_state;
        gpio_put(LED_BLUE_PIN, blue_led_state);
    }
    else if (strstr(*request, "GET /toggle_green") != NULL)
    {
        green_led_state = !green_led_state;
        gpio_put(LED_GREEN_PIN, green_led_state);
    }
    else if (strstr(*request, "GET /toggle_red") != NULL)
    {
        red_led_state = !red_led_state;
        gpio_put(LED_RED_PIN, red_led_state);
    }
    else if (strstr(*request, "GET /toogle_alarm") != NULL)
    {
        is_alarm_enabled = !is_alarm_enabled;
    }
};