/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-05-26 16:17:47
 * @LastEditTime: 2026-05-28 14:43:10
 * @License: GPL 3.0
 */
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <sys/lock.h>
#include <unistd.h>

#include "cpp_bus_driver_library.h"
#include "driver/i2c_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_startup_images.h"
#include "st7701_driver.h"
#include "t_panel_config.h"

namespace {

constexpr int kLvglTickPeriodMs = 1;
constexpr int kLvglTaskStackSize = 12 * 1024;
constexpr int kLvglTaskPriority = 2;
constexpr int kLvglRefreshPeriodMs = 10;
constexpr int kLvglTaskMinDelayMs = kLvglRefreshPeriodMs;
constexpr int kLvglTaskMaxDelayMs = 500;
constexpr int kLvglDrawBufferLines = 20;
constexpr int kImagePlayIntervalMs = 1000;
constexpr int kBacklightFadeTimeMs = 800;
constexpr int kXl9535I2cFreqHz = 400000;
constexpr int kBytesPerPixel = 2;

_lock_t g_lvgl_api_lock;

extern "C" void example_lvgl_demo_ui(lv_display_t* display);

const lv_image_dsc_t* const kImageTable[] = {
    &kLvglStartupImage1,
    &kLvglStartupImage2,
    &kLvglStartupImage3,
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

bool ShowStartupImages(lv_display_t* display) {
  lv_obj_t* screen = lv_display_get_screen_active(display);
  lv_obj_clean(screen);
  lv_obj_set_style_bg_color(screen, lv_color_white(), 0);

  lv_obj_t* image = lv_image_create(screen);
  if (image == nullptr) {
    printf("Create startup image object failed\n");
    return false;
  }

  for (size_t i = 0; i < kImageCount; ++i) {
    lv_image_set_src(image, kImageTable[i]);
    lv_obj_center(image);
    lv_refr_now(display);
    vTaskDelay(pdMS_TO_TICKS(kImagePlayIntervalMs));
  }

  lv_obj_delete(image);
  lv_obj_clean(screen);
  lv_refr_now(display);
  return true;
}

bool NotifyLvglFlushReady(
    esp_lcd_panel_handle_t panel,
    const esp_lcd_rgb_panel_event_data_t* event_data,
    void* user_ctx) {
  lv_display_t* display = static_cast<lv_display_t*>(user_ctx);
  lv_display_flush_ready(display);
  return false;
}

void LvglFlush(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
  esp_lcd_panel_handle_t panel_handle =
      static_cast<esp_lcd_panel_handle_t>(lv_display_get_user_data(display));

  if (esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1,
          area->y2 + 1, px_map) != ESP_OK) {
    printf("LVGL flush failed\n");
    lv_display_flush_ready(display);
  }
}

void IncreaseLvglTick(void* arg) {
  lv_tick_inc(kLvglTickPeriodMs);
}

void LvglTask(void* arg) {
  printf("LvglTask start\n");

  while (true) {
    _lock_acquire(&g_lvgl_api_lock);
    uint32_t delay_ms = lv_timer_handler();
    _lock_release(&g_lvgl_api_lock);

    if (delay_ms < kLvglTaskMinDelayMs) {
      delay_ms = kLvglTaskMinDelayMs;
    } else if (delay_ms > kLvglTaskMaxDelayMs) {
      delay_ms = kLvglTaskMaxDelayMs;
    }
    usleep(delay_ms * 1000);
  }
}

bool InitLvgl(esp_lcd_panel_handle_t panel_handle) {
  lv_init();

  lv_display_t* display = lv_display_create(
      t_panel::device::st7701::kWidth, t_panel::device::st7701::kHeight);
  if (display == nullptr) {
    printf("Create LVGL display failed\n");
    return false;
  }
  lv_display_set_user_data(display, panel_handle);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_timer_t* refresh_timer = lv_display_get_refr_timer(display);
  if (refresh_timer != nullptr) {
    lv_timer_set_period(refresh_timer, kLvglRefreshPeriodMs);
  }

  const size_t draw_buffer_size = t_panel::device::st7701::kWidth *
                                  kLvglDrawBufferLines * kBytesPerPixel;
  void* draw_buffer1 =
      heap_caps_malloc(draw_buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  void* draw_buffer2 =
      heap_caps_malloc(draw_buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  if (draw_buffer1 == nullptr || draw_buffer2 == nullptr) {
    printf("Allocate LVGL draw buffers failed\n");
    if (draw_buffer1 != nullptr) {
      heap_caps_free(draw_buffer1);
    }
    if (draw_buffer2 != nullptr) {
      heap_caps_free(draw_buffer2);
    }
    return false;
  }

  lv_display_set_buffers(
      display, draw_buffer1, draw_buffer2, draw_buffer_size,
      LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, LvglFlush);

  esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
  callbacks.on_color_trans_done = NotifyLvglFlushReady;
  if (esp_lcd_rgb_panel_register_event_callbacks(
          panel_handle, &callbacks, display) != ESP_OK) {
    printf("Register RGB panel callbacks failed\n");
    return false;
  }

  if (!ShowStartupImages(display)) {
    printf("Show startup images failed\n");
    return false;
  }

  const esp_timer_create_args_t tick_timer_args = {
      .callback = IncreaseLvglTick,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "lvgl_tick",
      .skip_unhandled_events = false,
  };
  esp_timer_handle_t tick_timer = nullptr;
  if (esp_timer_create(&tick_timer_args, &tick_timer) != ESP_OK) {
    printf("Create LVGL tick timer failed\n");
    return false;
  }
  if (esp_timer_start_periodic(
          tick_timer, kLvglTickPeriodMs * 1000) != ESP_OK) {
    printf("Start LVGL tick timer failed\n");
    return false;
  }

  _lock_acquire(&g_lvgl_api_lock);
  example_lvgl_demo_ui(display);
  _lock_release(&g_lvgl_api_lock);

  if (xTaskCreate(LvglTask, "LvglTask", kLvglTaskStackSize, nullptr,
          kLvglTaskPriority, nullptr) != pdPASS) {
    printf("Create LVGL task failed\n");
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
  printf("Init ST7701 start\n");
  if (!st7701_driver::InitSt7701(xl9535.get(), &panel_handle)) {
    printf("Init ST7701 failed\n");
    return;
  }
  printf("Init ST7701 done\n");
  if (!st7701_driver::InitBacklight()) {
    printf("Init backlight failed\n");
    return;
  }
  printf("Init LVGL start\n");
  st7701_driver::StartBacklightGradient(100, kBacklightFadeTimeMs);

  if (!InitLvgl(panel_handle)) {
    printf("Init LVGL failed\n");
    return;
  }
}
