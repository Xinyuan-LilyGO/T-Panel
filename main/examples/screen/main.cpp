#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#include "cpp_bus_driver_library.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "material_16bit_480x480px.h"
#include "st7701_driver.h"
#include "t_panel_config.h"

namespace {

constexpr int kImagePlayIntervalMs = 1000;
constexpr int kBacklightFadeTimeMs = 800;
constexpr int kXl9535I2cFreqHz = 400000;

const uint8_t* const kImageTable[] = {
    gImage_1,
    gImage_2,
    gImage_3,
    gImage_4,
    gImage_5,
};

constexpr size_t kImageCount = sizeof(kImageTable) / sizeof(kImageTable[0]);

bool InitXl9535(std::unique_ptr<cpp_bus_driver::Xl95x5>* xl9535) {
  auto i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c1>(
      t_panel::gpio::i2c::kSda, t_panel::gpio::i2c::kScl, I2C_NUM_0);
  auto expander = std::make_unique<cpp_bus_driver::Xl95x5>(
      i2c_bus, t_panel::device::xl95x5::kI2cAddress);

  if (!expander->Init(kXl9535I2cFreqHz)) {
    printf("Init XL9535 failed\n");
    return false;
  }

  *xl9535 = std::move(expander);
  return true;
}

bool ShowImage(esp_lcd_panel_handle_t panel_handle, size_t image_index) {
  if (esp_lcd_panel_draw_bitmap(panel_handle, 0, 0,
          t_panel::device::st7701::kWidth, t_panel::device::st7701::kHeight,
          kImageTable[image_index]) != ESP_OK) {
    printf("Show image %u failed\n", static_cast<unsigned>(image_index));
    return false;
  }

  return true;
}

}  // namespace

extern "C" void app_main(void) {
  printf("Ciallo\n");

  std::unique_ptr<cpp_bus_driver::Xl95x5> xl9535;
  esp_lcd_panel_handle_t panel_handle = nullptr;

  if (!InitXl9535(&xl9535)) {
    return;
  }
  if (!st7701_driver::InitSt7701(xl9535.get(), &panel_handle)) {
    printf("InitSt7701 failed\n");
    return;
  }
  if (!st7701_driver::InitBacklight()) {
    printf("InitBacklight failed\n");
    return;
  }

  if (!ShowImage(panel_handle, 0)) {
    printf("ShowImage failed\n");
    return;
  }
  st7701_driver::StartBacklightGradient(100, kBacklightFadeTimeMs);

  size_t image_index = 1;
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(kImagePlayIntervalMs));
    ShowImage(panel_handle, image_index);
    image_index = (image_index + 1) % kImageCount;
  }
}
