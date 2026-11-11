
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#include "dht11.h"
#include "light_map.h"
#include "state_machine.h"
#include "app_multisensor.h"

#define DHT_GPIO         GPIO_NUM_4
#define LDR_ADC_CHANNEL  ADC_CHANNEL_4
#define LDR_INVERTED     0

#define HUMIDITY_LIMIT        85
#define HUMIDITY_LIMIT_CLEAR  80
#define DHT_MAX_FAILS         3

#define RAINMAKER_REPORT_INTERVAL_MS 5000

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
    int temperature;
    int humidity;
    int light;
} sensor_reading_t;

static QueueHandle_t sensor_queue;
static SemaphoreHandle_t log_mutex;
static EventGroupHandle_t sensor_events;

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
        int temperature = 0;
        int humidity = 0;

        esp_err_t err = dht11_read(
            DHT_GPIO,
            &temperature,
            &humidity
        );

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

            safe_log(
                ESP_LOG_WARN,
                "DHT11 read failed (%d): %s",
                fail_count,
                esp_err_to_name(err)
            );

            if (fail_count >= DHT_MAX_FAILS) {
                xEventGroupSetBits(sensor_events, BIT_DHT_FAULT);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void ldr_task(void *arg)
{
    adc_oneshot_unit_handle_t adc;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(&unit_cfg, &adc)
    );

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc,
            LDR_ADC_CHANNEL,
            &chan_cfg
        )
    );

    while (true) {
        int raw = 0;

        if (adc_oneshot_read(adc, LDR_ADC_CHANNEL, &raw) == ESP_OK) {
            int light = light_map_percent(raw, LDR_INVERTED);

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

static void report_task(void *arg)
{
    sensor_reading_t r;

    int temperature = 0;
    int humidity = 0;
    int light = 0;

    bool limit_active = false;
    bool dht_ready = false;
    bool light_ready = false;
    bool first_report_done = false;

    TickType_t last_report_tick = 0;

    while (true) {
        if (xQueueReceive(sensor_queue, &r, portMAX_DELAY) == pdTRUE) {

            if (r.type == READING_DHT) {
                temperature = r.temperature;
                humidity = r.humidity;
                dht_ready = true;

                if (!limit_active && humidity >= HUMIDITY_LIMIT) {
                    limit_active = true;

                    xEventGroupSetBits(
                        sensor_events,
                        BIT_LIMIT_EXCEEDED
                    );
                } else if (
                    limit_active &&
                    humidity < HUMIDITY_LIMIT_CLEAR
                ) {
                    limit_active = false;

                    xEventGroupSetBits(
                        sensor_events,
                        BIT_LIMIT_CLEARED
                    );
                }

            } else {
                light = r.light;
                light_ready = true;
            }

            safe_log(
                ESP_LOG_INFO,
                "Temp: %d C | Humidity: %d %% | Light: %d",
                temperature,
                humidity,
                light
            );

            // Publish only after BOTH sensors have valid readings.
            if (dht_ready && light_ready) {

                TickType_t now = xTaskGetTickCount();

                bool report_due =
                    !first_report_done ||
                    (now - last_report_tick) >=
                    pdMS_TO_TICKS(RAINMAKER_REPORT_INTERVAL_MS);

                if (report_due) {
                    esp_err_t err = app_multisensor_report(
                        (float)temperature,
                        humidity,
                        light
                    );

                    if (err == ESP_OK) {
                        first_report_done = true;
                        last_report_tick = now;

                        safe_log(
                            ESP_LOG_INFO,
                            "RainMaker readings reported"
                        );
                    } else {
                        safe_log(
                            ESP_LOG_WARN,
                            "RainMaker report failed: %s",
                            esp_err_to_name(err)
                        );
                    }
                }
            }
        }
    }
}

static system_state_t apply_event(
    system_state_t current,
    sm_event_t evt
)
{
    system_state_t next = sm_next_state(current, evt);

    if (next != current) {
        safe_log(
            ESP_LOG_INFO,
            "State: %s -> %s",
            sm_state_name(current),
            sm_state_name(next)
        );
    }

    return next;
}

static void state_task(void *arg)
{
    system_state_t state = STATE_INIT;

    safe_log(
        ESP_LOG_INFO,
        "State: %s",
        sm_state_name(state)
    );

    const EventBits_t both = BIT_DHT_OK | BIT_LDR_OK;

    EventBits_t bits = xEventGroupWaitBits(
        sensor_events,
        both,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(10000)
    );

    if ((bits & both) == both) {
        state = apply_event(state, SM_EVT_SENSORS_READY);
    } else {
        state = apply_event(state, SM_EVT_SENSOR_FAULT);
    }

    const EventBits_t watched =
        BIT_DHT_OK |
        BIT_DHT_FAULT |
        BIT_LIMIT_EXCEEDED |
        BIT_LIMIT_CLEARED;

    while (true) {
        bits = xEventGroupWaitBits(
            sensor_events,
            watched,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );

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
    // Create FreeRTOS resources first.
    log_mutex = xSemaphoreCreateMutex();
    sensor_queue = xQueueCreate(10, sizeof(sensor_reading_t));
    sensor_events = xEventGroupCreate();

    if (log_mutex == NULL ||
        sensor_queue == NULL ||
        sensor_events == NULL) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS resources");
        return;
    }

    safe_log(ESP_LOG_INFO, "Multisensor starting");

    // Initialize RainMaker.
    esp_err_t err = app_multisensor_init();

    if (err != ESP_OK) {
        safe_log(
            ESP_LOG_WARN,
            "RainMaker initialization failed: %s",
            esp_err_to_name(err)
        );
        return;
    }

    // Start RainMaker and network provisioning.
    err = app_multisensor_start();

    if (err != ESP_OK) {
        safe_log(
            ESP_LOG_WARN,
            "RainMaker startup failed: %s",
            esp_err_to_name(err)
        );
        return;
    }

    safe_log(ESP_LOG_INFO, "RainMaker started");

    // Start the existing sensor and state tasks.
    xTaskCreate(state_task, "state_task", 4096, NULL, 5, NULL);
    xTaskCreate(report_task, "report_task", 4096, NULL, 5, NULL);
    xTaskCreate(dht_task, "dht_task", 4096, NULL, 5, NULL);
    xTaskCreate(ldr_task, "ldr_task", 4096, NULL, 5, NULL);
}
