#pragma once
#include "driver/gpio.h"
#include "esp_err.h"

// Reads the DHT11. temperature in whole °C, humidity in whole %RH.
esp_err_t dht11_read(gpio_num_t pin, int *temperature, int *humidity);