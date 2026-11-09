#ifndef APP_NODE_H
#define APP_NODE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_rmaker_core.h"

typedef void (*app_node_device_init_cb_t)(
    esp_rmaker_device_t *device,
    void *arg
);

typedef void (*app_node_event_cb_t)(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
);

typedef struct
{
    const char *node_name;
    const char *node_type;
    const char *device_name;
    const char *device_type;

    app_node_device_init_cb_t init_device;
    void *init_device_arg;

} app_node_params_t;

typedef struct
{
    esp_rmaker_node_t *rmaker_node;
    esp_rmaker_device_t *device;

    bool connected;

    app_node_params_t init_params;

    app_node_event_cb_t event_callback;
    void *event_callback_arg;

} app_node_t;

esp_err_t app_node_init(
    app_node_t *node,
    const app_node_params_t *params
);

esp_err_t app_node_start(app_node_t *node);

void app_node_add_event_handler(
    app_node_t *node,
    app_node_event_cb_t callback,
    void *arg
);

bool app_node_is_connected(const app_node_t *node);

#endif