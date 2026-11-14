idf_component_register(
    SRCS
        "app_driver.c"
        "app_node.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        nvs_flash
        esp_event
        esp_rainmaker
        rmaker_common
        rmaker_app_network
        rmaker_app_insights
)