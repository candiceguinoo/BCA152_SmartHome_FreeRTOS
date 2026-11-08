#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "debounce.h"

#define LED_GPIO     GPIO_NUM_4
#define RELAY_GPIO   GPIO_NUM_19
#define BUTTON_GPIO  GPIO_NUM_0

#define DEBOUNCE_MS  50

// Most relay modules are active-low. Set to 0 if yours is active-high.
#define RELAY_ACTIVE_LOW 1

static const char *TAG = "smart_switch";

typedef enum {
    EVT_TOGGLE
} switch_event_t;

static QueueHandle_t event_queue;
static SemaphoreHandle_t log_mutex;
static bool relay_state = false;

// Only one task at a time may print, so log lines never interleave.
static void safe_log(const char *msg)
{
    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        ESP_LOGI(TAG, "%s", msg);
        xSemaphoreGive(log_mutex);
    }
}

static void relay_set(bool on)
{
    relay_state = on;
#if RELAY_ACTIVE_LOW
    gpio_set_level(RELAY_GPIO, on ? 0 : 1);
#else
    gpio_set_level(RELAY_GPIO, on ? 1 : 0);
#endif
    safe_log(on ? "Relay ON" : "Relay OFF");
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

// Reads the BOOT button, debounces it with the debounce module,
// and sends an event to the queue on each accepted press.
static void button_task(void *arg)
{
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    debounce_t deb;
    debounce_init(&deb, DEBOUNCE_MS, pdTICKS_TO_MS(xTaskGetTickCount()));

    while (true) {
        int level = gpio_get_level(BUTTON_GPIO);
        if (debounce_update(&deb, level, pdTICKS_TO_MS(xTaskGetTickCount()))) {
            safe_log("Button pressed");
            switch_event_t evt = EVT_TOGGLE;
            xQueueSend(event_queue, &evt, 0);
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
    // Create the mutex and queue BEFORE any task or log call uses them
    log_mutex = xSemaphoreCreateMutex();
    event_queue = xQueueCreate(5, sizeof(switch_event_t));

    safe_log("Smart switch starting");

    // Configure the relay pin and start with the relay off
    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);
    relay_set(false);

    xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
    xTaskCreate(relay_task, "relay_task", 2048, NULL, 5, NULL);
}