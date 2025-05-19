## **WEBSERVER - CASA INTELIGENTE (ENTREGA FINAL)**

---

## Objetivo Geral  
Desenvolver um webserver utilizando a **Raspberry Pico W**, com o propósito de uma aplicação para casa inteligente. O sistema permitirá o controle remoto de dispositivos como matriz de LEDs, LEDs, buzzers e fará leitura de temperatura e umidade ambiente usando o sensor **DHT11**. Este projeto visa consolidar os conhecimentos adquiridos em aula sobre comunicação em IoT.

---

## Descrição Funcional  
O projeto implementa um sistema de controle e monitoramento de uma casa inteligente com as seguintes funcionalidades:

- **Monitoramento de temperatura ambiente**  
- **Monitoramento de temperatura do sistema**  
- **Controle da iluminação (LEDs)**  
- **Controle do Alarme (Buzzers)**  
- **Modo BOOTSEL ao pressionar o botão B**  
- **Feedback dos parâmetros do sistema através do display**  
- **Alarme de Limites de temperatura**  

Todas as funcionalidades são coordenadas pelo webserver, que além de fornecer uma interface para acompanhamento dos dados do sistema, também oferece a possibilidade de controlar o Raspberry à distância em uma rede local.

---

## Mapeamento dos Periféricos  
Cada periférico desempenha as seguintes funções no sistema:

| **Função** | **Periférico** | **Descrição** |
|------------|----------------|----------------|
| Monitoramento de temperatura ambiente | **DHT11** | Mede a temperatura do ambiente que é lida pelo Raspberry |
| Monitoramento de umidade ambiente | **DHT11** | Mede a umidade do ambiente que é lida pelo Raspberry |
| Monitoramento de temperatura interna | **Sensor do RP2040** | Mede a temperatura interna do RP2040 |
| Controle de iluminação | **LEDs** | Permite controlar os LEDs verde, vermelho e azul da BitDogLab de forma independente |
| Controle do Alarme | **Buzzers** | Permite controlar o acionamento do alarme usando os buzzers da BitDogLab |
| Modo Bootsel | **Botão B** | Ao pressionar o botão B, causa uma interrupção que coloca a placa em modo de gravação (BOOTSEL) |
| Modo de Leitura do Sistema | **Botão A** | Ao pressionar o botão A, causa uma interrupção modificando o modo de monitoramento do sistema |
| Monitoramento do sistema | **Display SSD1306** | Exibe diferentes valores de leitura do sistema em três modos: SENSOR, LEDS, WIFI |

---

## Bibliotecas Utilizadas  
O webserver conta com as seguintes bibliotecas customizadas:

- **wifi.h**  
  - Funções: `wifi_init`, `wifi_poll`, `wifi_disconnect`  
  - Gerencia a conexão Wi-Fi e a manutenção da comunicação na rede 2.4G.  

- **webserver.h**  
  - Funções: `tcp_server_accept`, `tcp_server_recv`, `tcp_sent_callback`, `webserver_init`, `user_request`  
  - Estabelece a comunicação e controle do servidor web.  

- **dht11.h**  
  - Funções: `dht11_init`, `dht11_read_humidity`, `dht11_read_temperature`, `dht11_read`  
  - Responsável pela inicialização e leitura do sensor de umidade e temperatura **DHT11**.  

- **buzzer.h**  
  - Funções: `initialization_buzzers`, `buzzer_pwm`  
  - Controla a inicialização e ativação dos buzzers para os alertas sonoros.
    
- **ssd1306.h**  
  - Funções: `ssd1306_init`, `ssd1306_circle` e `ssd1306_draw_string`
  - Biblioteca com as funções responsáveis pelo controle do display OLED.  

---

## Uso dos Periféricos da BitDogLab  
Foram utilizados os seguintes periféricos:

- **Display:** Utilizado para a exibição das informações do sistema. 
- **Botão A:** Utilizado para mudança de modos de exibição do display.  
- **Botão B:** Utilizado para interromper a execução e entrar em modo BOOTSEL.  
- **Led RGB:** Utilizado para o controle da luminosidade do ambiente através do webserver.  
- **Buzzers:** Utilizado para o acionamento do alarme através do servidor.  
- **Sensor DHT11:** Utilizado para o monitoramento da temperatura e umidade ambiente.  

---

- **Vídeo de Demonstração:** [[YouTube](https://youtu.be/7B6Rz4DQe1w)](https://youtu.be/2jItLws7qHA?si=ZoJ9htPo5tCIxKky)
  
---
