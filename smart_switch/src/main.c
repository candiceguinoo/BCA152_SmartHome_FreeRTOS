
#include "app_driver.h"
#include "app_plug.h"

#include "esp_log.h"

static const char *TAG = "main";

static app_plug_t app_plug;

void app_main(void)
{
    esp_err_t err;

    err = app_driver_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Driver initialization failed");
        return;
    }

    err = app_plug_init(&app_plug);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Plug initialization failed");
        return;
    }

    err = app_plug_start(&app_plug);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Plug start failed");
        return;
    }

    err = app_driver_start();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Network start failed");
        return;
    }
}
