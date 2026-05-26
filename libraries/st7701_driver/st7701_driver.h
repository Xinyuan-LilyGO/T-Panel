#pragma once

#include "cpp_bus_driver_library.h"
#include "esp_lcd_panel_ops.h"

namespace st7701_driver {

bool InitSt7701(
    cpp_bus_driver::Xl95x5* xl9535, esp_lcd_panel_handle_t* panel_handle);
bool InitBacklight();
bool SetBacklight(uint8_t duty);
bool StartBacklightGradient(uint8_t target_duty, int32_t time_ms);

}  // namespace st7701_driver
