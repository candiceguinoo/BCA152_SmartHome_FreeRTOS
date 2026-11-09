#pragma once
#include <esp_log.h>
#include <nvs_flash.h>
#include <app_network.h>

namespace app
{
class AppDriver
{
public:
    void init()
    {
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
            err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            nvs_flash_init();
        }
        app_network_init();
    }

    void start()
    {
        app_network_start(POP_TYPE_RANDOM);
    }
};
} // namespace app