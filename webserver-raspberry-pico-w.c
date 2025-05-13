#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/bootrom.h"
#include "hardware/adc.h"

#include "lwipopts.h"
#include "lwip/pbuf.h"  // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"   // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h" // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

// bibliotecas customizadas
#include "lib/wifi.h"      // configuração da conexão com wifi
#include "lib/webserver.h" // configuração do webserver
#include "lib/dht11.h"     // leitura do sensor dht11
#include "lib/buzzer.h"    // controle do buzzer

#define BUTTON_B 6           // botão B para o modo bootsel
#define BUZZER_A 10          // PORTA DO BUZZER A
#define BUZZER_B 21          // PORTA DO BUZZER B
#define BUZZER_FREQUENCY 200 // FREQUENCIA DO BUZZER
#define DHT11_PIN 28         // Pino do sensor DHT11

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN // GPIO do CI CYW43
#define LED_BLUE_PIN 12               // GPIO12 - LED azul
#define LED_GREEN_PIN 11              // GPIO11 - LED verde
#define LED_RED_PIN 13                // GPIO13 - LED vermelho
#define DEBOUNCE_DELAY 200            // tempo mínimo de debounce do botão

#define DHTPIN 28 // PINO do sensor de umidade e temperatura

volatile bool is_alarm_enabled = false; // variável global para o controle do alarme
// variável global para a leitura de temperatura e umidade do sensor
volatile float dht_temperature = 0;
volatile float dht_humidity = 0;
volatile float internal_temperature = 0;
// controle dos leds
volatile bool green_led_state = false;
volatile bool blue_led_state = false;
volatile bool red_led_state = false;
volatile uint32_t last_button_press_time = 0; // tempo de debounce do botão
// inicializa um led
void init_led(uint pin);
// Leitura da temperatura interna
float internal_temp_read(void);

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if ((current_time - last_button_press_time) > DEBOUNCE_DELAY)
    {
        // Executa a ação do botão (modo BOOTSEL)
        reset_usb_boot(0, 0);
        last_button_press_time = current_time;
    }
}

int main()
{

    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // inicializa os leds
    init_led(LED_BLUE_PIN);  // led azul
    init_led(LED_RED_PIN);   // led vermelho
    init_led(LED_GREEN_PIN); // led verde

    // inicializa o sistema de entrada e saída padrão
    stdio_init_all();
    // inicializa a arquitetura cyw43 (wifi e bluetooth) e faz a conexão com a rede
    wifi_init();
    // incializar o webserver
    webserver_init();
    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true); // habilita o sensor de temperatura
    // inicializa a porta do sensor dht11
    dht11_init(DHT11_PIN);
    // INCIALIZA OS BUZZERS
    initialization_buzzers(BUZZER_A, BUZZER_B);

    while (true)
    {
        wifi_poll();   // Mantém a conexão Wi-Fi ativa
        sleep_ms(100); // Reduz o uso da CPU

        // em caso de leitura, exibe a informação
        // caso a leitura do sensor retorne 0, ocorreu algum erro
        if (dht11_read(&dht_temperature, &dht_humidity))
        {
            printf("Temperatura: %.2f°C, Umidade: %.2f%%\n", dht_temperature, dht_humidity);
        }
        else
        {
            printf("Erro ao ler o sensor DHT11\n");
        }

        // le a temperatura interna
        internal_temp_read();

        if (is_alarm_enabled) // alarme
        {
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 100);
            sleep_ms(200);
        }
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

// Leitura da temperatura interna
float internal_temp_read(void)
{
    adc_select_input(4);
    uint16_t raw_value = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    internal_temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
}