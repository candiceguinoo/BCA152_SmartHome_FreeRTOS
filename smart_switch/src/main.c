#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_GPIO     GPIO_NUM_4
#define RELAY_GPIO   GPIO_NUM_19
#define BUTTON_GPIO  GPIO_NUM_0

// Most relay modules are active-low. Set to 0 if yours is active-high.
#define RELAY_ACTIVE_LOW 1

static const char *TAG = "smart_switch";

typedef enum {
    EVT_TOGGLE
} switch_event_t;

static QueueHandle_t event_queue;
static bool relay_state = false;

static void relay_set(bool on)
{
    relay_state = on;
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

// Reads the BOOT button, debounces it, and sends an event to the queue.
static void button_task(void *arg)
{
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    int last_stable = 1;   // 1 = released
    int last_read = 1;
    TickType_t last_change = xTaskGetTickCount();

    while (true) {
        int now = gpio_get_level(BUTTON_GPIO);
        if (now != last_read) {
            last_read = now;
            last_change = xTaskGetTickCount();
        }
        // accept the new level only after it stayed the same for 50 ms
        if ((xTaskGetTickCount() - last_change) > pdMS_TO_TICKS(50) &&
            now != last_stable) {
            last_stable = now;
            if (last_stable == 0) {            // pressed
                switch_event_t evt = EVT_TOGGLE;
                xQueueSend(event_queue, &evt, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Waits for events from the queue and drives the relay.
static void relay_task(void *arg)
{
    switch_event_t evt;
    while (true) {
        if (xQueueReceive(event_queue, &evt, portMAX_DELAY) == pdTRUE) {
            if (evt == EVT_TOGGLE) {
                relay_set(!relay_state);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Smart switch starting");

    // Configure the relay pin and start with the relay off
    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);
    relay_set(false);

    // Create the queue BEFORE the tasks that use it
    event_queue = xQueueCreate(5, sizeof(switch_event_t));

    xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
    xTaskCreate(relay_task, "relay_task", 2048, NULL, 5, NULL);
}