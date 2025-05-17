#include "pico/stdlib.h"     // Funções padrão para controle do Raspberry Pi Pico
#include "pico/cyw43_arch.h" // Controle do Wi-Fi e Bluetooth
#include "pico/bootrom.h"    // Para reiniciar o dispositivo em modo USB
#include "hardware/adc.h"    // Controle do conversor AD (temperatura interna)
#include "hardware/i2c.h"    // Controle do barramento I2C

#include "lwipopts.h"   // Opções do stack IP (configuração da rede)
#include "lwip/pbuf.h"  // Manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"   // Funções para manipular conexões TCP
#include "lwip/netif.h" // Controle das interfaces de rede

// Bibliotecas customizadas para sensores e periféricos
#include "lib/wifi.h"      // Controle da conexão Wi-Fi
#include "lib/webserver.h" // Configuração do servidor web
#include "lib/dht11.h"     // Leitura do sensor DHT11 (umidade e temperatura)
#include "lib/buzzer.h"    // Controle do buzzer
#include "lib/ssd1306.h"   // Controle do display OLED SSD1306
#include "lib/font.h"      // Fontes para o display

// Pinos dos botões
#define BUTTON_A 5 // Botão A (controle do display)
#define BUTTON_B 6 // Botão B (modo BOOTSEL)

// Pinos dos buzzers
#define BUZZER_A 10          // Pino do Buzzer A
#define BUZZER_B 21          // Pino do Buzzer B
#define BUZZER_FREQUENCY 200 // Frequência do Buzzer

// Pino do sensor DHT11
#define DHT11_PIN 28 // Pino do sensor de umidade e temperatura

// Pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN // LED embutido no módulo CYW43
#define LED_BLUE_PIN 12               // LED azul
#define LED_GREEN_PIN 11              // LED verde
#define LED_RED_PIN 13                // LED vermelho

// Parâmetros do barramento I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C // Endereço do display SSD1306 no barramento I2C

// Tempo mínimo de debounce para os botões
#define DEBOUNCE_DELAY 200

// Controle do estado do alarme
volatile bool is_alarm_enabled = false;

// Leituras de temperatura e umidade
volatile float dht_temperature = 0;
volatile float dht_humidity = 0;
volatile float internal_temperature = 0;

// limite para temperatura máxima do sistema
volatile float temperature_limit = 25;

// Estado dos LEDs
volatile bool green_led_state = false;
volatile bool blue_led_state = false;
volatile bool red_led_state = false;

// Controle do debounce dos botões
volatile uint32_t last_button_press_time_a = 0;
volatile uint32_t last_button_press_time_b = 0;

// Modos de exibição possíveis
typedef enum
{
    SENSOR, // Exibe temperatura e umidade
    LEDS,   // Exibe estado dos LEDs
    WIFI    // Exibe informações da rede Wi-Fi
} display_mode_t;

// Define o modo inicial do display
display_mode_t display_mode = SENSOR;

// inicializa um led
void init_led(uint pin);
// Leitura da temperatura interna
float internal_temp_read(void);
// faz leituras médias do sensor para evitar possíveis flutuações
float read_dht11_average(float *temperature, float *humidity);

// Manipula as interrupções dos botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Tempo atual em ms

    // Verifica se o botão B foi pressionado (modo BOOTSEL)
    if (gpio == BUTTON_B)
    {
        if ((current_time - last_button_press_time_b) > DEBOUNCE_DELAY)
        {
            reset_usb_boot(0, 0); // Reinicia o dispositivo em modo BOOTSEL
            last_button_press_time_b = current_time;
        }
    }
    // Verifica se o botão A foi pressionado (troca de modo)
    else if (gpio == BUTTON_A)
    {
        if ((current_time - last_button_press_time_a) > DEBOUNCE_DELAY)
        {
            display_mode = (display_mode + 1) % 3; // Avança para o próximo modo (cicla entre 0 e 2)
            last_button_press_time_a = current_time;
        }
    }
}

int main()
{

    // Configura os botões com interrupção
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicializa os LEDs
    init_led(LED_BLUE_PIN);
    init_led(LED_RED_PIN);
    init_led(LED_GREEN_PIN);

    // Inicializa os sistemas principais
    stdio_init_all();                           // Serial
    wifi_init();                                // Wi-Fi
    webserver_init();                           // Servidor Web
    adc_init();                                 // Conversor ADC
    adc_set_temp_sensor_enabled(true);          // Ativa o sensor de temperatura interna
    dht11_init(DHT11_PIN);                      // Sensor DHT11
    initialization_buzzers(BUZZER_A, BUZZER_B); // Buzzers

    // Inicializa o barramento I2C para o display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Configura e limpa o display
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    while (true)
    {
        wifi_poll(); // Mantém a conexão Wi-Fi ativa

        // em caso de leitura, exibe a informação
        // caso a leitura do sensor retorne 0, ocorreu algum erro
        float avg_temperature = 0, avg_humidity = 0;
        int num_valid_reads = read_dht11_average(&avg_temperature, &avg_humidity);

        if (num_valid_reads > 0)
        {
            dht_temperature = avg_temperature;
            dht_humidity = avg_humidity;
            printf("Temperatura Média: %.2f°C, Umidade Média: %.2f%%\n", dht_temperature, dht_humidity);
        }
        else
        {
            printf("Erro ao ler o sensor DHT11\n");
        }

        // le a temperatura interna
        internal_temp_read();

        // limpa a tela do display
        ssd1306_fill(&ssd, false);

        if (dht_temperature > temperature_limit) // verifica antes se o limite da temperatura nao foi atingido
        {
            char temperature[16];
            // formata dados lidos pelo sensor para exibição
            snprintf(temperature, sizeof(temperature), "%.2f°C", dht_temperature);

            ssd1306_fill(&ssd, false);                                             // Limpa a tela
            ssd1306_send_data(&ssd);                                               // envia dados para display
            ssd1306_draw_string(&ssd, "SISTEMA", 0, 11);                           // Desenha uma string
            ssd1306_draw_string(&ssd, "SUPERAQUECIDO", 0, 20);                     // Desenha uma string
            ssd1306_draw_string(&ssd, temperature, WIDTH / 2 - 4, HEIGHT / 2 + 4); // Desenha uma string
            ssd1306_send_data(&ssd);                                               // envia dados para display
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY * 10, 50);
            sleep_ms(250);
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY * 5, 50);
            sleep_ms(250);
        }
        else if (is_alarm_enabled) // caso o alarme esteja ativado pisca a tela do display e ativa o buzzer para um toque curto
        {
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 100);
            ssd1306_fill(&ssd, true);  // Preenche a tela
            ssd1306_send_data(&ssd);   // envia dados para display
            sleep_ms(250);             // aguarda um tempo
            ssd1306_fill(&ssd, false); // Limpa a tela
            ssd1306_send_data(&ssd);   // envia dados para display
            sleep_ms(250);             // aguarda um tempo
        }
        else if (display_mode == SENSOR)
        {
            char humidity[16], temperature[16];
            // formata dados lidos pelo sensor para exibição
            snprintf(humidity, sizeof(humidity), "%.2f%%", dht_humidity);
            snprintf(temperature, sizeof(temperature), "%.2f°C", dht_temperature);

            ssd1306_rect(&ssd, 3, 3, 122, 60, true, false); // Desenha a borda
            ssd1306_draw_string(&ssd, "temp:", 8, 11);      // Desenha uma string
            ssd1306_draw_string(&ssd, temperature, 48, 11); // Desenha uma string
            ssd1306_draw_string(&ssd, "hum:", 8, 52);       // Desenha uma string
            ssd1306_draw_string(&ssd, humidity, 48, 52);    // Desenha uma string
            ssd1306_send_data(&ssd);                        // Atualiza o display
        }
        else if (display_mode == LEDS)
        {
            // desenha 3 círculos que representam se o led está aceso ou não dependendo do valor em cada pino
            // caso o led esteja aceso o circulo fica preenchido e caso contrário ele ficará vazio
            ssd1306_fill(&ssd, false);                                       // limpa a tela
            ssd1306_circle(&ssd, 38, 16, 12, true, gpio_get(LED_RED_PIN));   // Desenha um círculo
            ssd1306_draw_string(&ssd, "R", 38, 32);                          // Desenha uma string
            ssd1306_circle(&ssd, 66, 16, 12, true, gpio_get(LED_GREEN_PIN)); // Desenha um círculo
            ssd1306_draw_string(&ssd, "G", 66, 32);                          // Desenha uma string
            ssd1306_circle(&ssd, 94, 16, 12, true, gpio_get(LED_BLUE_PIN));  // Desenha um círculo
            ssd1306_draw_string(&ssd, "B", 94, 32);                          // Desenha uma string
            ssd1306_send_data(&ssd);                                         // Atualiza o display
        }
        else
        {

            char ssid[16], signal[16], channel[16], ip[16];
            // formata dados da rede para exibição
            wifi_info_t wifi_info = wifi_get_info();
            printf("SSID: %s\n", wifi_info.ssid);
            printf("Qualidade do Sinal: %d%%\n", wifi_info.signal_quality);
            printf("Canal: %d\n", wifi_info.channel);
            printf("Endereço IP: %s\n", wifi_info.ip_address);

            snprintf(ssid, sizeof(ssid), "%s", wifi_info.ssid);
            snprintf(signal, sizeof(signal), "%dDB", wifi_info.signal_quality);
            snprintf(channel, sizeof(channel), "%d", wifi_info.channel);
            snprintf(ip, sizeof(ip), "%s", "192.168.0.54");

            ssd1306_fill(&ssd, false); // Limpa a tela

            ssd1306_draw_string(&ssd, "rede:", 8, 8); // Desenha uma string
            ssd1306_draw_string(&ssd, ssid, 48, 8);   // Desenha uma string

            ssd1306_draw_string(&ssd, "sinal:", 8, 24); // Desenha uma string
            ssd1306_draw_string(&ssd, signal, 56, 24);  // Desenha uma string

            ssd1306_draw_string(&ssd, "canal:", 8, 40); // Desenha uma string
            ssd1306_draw_string(&ssd, channel, 56, 40); // Desenha uma string

            ssd1306_draw_string(&ssd, "IP:", 8, 56); // Desenha uma string
            ssd1306_draw_string(&ssd, ip, 32, 56);   // Desenha uma string

            ssd1306_send_data(&ssd); // Atualiza o display
        }

        sleep_ms(500);
    }

    // Desligar a arquitetura CYW43.
    wifi_disconnect();
    return 0;
}

// Inicializa um LED em um pino específico
void init_led(uint pin)
{
    gpio_init(pin);              // Configura o pino
    gpio_set_dir(pin, GPIO_OUT); // Define como saída
    gpio_put(pin, false);        // Apaga o LED
}

// Leitura da temperatura interna do microcontrolador
float internal_temp_read(void)
{
    adc_select_input(4);                              // Seleciona o sensor interno de temperatura (canal 4)
    uint16_t raw_value = adc_read();                  // Lê o valor bruto do ADC
    const float conversion_factor = 3.3f / (1 << 12); // Converte para tensão (0-3.3V)
    internal_temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
    return internal_temperature;
}

// faz leituras médias do sensor para evitar possíveis flutuações
float read_dht11_average(float *temperature, float *humidity)
{
    float temp_sum = 0;
    float hum_sum = 0;
    int valid_reads = 0;

    for (int i = 0; i < 50; ++i)
    {
        float temp, hum;
        if (dht11_read(&temp, &hum))
        {
            temp_sum += temp;
            hum_sum += hum;
            valid_reads++;
        }
        else
        {
            // Se a leitura falhar, espera um pouco antes de tentar novamente
            sleep_ms(50);
        }
    }

    // Evita divisão por zero caso todas as leituras falhem
    if (valid_reads > 0)
    {
        *temperature = temp_sum / valid_reads;
        *humidity = hum_sum / valid_reads;
    }
    else
    {
        // Em caso de erro em todas as leituras, retorna -1 para indicar falha
        *temperature = -1;
        *humidity = -1;
    }

    return valid_reads;
}
