#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dht11.h"

#define DHT_GPIO GPIO_NUM_4

static const char *TAG = "multisensor";

static void dht_task(void *arg)
{
    while (true) {
        int temperature = 0, humidity = 0;
        esp_err_t err = dht11_read(DHT_GPIO, &temperature, &humidity);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Temperature: %d C, Humidity: %d %%", temperature, humidity);
        } else {
            ESP_LOGW(TAG, "DHT11 read failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(2000));   // DHT11 needs at least 1 s between reads
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Multisensor starting");
    xTaskCreate(dht_task, "dht_task", 4096, NULL, 5, NULL);
}