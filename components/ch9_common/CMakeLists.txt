idf_component_register(INCLUDE_DIRS "include"
                       REQUIRES nvs_flash esp_event esp_rainmaker rmaker_common
                                rmaker_app_network rmaker_app_insights)