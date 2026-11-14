
#include "AppDriver.hpp"
#include "AppPlug.hpp"
#include "esp_log.h"

namespace
{
    app::AppDriver app_driver;
    app::AppPlug app_plug;
}

extern "C" void app_main(void)
{
    ESP_LOGE("MAIN", "STEP 1: app_main started");

    app_driver.init();
    ESP_LOGE("MAIN", "STEP 2: AppDriver initialized");

    app_plug.init();
    ESP_LOGE("MAIN", "STEP 3: AppPlug initialized");

    app_plug.start();
    ESP_LOGE("MAIN", "STEP 4: AppPlug started");

    app_driver.start();
    ESP_LOGE("MAIN", "STEP 5: AppDriver started");
}
