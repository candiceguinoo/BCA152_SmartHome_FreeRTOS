#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "dht11.h"

#define DHT_GPIO         GPIO_NUM_4
#define LDR_ADC_CHANNEL  ADC_CHANNEL_4   // GPIO5 on ESP32-S3 = ADC1 channel 4

// Set to 1 if your LDR module gives a LOWER AO value when it gets brighter.
#define LDR_INVERTED     0

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

static void ldr_task(void *arg)
{
    adc_oneshot_unit_handle_t adc;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, LDR_ADC_CHANNEL, &chan_cfg));

    while (true) {
        int raw = 0;
        esp_err_t err = adc_oneshot_read(adc, LDR_ADC_CHANNEL, &raw);
        if (err == ESP_OK) {
            int light = raw * 100 / 4095;          // map 0-4095 to 0-100
#if LDR_INVERTED
            light = 100 - light;
#endif
            ESP_LOGI(TAG, "Light: raw=%d, intensity=%d", raw, light);
        } else {
            ESP_LOGW(TAG, "ADC read failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Multisensor starting");
    xTaskCreate(dht_task, "dht_task", 4096, NULL, 5, NULL);
    xTaskCreate(ldr_task, "ldr_task", 4096, NULL, 5, NULL);
}