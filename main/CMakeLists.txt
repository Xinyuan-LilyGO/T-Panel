if(CONFIG_EXAMPLE_BUILD_SCREEN)
    set(project_path "examples/screen/")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_LVGL)
    set(project_path "examples/screen_lvgl/")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_LVGL_TOUCH_DRAW)
    set(project_path "examples/screen_lvgl_touch_draw/")
elseif(CONFIG_EXAMPLE_BUILD_SD)
    set(project_path "examples/sd/")
    set(requires "fatfs" "sdmmc" "esp_driver_sdspi" "esp_driver_spi")
elseif(CONFIG_EXAMPLE_BUILD_RS485_CAN)
    set(project_path "examples/rs485_can/")
    set(requires "esp_driver_twai" "esp_driver_uart" "esp_driver_usb_serial_jtag" "esp_timer")
endif()

idf_component_register(
    SRC_DIRS ${project_path}
    INCLUDE_DIRS ${project_path}
    REQUIRES ${requires}
)
