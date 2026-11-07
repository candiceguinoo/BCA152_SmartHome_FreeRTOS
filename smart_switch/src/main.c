#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_GPIO    GPIO_NUM_4
#define RELAY_GPIO  GPIO_NUM_19

// Most relay modules are active-low. Set to 0 if yours is active-high.
#define RELAY_ACTIVE_LOW 1

static const char *TAG = "smart_switch";

static void relay_set(bool on)
{
#if RELAY_ACTIVE_LOW
    gpio_set_level(RELAY_GPIO, on ? 0 : 1);
#else
    gpio_set_level(RELAY_GPIO, on ? 1 : 0);
#endif
    ESP_LOGI(TAG, "Relay %s", on ? "ON" : "OFF");
}

static void led_task(void *arg)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    int level = 0;
    while (true) {
        level = !level;
        gpio_set_level(LED_GPIO, level);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void relay_task(void *arg)
{
    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);
    relay_set(false);   // start with the relay off

    bool state = false;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        state = !state;
        relay_set(state);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Smart switch starting");
    xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
    xTaskCreate(relay_task, "relay_task", 2048, NULL, 5, NULL);
}