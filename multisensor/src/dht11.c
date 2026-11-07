#include "dht11.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

// Waits until the pin reaches `level`. Returns elapsed µs, or -1 on timeout.
static int wait_for(gpio_num_t pin, int level, int timeout_us)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(pin) != level) {
        if (esp_timer_get_time() - start > timeout_us) {
            return -1;
        }
    }
    return (int)(esp_timer_get_time() - start);
}

esp_err_t dht11_read(gpio_num_t pin, int *temperature, int *humidity)
{
    uint8_t data[5] = {0};
    static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);

    // Start signal: pull low at least 18 ms, then release
    gpio_set_level(pin, 0);
    esp_rom_delay_us(20000);
    gpio_set_level(pin, 1);

    esp_err_t result = ESP_OK;
    portENTER_CRITICAL(&mux);   // timing-critical: block interrupts (about 4 ms)

    // Sensor response: low 80 µs, high 80 µs
    if (wait_for(pin, 0, 100) < 0 ||
        wait_for(pin, 1, 100) < 0 ||
        wait_for(pin, 0, 100) < 0) {
        result = ESP_ERR_TIMEOUT;
    } else {
        // 40 bits. Each starts with ~50 µs low, then a high pulse:
        // ~27 µs = bit 0, ~70 µs = bit 1
        for (int i = 0; i < 40; i++) {
            if (wait_for(pin, 1, 100) < 0) { result = ESP_ERR_TIMEOUT; break; }
            int high_us = wait_for(pin, 0, 120);
            if (high_us < 0) { result = ESP_ERR_TIMEOUT; break; }
            data[i / 8] <<= 1;
            if (high_us > 40) {
                data[i / 8] |= 1;
            }
        }
    }

    portEXIT_CRITICAL(&mux);
    if (result != ESP_OK) {
        return result;
    }

    // Checksum: sum of the first four bytes
    if (((data[0] + data[1] + data[2] + data[3]) & 0xFF) != data[4]) {
        return ESP_ERR_INVALID_CRC;
    }

    *humidity = data[0];
    *temperature = data[2];
    return ESP_OK;
}