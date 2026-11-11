#ifndef APP_MULTISENSOR_H
#define APP_MULTISENSOR_H

#include "esp_err.h"

esp_err_t app_multisensor_init(void);
esp_err_t app_multisensor_start(void);

esp_err_t app_multisensor_report(
    float temperature,
    int humidity,
    int light
);

#endif
