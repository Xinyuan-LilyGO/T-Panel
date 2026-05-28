if(CONFIG_EXAMPLE_BUILD_SCREEN)
    set(project_path "examples/screen/")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_LVGL)
    set(project_path "examples/screen_lvgl/")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_LVGL_TOUCH_DRAW)
    set(project_path "examples/screen_lvgl_touch_draw/")
elseif(CONFIG_EXAMPLE_BUILD_SD)
    set(project_path "examples/sd/")
    set(requires "fatfs" "sdmmc" "esp_driver_sdspi" "esp_driver_spi")
endif()

idf_component_register(
    SRC_DIRS ${project_path}
    INCLUDE_DIRS ${project_path}
    REQUIRES ${requires}
)
