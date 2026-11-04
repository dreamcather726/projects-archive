idf_component_register(
    SRCS 
        "main.cpp"
        "../lib/LED.cpp"
        "../lib/motor.cpp"
        "../lib/uart.cpp"

    INCLUDE_DIRS
        "."
        "../include"
    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_ledc
        esp_driver_uart
        freertos
        log
)
