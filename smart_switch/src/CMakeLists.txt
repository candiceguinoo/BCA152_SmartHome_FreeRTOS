idf_component_register(
    SRCS
        "main.cpp"
        "debounce.c"

    INCLUDE_DIRS
        "."
        "../include"

    REQUIRES
        ch9_common
        esp_driver_gpio
        esp_rainmaker
)
