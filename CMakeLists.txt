cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(webserver-raspberry-pico-w C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME} webserver-raspberry-pico-w.c lib/wifi.c lib/webserver.c lib/dht11.c lib/buzzer.c)

pico_set_program_name(${PROJECT_NAME} "${PROJECT_NAME}")
pico_set_program_version(${PROJECT_NAME} "0.1")

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
        pico_cyw43_arch_none
        hardware_adc
        hardware_gpio
        hardware_pwm
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME} 
        )

pico_add_extra_outputs(${PROJECT_NAME})

