if(CONFIG_EXAMPLE_BUILD_SCREEN)
    set(project_path "examples/screen/")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_LVGL)
    set(project_path "examples/screen_lvgl/")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_LVGL_TOUCH_DRAW)
    set(project_path "examples/screen_lvgl_touch_draw/")
elseif(CONFIG_EXAMPLE_BUILD_SD)
    set(project_path "examples/sd/")
elseif(CONFIG_EXAMPLE_BUILD_RS485_CAN)
    set(project_path "examples/rs485_can/")
elseif(CONFIG_EXAMPLE_BUILD_GENERAL_TEST)
    set(project_path "examples/general_test/")
endif()

idf_component_register(
    SRC_DIRS ${project_path}
    INCLUDE_DIRS ${project_path}
    REQUIRES
    "esp_driver_gpio"
    "esp_driver_i2c"
    "esp_driver_sdspi"
    "esp_driver_spi"
    "esp_driver_twai"
    "esp_driver_uart"
    "esp_driver_usb_serial_jtag"
    "esp_event"
    "esp_netif"
    "esp_timer"
    "esp_wifi"
    "fatfs"
    "lwip"
    "nvs_flash"
    "sdmmc"
)
