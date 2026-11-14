
#pragma once

#include "AppNode.hpp"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "debounce.h"
#include <cstring>

namespace app
{
    typedef enum
    {
        PLUG_EVT_TOGGLE,
        PLUG_EVT_ON,
        PLUG_EVT_OFF
    } plug_event_t;

    class AppPlug : public AppNode
    {
    private:
        static constexpr gpio_num_t RELAY_GPIO = GPIO_NUM_19;
        static constexpr gpio_num_t LED_GPIO = GPIO_NUM_4;
        static constexpr gpio_num_t BUTTON_GPIO = GPIO_NUM_0;
        static constexpr bool RELAY_ACTIVE_LOW = true;

        esp_rmaker_param_t *m_power_param = nullptr;
        QueueHandle_t m_queue = nullptr;
        SemaphoreHandle_t m_log_mutex = nullptr;
        bool m_state = false;

        void log(const char *msg)
        {
            if (m_log_mutex != nullptr &&
                xSemaphoreTake(m_log_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                ESP_LOGI("plug", "%s", msg);
                xSemaphoreGive(m_log_mutex);
            }
        }

        void apply(bool on, bool report)
        {
            m_state = on;

            gpio_set_level(
                RELAY_GPIO,
                (RELAY_ACTIVE_LOW ? !on : on) ? 1 : 0
            );

            gpio_set_level(LED_GPIO, on ? 1 : 0);

            if (report && m_power_param)
            {
                esp_rmaker_param_update_and_report(
                    m_power_param,
                    esp_rmaker_bool(on)
                );
            }

            log(on ? "Relay ON" : "Relay OFF");
        }

        static void buttonTask(void *arg)
        {
            AppPlug *self = static_cast<AppPlug *>(arg);

            ESP_LOGI("plug", "Button task started");

            gpio_reset_pin(BUTTON_GPIO);
            gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
            gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

            debounce_t deb;

            debounce_init(
                &deb,
                50,
                pdTICKS_TO_MS(xTaskGetTickCount())
            );

            while (true)
            {
                int level = gpio_get_level(BUTTON_GPIO);

                if (debounce_update(
                        &deb,
                        level,
                        pdTICKS_TO_MS(xTaskGetTickCount())))
                {
                    self->log("Button pressed");

                    plug_event_t evt = PLUG_EVT_TOGGLE;
                    xQueueSend(self->m_queue, &evt, 0);
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        static void relayTask(void *arg)
        {
            AppPlug *self = static_cast<AppPlug *>(arg);

            ESP_LOGI("plug", "Relay task started");

            plug_event_t evt;

            while (true)
            {
                if (xQueueReceive(
                        self->m_queue,
                        &evt,
                        portMAX_DELAY) == pdTRUE)
                {
                    switch (evt)
                    {
                    case PLUG_EVT_TOGGLE:
                        self->apply(!self->m_state, true);
                        break;

                    case PLUG_EVT_ON:
                        self->apply(true, true);
                        break;

                    case PLUG_EVT_OFF:
                        self->apply(false, true);
                        break;
                    }
                }
            }
        }

        static esp_err_t requestHandler(
            const esp_rmaker_device_t *device,
            const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val,
            void *priv_data,
            esp_rmaker_write_ctx_t *ctx)
        {
            AppPlug *self = static_cast<AppPlug *>(priv_data);

            if (strcmp(
                    esp_rmaker_param_get_type(param),
                    ESP_RMAKER_PARAM_POWER) == 0)
            {
                plug_event_t evt =
                    val.val.b ? PLUG_EVT_ON : PLUG_EVT_OFF;

                xQueueSend(self->m_queue, &evt, 0);
            }

            return ESP_OK;
        }

        static void definePlug(
            esp_rmaker_device_t *device,
            void *arg)
        {
            AppPlug *self = static_cast<AppPlug *>(arg);

            self->m_power_param =
                esp_rmaker_power_param_create("power", false);

            esp_rmaker_device_add_param(
                device,
                self->m_power_param
            );

            esp_rmaker_device_assign_primary_param(
                device,
                self->m_power_param
            );

            esp_rmaker_device_add_cb(
                device,
                requestHandler,
                nullptr
            );
        }

    public:
        AppPlug()
            : AppNode({
                  "Plug Node",
                  "esp.node.plug",
                  "Plug",
                  "esp.device.plug",
                  definePlug,
                  this
              })
        {
        }

        void init() override
        {
            ESP_LOGI("plug", "Initializing smart plug");

            m_log_mutex = xSemaphoreCreateMutex();
            m_queue = xQueueCreate(8, sizeof(plug_event_t));

            if (m_log_mutex == nullptr || m_queue == nullptr)
            {
                ESP_LOGE("plug", "Failed to create mutex or queue");
                return;
            }

            gpio_reset_pin(RELAY_GPIO);
            gpio_set_level(
                RELAY_GPIO,
                RELAY_ACTIVE_LOW ? 1 : 0
            );
            gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);

            gpio_reset_pin(LED_GPIO);
            gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

            AppNode::init();

            apply(false, false);

            ESP_LOGI("plug", "Smart plug initialization complete");
        }

        void start() override
        {
            ESP_LOGI("plug", "AppPlug start() called");

            AppNode::start();

            BaseType_t button_ok = xTaskCreate(
                buttonTask,
                "button_task",
                3072,
                this,
                5,
                nullptr
            );

            BaseType_t relay_ok = xTaskCreate(
                relayTask,
                "relay_task",
                3072,
                this,
                5,
                nullptr
            );

            ESP_LOGI(
                "plug",
                "Button task created: %d",
                button_ok
            );

            ESP_LOGI(
                "plug",
                "Relay task created: %d",
                relay_ok
            );
        }
    };

} // namespace app
