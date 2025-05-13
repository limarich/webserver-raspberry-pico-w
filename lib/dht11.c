#include "dht11.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define DHT11_ADC_CHANNEL 2 // ADC 2 corresponde ao GPIO 28

void dht11_init(int pin)
{
    // Inicializa o sistema de ADC
    adc_init();
    adc_gpio_init(pin);
    adc_select_input(DHT11_ADC_CHANNEL);
}

// Lê a umidade do sensor DHT11
float dht11_read_humidity()
{
    uint16_t raw_value = adc_read();
    // Converte para porcentagem
    float humidity = (raw_value / 4095.0) * 100.0;
    return humidity;
}

// Lê a temperatura do sensor DHT11
float dht11_read_temperature()
{
    uint16_t raw_value = adc_read();
    // Converte para temperatura em Celsius
    float temperature = (raw_value / 4095.0) * 50.0; // máximo é 50°C
    return temperature;
}

// Lê ambos temperatura e umidade do sensor DHT11
int dht11_read(float *temperature, float *humidity)
{
    adc_select_input(DHT11_ADC_CHANNEL);
    if (temperature == NULL || humidity == NULL)
        return 0;

    *humidity = dht11_read_humidity();
    *temperature = dht11_read_temperature();

    return 1;
}