#include <stdarg.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "dht11.h"
#include "state_machine.h"

#define DHT_GPIO         GPIO_NUM_4
#define LDR_ADC_CHANNEL  ADC_CHANNEL_4   // GPIO5 on ESP32-S3 = ADC1 channel 4
#define LDR_INVERTED     0               // your module: brighter = higher, so 0

#define HUMIDITY_LIMIT        55  // % : warning above this
#define HUMIDITY_LIMIT_CLEAR  50 // % : back to normal below this (hysteresis)
#define DHT_MAX_FAILS         3    // consecutive failed reads before FAULT

// Event group bits
#define BIT_DHT_OK          (1 << 0)
#define BIT_LDR_OK          (1 << 1)
#define BIT_DHT_FAULT       (1 << 2)
#define BIT_LIMIT_EXCEEDED  (1 << 3)
#define BIT_LIMIT_CLEARED   (1 << 4)

static const char *TAG = "multisensor";

typedef enum {
    READING_DHT,
    READING_LIGHT
} reading_type_t;

typedef struct {
    reading_type_t type;
    int temperature;   // used when type == READING_DHT
    int humidity;      // used when type == READING_DHT
    int light;         // used when type == READING_LIGHT (0-100)
} sensor_reading_t;

static QueueHandle_t sensor_queue;
static SemaphoreHandle_t log_mutex;
static EventGroupHandle_t sensor_events;

// Only one task at a time may print, so log lines never interleave.
static void safe_log(esp_log_level_t level, const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (level == ESP_LOG_WARN) {
            ESP_LOGW(TAG, "%s", buf);
        } else {
            ESP_LOGI(TAG, "%s", buf);
        }
        xSemaphoreGive(log_mutex);
    }
}

static void dht_task(void *arg)
{
    int fail_count = 0;

    while (true) {
        int temperature = 0, humidity = 0;
        esp_err_t err = dht11_read(DHT_GPIO, &temperature, &humidity);
        if (err == ESP_OK) {
            fail_count = 0;
            sensor_reading_t r = {
                .type = READING_DHT,
                .temperature = temperature,
                .humidity = humidity,
            };
            xQueueSend(sensor_queue, &r, 0);
            xEventGroupSetBits(sensor_events, BIT_DHT_OK);
        } else {
            fail_count++;
            safe_log(ESP_LOG_WARN, "DHT11 read failed (%d): %s",
                     fail_count, esp_err_to_name(err));
            if (fail_count >= DHT_MAX_FAILS) {
                xEventGroupSetBits(sensor_events, BIT_DHT_FAULT);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));   // DHT11 needs at least 1 s between reads
    }
}

static void ldr_task(void *arg)
{
    adc_oneshot_unit_handle_t adc;
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, LDR_ADC_CHANNEL, &chan_cfg));

    while (true) {
        int raw = 0;
        if (adc_oneshot_read(adc, LDR_ADC_CHANNEL, &raw) == ESP_OK) {
            int light = raw * 100 / 4095;
#if LDR_INVERTED
            light = 100 - light;
#endif
            sensor_reading_t r = {
                .type = READING_LIGHT,
                .light = light,
            };
            xQueueSend(sensor_queue, &r, 0);
            xEventGroupSetBits(sensor_events, BIT_LDR_OK);
        } else {
            safe_log(ESP_LOG_WARN, "ADC read failed");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Receives readings from both sensor tasks, prints one combined line,
// and raises threshold events for the state task.
static void report_task(void *arg)
{
    sensor_reading_t r;
    int temperature = 0, humidity = 0, light = 0;
    bool limit_active = false;

    while (true) {
        if (xQueueReceive(sensor_queue, &r, portMAX_DELAY) == pdTRUE) {
            if (r.type == READING_DHT) {
                temperature = r.temperature;
                humidity = r.humidity;

                if (!limit_active && humidity >= HUMIDITY_LIMIT) {
                    limit_active = true;
                    xEventGroupSetBits(sensor_events, BIT_LIMIT_EXCEEDED);
                } else if (limit_active && humidity < HUMIDITY_LIMIT_CLEAR) {
                    limit_active = false;
                    xEventGroupSetBits(sensor_events, BIT_LIMIT_CLEARED);
                }
            } else {
                light = r.light;
            }
            safe_log(ESP_LOG_INFO, "Temp: %d C | Humidity: %d %% | Light: %d",
                     temperature, humidity, light);
        }
    }
}

static system_state_t apply_event(system_state_t current, sm_event_t evt)
{
    system_state_t next = sm_next_state(current, evt);
    if (next != current) {
        safe_log(ESP_LOG_INFO, "State: %s -> %s",
                 sm_state_name(current), sm_state_name(next));
    }
    return next;
}

// Owns the system state. Wakes up only when an event group bit is set.
static void state_task(void *arg)
{
    system_state_t state = STATE_INIT;
    safe_log(ESP_LOG_INFO, "State: %s", sm_state_name(state));

    // Wait up to 10 s for BOTH sensors to report at least once
    const EventBits_t both = BIT_DHT_OK | BIT_LDR_OK;
    EventBits_t bits = xEventGroupWaitBits(sensor_events, both,
                                           pdFALSE,   // do not clear
                                           pdTRUE,    // wait for ALL bits
                                           pdMS_TO_TICKS(10000));
    if ((bits & both) == both) {
        state = apply_event(state, SM_EVT_SENSORS_READY);
    } else {
        state = apply_event(state, SM_EVT_SENSOR_FAULT);
    }

    const EventBits_t watched = BIT_DHT_OK | BIT_DHT_FAULT |
                                BIT_LIMIT_EXCEEDED | BIT_LIMIT_CLEARED;
    while (true) {
        bits = xEventGroupWaitBits(sensor_events, watched,
                                   pdTRUE,    // clear bits when read
                                   pdFALSE,   // wake on ANY bit
                                   portMAX_DELAY);

        if (bits & BIT_DHT_FAULT) {
            state = apply_event(state, SM_EVT_SENSOR_FAULT);
        }
        if ((bits & BIT_DHT_OK) && state == STATE_FAULT) {
            state = apply_event(state, SM_EVT_SENSOR_RECOVERED);
        }
        if (bits & BIT_LIMIT_EXCEEDED) {
            state = apply_event(state, SM_EVT_THRESHOLD_EXCEEDED);
        }
        if (bits & BIT_LIMIT_CLEARED) {
            state = apply_event(state, SM_EVT_VALUES_NORMAL);
        }
    }
}

void app_main(void)
{
    // Create the mutex, queue and event group BEFORE any task uses them
    log_mutex = xSemaphoreCreateMutex();
    sensor_queue = xQueueCreate(10, sizeof(sensor_reading_t));
    sensor_events = xEventGroupCreate();

    safe_log(ESP_LOG_INFO, "Multisensor starting");

    xTaskCreate(state_task, "state_task", 4096, NULL, 5, NULL);
    xTaskCreate(report_task, "report_task", 4096, NULL, 5, NULL);
    xTaskCreate(dht_task, "dht_task", 4096, NULL, 5, NULL);
    xTaskCreate(ldr_task, "ldr_task", 4096, NULL, 5, NULL);
}