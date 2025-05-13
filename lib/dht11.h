#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>
#include <stdbool.h>

// Inicializa o sensor DHT11
void dht11_init(int pin);

// Lê a umidade do sensor DHT11
float dht11_read_humidity();

// Lê a temperatura do sensor DHT11
float dht11_read_temperature();

// Lê ambos temperatura e umidade do sensor DHT11
int dht11_read(float *temperature, float *humidity);

#endif // DHT11_H
