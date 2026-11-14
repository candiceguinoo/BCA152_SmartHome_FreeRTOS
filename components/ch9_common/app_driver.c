#include "app_driver.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "app_network.h"

static const char *TAG = "app_driver";

esp_err_t app_driver_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "Erasing NVS flash...");

        err = nvs_flash_erase();

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "NVS erase failed: %s", esp_err_to_name(err));
            return err;
        }

        err = nvs_flash_init();
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(err));
        return err;
    }

	app_network_init();
	return ESP_OK;}

esp_err_t app_driver_start(void)
{
    return app_network_start(POP_TYPE_RANDOM);
}