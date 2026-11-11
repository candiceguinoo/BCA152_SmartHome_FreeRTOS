#include "app_multisensor.h"

#include "app_driver.h"
#include "app_node.h"

#include "esp_log.h"
#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_types.h"
#include "esp_rmaker_standard_params.h"

static const char *TAG = "app_multisensor";

static app_node_t rainmaker_node;

static esp_rmaker_param_t *temperature_param;
static esp_rmaker_param_t *humidity_param;
static esp_rmaker_param_t *light_param;

static void init_sensor_device(esp_rmaker_device_t *device, void *arg)
{
    (void)arg;

    temperature_param = esp_rmaker_temperature_param_create(
        "Temperature",
        0.0f
    );

    humidity_param = esp_rmaker_param_create(
        "Humidity",
        NULL,
        esp_rmaker_float(0.0f),
        PROP_FLAG_READ | PROP_FLAG_TIME_SERIES
    );

    light_param = esp_rmaker_param_create(
        "Light",
        NULL,
        esp_rmaker_int(0),
        PROP_FLAG_READ | PROP_FLAG_TIME_SERIES
    );

    if (temperature_param == NULL ||
        humidity_param == NULL ||
        light_param == NULL)
    {
        ESP_LOGE(TAG, "Failed to create sensor parameters");
        return;
    }

    esp_rmaker_device_add_param(device, temperature_param);
    esp_rmaker_device_add_param(device, humidity_param);
    esp_rmaker_device_add_param(device, light_param);

    esp_rmaker_device_assign_primary_param(
        device,
        temperature_param
    );

    ESP_LOGI(TAG, "Sensor parameters created successfully");
}

esp_err_t app_multisensor_init(void)
{
    esp_err_t err = app_driver_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Driver initialization failed");
        return err;
    }

    const app_node_params_t params = {
        .node_name = "Smart Greenhouse Multisensor",
        .node_type = "ESP32-S3",
        .device_name = "Greenhouse Sensors",
        .device_type = ESP_RMAKER_DEVICE_TEMP_SENSOR,
        .init_device = init_sensor_device,
        .init_device_arg = NULL
    };

    err = app_node_init(&rainmaker_node, &params);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RainMaker node initialization failed");
        return err;
    }

    if (temperature_param == NULL ||
        humidity_param == NULL ||
        light_param == NULL)
    {
        ESP_LOGE(TAG, "Sensor parameter initialization incomplete");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "RainMaker initialized successfully");

    return ESP_OK;
}

esp_err_t app_multisensor_start(void)
{
    esp_err_t err = app_node_start(&rainmaker_node);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RainMaker start failed");
        return err;
    }

    return app_driver_start();
}

esp_err_t app_multisensor_report(
    float temperature,
    int humidity,
    int light
)
{
    if (temperature_param == NULL ||
        humidity_param == NULL ||
        light_param == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err;

    err = esp_rmaker_param_update(
        temperature_param,
        esp_rmaker_float(temperature)
    );

    if (err != ESP_OK) return err;

    err = esp_rmaker_param_update(
        humidity_param,
        esp_rmaker_float((float)humidity)
    );

    if (err != ESP_OK) return err;

    return esp_rmaker_param_update_and_report(
        light_param,
        esp_rmaker_int(light)
    );
}
