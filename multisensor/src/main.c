#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "multisensor";

void app_main(void)
{
    int count = 0;
    while (true) {
        ESP_LOGI(TAG, "Multisensor alive, count=%d", count++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}