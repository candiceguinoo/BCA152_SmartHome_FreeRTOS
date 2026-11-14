
#include <string.h>

#include "app_plug.h"
#include "debounce.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rmaker_standard_params.h"
#include "esp_rmaker_standard_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RELAY_GPIO GPIO_NUM_19
#define LED_GPIO GPIO_NUM_4
#define BUTTON_GPIO GPIO_NUM_0

#define RELAY_ACTIVE_LOW true

static const char *TAG = "plug";

/* Log messages using the mutex. */
static void plug_log(app_plug_t *plug, const char *message)
{
    if (plug->log_mutex != NULL &&
        xSemaphoreTake(plug->log_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        ESP_LOGI(TAG, "%s", message);
        xSemaphoreGive(plug->log_mutex);
    }
}

/* Apply the relay and LED states. */
static void plug_apply(app_plug_t *plug, bool on, bool report)
{
    plug->state = on;

    gpio_set_level(
        RELAY_GPIO,
        (RELAY_ACTIVE_LOW ? !on : on) ? 1 : 0
    );

    gpio_set_level(LED_GPIO, on ? 1 : 0);

    if (report && plug->power_param != NULL)
    {
        esp_rmaker_param_update_and_report(
            plug->power_param,
            esp_rmaker_bool(on)
        );
    }

    plug_log(plug, on ? "Relay ON" : "Relay OFF");
}

/* Handle button presses. */
static void button_task(void *arg)
{
    app_plug_t *plug = (app_plug_t *)arg;

    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    debounce_t deb;
    debounce_init(&deb, 50, pdTICKS_TO_MS(xTaskGetTickCount()));

    while (true)
    {
        int level = gpio_get_level(BUTTON_GPIO);

        if (debounce_update(
                &deb,
                level,
                pdTICKS_TO_MS(xTaskGetTickCount())))
        {
            plug_log(plug, "Button pressed");

            plug_event_t event = PLUG_EVT_TOGGLE;
            xQueueSend(plug->queue, &event, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Process relay events from the queue. */
static void relay_task(void *arg)
{
    app_plug_t *plug = (app_plug_t *)arg;
    plug_event_t event;

    while (true)
    {
        if (xQueueReceive(plug->queue, &event, portMAX_DELAY) == pdTRUE)
        {
            switch (event)
            {
                case PLUG_EVT_TOGGLE:
                    plug_apply(plug, !plug->state, true);
                    break;

                case PLUG_EVT_ON:
                    plug_apply(plug, true, true);
                    break;

                case PLUG_EVT_OFF:
                    plug_apply(plug, false, true);
                    break;

                default:
                    break;
            }
        }
    }
}

/* Handle RainMaker power requests. */
static esp_err_t request_handler(
    const esp_rmaker_device_t *device,
    const esp_rmaker_param_t *param,
    const esp_rmaker_param_val_t value,
    void *priv_data,
    esp_rmaker_write_ctx_t *ctx)
{
    (void)device;
    (void)ctx;

    app_plug_t *plug = (app_plug_t *)priv_data;

    if (plug == NULL || param == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *type = esp_rmaker_param_get_type(param);

    if (type != NULL &&
        strcmp(type, ESP_RMAKER_PARAM_POWER) == 0)
    {
        plug_event_t event = value.val.b ? PLUG_EVT_ON : PLUG_EVT_OFF;
        xQueueSend(plug->queue, &event, 0);
    }

    return ESP_OK;
}

/* Define the plug's RainMaker power parameter. */
static void define_plug(esp_rmaker_device_t *device, void *arg)
{
    app_plug_t *plug = (app_plug_t *)arg;

    plug->power_param = esp_rmaker_power_param_create("power", false);

    esp_rmaker_device_add_param(device, plug->power_param);

    esp_rmaker_device_assign_primary_param(
        device,
        plug->power_param
    );

    esp_rmaker_device_add_cb(device, request_handler, NULL);
}

/* Initialize the smart plug. */
esp_err_t app_plug_init(app_plug_t *plug)
{
    if (plug == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(plug, 0, sizeof(app_plug_t));

    plug->log_mutex = xSemaphoreCreateMutex();
    plug->queue = xQueueCreate(8, sizeof(plug_event_t));

    if (plug->log_mutex == NULL || plug->queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex or queue");
        return ESP_ERR_NO_MEM;
    }

    gpio_reset_pin(RELAY_GPIO);
    gpio_set_level(RELAY_GPIO, RELAY_ACTIVE_LOW ? 1 : 0);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    app_node_params_t params = {
        .node_name = "Plug Node",
        .node_type = "esp.node.plug",
        .device_name = "Plug",
        .device_type = "esp.device.plug",
        .init_device = define_plug,
        .init_device_arg = plug
    };

    esp_err_t err = app_node_init(&plug->node, &params);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize RainMaker node");
        return err;
    }

    plug_apply(plug, false, false);

    return ESP_OK;
}

/* Start RainMaker and the FreeRTOS tasks. */
esp_err_t app_plug_start(app_plug_t *plug)
{
    if (plug == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = app_node_start(&plug->node);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start RainMaker");
        return err;
    }

    if (xTaskCreate(
            button_task,
            "button_task",
            3072,
            plug,
            5,
            NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create button task");
        return ESP_FAIL;
    }

    if (xTaskCreate(
            relay_task,
            "relay_task",
            3072,
            plug,
            5,
            NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create relay task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
