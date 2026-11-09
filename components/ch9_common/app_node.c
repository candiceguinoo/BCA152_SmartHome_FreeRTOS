#include <string.h>

#include "app_node.h"

#include "esp_log.h"
#include "esp_rmaker_standard_types.h"
#include "esp_rmaker_standard_params.h"
#include "esp_rmaker_standard_devices.h"
#include "esp_rmaker_common_events.h"
#include "esp_rmaker_ota.h"
#include "esp_rmaker_schedule.h"
#include "esp_rmaker_scenes.h"
#include "app_insights.h"

static const char *TAG = "app_node";

static void app_node_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    app_node_t *node = (app_node_t *)arg;

    if (node == NULL)
    {
        return;
    }

    if (event_base == RMAKER_COMMON_EVENT)
    {
        switch (event_id)
        {
            case RMAKER_MQTT_EVENT_CONNECTED:
                ESP_LOGI(node->init_params.device_name,
                         "MQTT connected");
                node->connected = true;
                break;

            case RMAKER_MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(node->init_params.device_name,
                         "MQTT disconnected");
                node->connected = false;
                break;

            default:
                break;
        }
    }

    if (node->event_callback != NULL)
    {
        node->event_callback(
            node->event_callback_arg,
            event_base,
            event_id,
            event_data
        );
    }
}

esp_err_t app_node_init(
    app_node_t *node,
    const app_node_params_t *params
)
{
    if (node == NULL || params == NULL ||
        params->node_name == NULL ||
        params->node_type == NULL ||
        params->device_name == NULL ||
        params->device_type == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(node, 0, sizeof(app_node_t));

    node->init_params = *params;
    node->connected = false;

    esp_err_t err;

    err = esp_event_handler_register(
        RMAKER_COMMON_EVENT,
        ESP_EVENT_ANY_ID,
        app_node_event_handler,
        node
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register RainMaker event handler");
        return err;
    }

    err = esp_event_handler_register(
        RMAKER_OTA_EVENT,
        ESP_EVENT_ANY_ID,
        app_node_event_handler,
        node
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register OTA event handler");
        return err;
    }

    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = true,
    };

    node->rmaker_node = esp_rmaker_node_init(
        &rainmaker_cfg,
        params->node_name,
        params->node_type
    );

    if (node->rmaker_node == NULL)
    {
        ESP_LOGE(TAG, "RainMaker node initialization failed");
        return ESP_FAIL;
    }

    node->device = esp_rmaker_device_create(
        params->device_name,
        params->device_type,
        node
    );

    if (node->device == NULL)
    {
        ESP_LOGE(TAG, "RainMaker device creation failed");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_param(
        node->device,
        esp_rmaker_name_param_create(
            ESP_RMAKER_DEF_NAME_PARAM,
            params->device_name
        )
    );

    if (params->init_device != NULL)
    {
        params->init_device(
            node->device,
            params->init_device_arg
        );
    }

    esp_rmaker_node_add_device(
        node->rmaker_node,
        node->device
    );

    esp_rmaker_ota_enable_default();
    esp_rmaker_timezone_service_enable();
    esp_rmaker_schedule_enable();
    esp_rmaker_scenes_enable();

    app_insights_enable();

    ESP_LOGI(TAG, "RainMaker node initialized successfully");

    return ESP_OK;
}

esp_err_t app_node_start(app_node_t *node)
{
    if (node == NULL || node->rmaker_node == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_rmaker_start();
}

void app_node_add_event_handler(
    app_node_t *node,
    app_node_event_cb_t callback,
    void *arg
)
{
    if (node == NULL || callback == NULL)
    {
        return;
    }

    node->event_callback = callback;
    node->event_callback_arg = arg;
}

bool app_node_is_connected(const app_node_t *node)
{
    if (node == NULL)
    {
        return false;
    }

    return node->connected;
}