
#ifndef APP_PLUG_H
#define APP_PLUG_H

#include <stdbool.h>

#include "app_node.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

typedef enum
{
    PLUG_EVT_TOGGLE,
    PLUG_EVT_ON,
    PLUG_EVT_OFF
} plug_event_t;

typedef struct
{
    app_node_t node;

    esp_rmaker_param_t *power_param;

    QueueHandle_t queue;
    SemaphoreHandle_t log_mutex;

    bool state;

} app_plug_t;

esp_err_t app_plug_init(app_plug_t *plug);
esp_err_t app_plug_start(app_plug_t *plug);

#endif
