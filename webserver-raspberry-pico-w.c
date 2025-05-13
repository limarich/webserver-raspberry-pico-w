#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/bootrom.h"
#include "hardware/adc.h"

#include "lwipopts.h"
#include "lwip/pbuf.h"  // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"   // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h" // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

// bibliotecas customizadas
#include "lib/wifi.h"
#include "lib/webserver.h"
#include "lib/dht11.h"
#include "lib/buzzer.h"

#define BUTTON_B 6           // botão B
#define BUZZER_A 10          // PORTA DO BUZZER A
#define BUZZER_B 21          // PORTA DO BUZZER B
#define BUZZER_FREQUENCY 200 // FREQUENCIA DO BUZZER

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN // GPIO do CI CYW43
#define LED_BLUE_PIN 12               // GPIO12 - LED azul
#define LED_GREEN_PIN 11              // GPIO11 - LED verde
#define LED_RED_PIN 13                // GPIO13 - LED vermelho

// inicializa um led
void init_led(uint pin);

void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

int main()
{

    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // inicializa os leds
    init_led(LED_BLUE_PIN);
    init_led(LED_RED_PIN);
    init_led(LED_GREEN_PIN);

    // inicializa o sistema de entrada e saída padrão
    stdio_init_all();
    // inicializa a arquitetura cyw43 (wifi e bluetooth) e faz a conexão com a rede
    wifi_init();
    // incializar o webserver
    webserver_init();
    // inicializa o sensor
    dht11_init(28);

    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);

    // INCIALIZA OS BUZZERS
    initialization_buzzers(BUZZER_A, BUZZER_B);

    float temperature = 0;
    float humidity = 0;
    while (true)
    {
        wifi_poll();   // Mantém a conexão Wi-Fi ativa
        sleep_ms(100); // Reduz o uso da CPU

        if (dht11_read(&temperature, &humidity))
        {
            printf("Temperatura: %.2f°C, Umidade: %.2f%%\n", temperature, humidity);
        }
        else
        {
            printf("Erro ao ler o sensor DHT11\n");
        }

        buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 100);
        sleep_ms(200);
    }

    // Desligar a arquitetura CYW43.
    wifi_disconnect();
    return 0;
}

// inicializa um led
void init_led(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, false);
}