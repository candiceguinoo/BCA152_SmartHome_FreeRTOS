
idf_component_register(
    SRCS
        "main.c"
        "app_plug.c"
        "debounce.c"

    INCLUDE_DIRS
        "."
        "../include"

    REQUIRES
        ch9_common
        driver
        esp_rainmaker
)
