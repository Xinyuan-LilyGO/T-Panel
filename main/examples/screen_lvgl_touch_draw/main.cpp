/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-05-26 16:17:47
 * @LastEditTime: 2026-05-28 14:49:29
 * @License: GPL 3.0
 */
#include <sys/lock.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#define TOUCH_MODULES_CST_MUTUAL

#include "TouchLib.h"
#include "cpp_bus_driver_library.h"
#include "driver/i2c_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "st7701_driver.h"
#include "t_panel_config.h"

namespace {

constexpr int kLvglTickPeriodMs = 1;
constexpr int kLvglTaskStackSize = 12 * 1024;
constexpr int kLvglTaskPriority = 2;
constexpr int kLvglRefreshPeriodMs = 33;
constexpr int kLvglTaskMinDelayMs = kLvglRefreshPeriodMs;
constexpr int kLvglTaskMaxDelayMs = 500;
constexpr int kLvglDrawBufferLines = 20;
constexpr int kBacklightFadeTimeMs = 800;
constexpr int kI2cFreqHz = 400000;
constexpr int kBytesPerPixel = 2;
constexpr int kTouchDrawPixelClockHz = 8000000;
constexpr int64_t kCanvasClearTimeoutUs = 5 * 1000 * 1000;

_lock_t g_lvgl_api_lock;

std::shared_ptr<cpp_bus_driver::HardwareI2c1> g_i2c_bus;
std::unique_ptr<cpp_bus_driver::HardwareI2c1> g_touch_i2c;
TouchLib g_touch;

lv_display_t* g_display = nullptr;
lv_obj_t* g_canvas = nullptr;
void* g_canvas_buffer = nullptr;
lv_point_t g_last_point = {};
bool g_has_last_point = false;
bool g_need_clear_canvas = false;
int64_t g_last_touch_us = 0;

bool InitXl9535(std::unique_ptr<cpp_bus_driver::Xl95x5>* xl9535) {
  g_i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c1>(
      t_panel::gpio::i2c::kSda, t_panel::gpio::i2c::kScl, I2C_NUM_0);
  auto expander = std::make_unique<cpp_bus_driver::Xl95x5>(
      g_i2c_bus, t_panel::device::xl95x5::kI2cAddress);

  if (!expander->Init(kI2cFreqHz)) {
    printf("Init XL9535 failed\n");
    return false;
  }

  *xl9535 = std::move(expander);
  return true;
}

int TouchI2cRead(
    uint8_t dev_addr, uint16_t reg_addr, uint8_t* data, uint8_t len) {
  if (g_touch_i2c == nullptr || data == nullptr) {
    return -1;
  }

  uint8_t reg_buffer[2] = {};
  size_t reg_size = 1;
  if (reg_addr > 0xFF) {
    reg_buffer[0] = static_cast<uint8_t>(reg_addr >> 8);
    reg_buffer[1] = static_cast<uint8_t>(reg_addr & 0xFF);
    reg_size = 2;
  } else {
    reg_buffer[0] = static_cast<uint8_t>(reg_addr);
  }

  return g_touch_i2c->WriteRead(reg_buffer, reg_size, data, len) ? 0 : -1;
}

int TouchI2cWrite(
    uint8_t dev_addr, uint16_t reg_addr, uint8_t* data, uint8_t len) {
  if (g_touch_i2c == nullptr) {
    return -1;
  }

  uint8_t write_buffer[34] = {};
  size_t write_size = 1;
  if (reg_addr > 0xFF) {
    if (len + 2 > sizeof(write_buffer)) {
      return -1;
    }
    write_buffer[0] = static_cast<uint8_t>(reg_addr >> 8);
    write_buffer[1] = static_cast<uint8_t>(reg_addr & 0xFF);
    write_size = 2;
  } else {
    if (len + 1 > sizeof(write_buffer)) {
      return -1;
    }
    write_buffer[0] = static_cast<uint8_t>(reg_addr);
  }

  if (len > 0 && data != nullptr) {
    std::copy(data, data + len, write_buffer + write_size);
  }
  write_size += len;

  return g_touch_i2c->Write(write_buffer, write_size) ? 0 : -1;
}

bool ResetTouch(cpp_bus_driver::Xl95x5* xl9535) {
  using Xl95x5Mode = cpp_bus_driver::Xl95x5::Mode;

  if (!xl9535->SetGpioMode(
          t_panel::gpio::xl95x5::kTouchRst, Xl95x5Mode::kOutput)) {
    printf("Set touch reset mode failed\n");
    return false;
  }
  if (!xl9535->GpioWrite(t_panel::gpio::xl95x5::kTouchRst, 0)) {
    printf("Set touch reset low failed\n");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(200));
  if (!xl9535->GpioWrite(t_panel::gpio::xl95x5::kTouchRst, 1)) {
    printf("Set touch reset high failed\n");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(200));

  return true;
}

bool InitTouch(cpp_bus_driver::Xl95x5* xl9535) {
  if (!ResetTouch(xl9535)) {
    return false;
  }

  g_touch_i2c = std::make_unique<cpp_bus_driver::HardwareI2c1>(g_i2c_bus);
  if (!g_touch_i2c->Init(kI2cFreqHz, t_panel::device::cst3240::kI2cAddress)) {
    printf("Init touch I2C failed\n");
    return false;
  }

  if (!g_touch.begin(t_panel::device::cst3240::kI2cAddress, -1, TouchI2cRead,
          TouchI2cWrite)) {
    printf("Init TouchLib failed\n");
    return false;
  }

  printf("Init CST3240 touch done\n");
  return true;
}

bool NotifyLvglFlushReady(esp_lcd_panel_handle_t panel,
    const esp_lcd_rgb_panel_event_data_t* event_data, void* user_ctx) {
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

void IncreaseLvglTick(void* arg) { lv_tick_inc(kLvglTickPeriodMs); }

void ClearCanvas() {
  if (g_canvas == nullptr) {
    return;
  }

  lv_canvas_fill_bg(g_canvas, lv_color_hex(0xD6D8DE), LV_OPA_COVER);
  g_has_last_point = false;
  g_need_clear_canvas = false;
}

void DrawLineOnCanvas(const lv_point_t& p1, const lv_point_t& p2) {
  if (g_canvas == nullptr) {
    return;
  }

  lv_layer_t layer;
  lv_canvas_init_layer(g_canvas, &layer);

  lv_draw_line_dsc_t dsc;
  lv_draw_line_dsc_init(&dsc);
  dsc.color = lv_palette_main(LV_PALETTE_RED);
  dsc.width = 4;
  dsc.round_end = 1;
  dsc.round_start = 1;
  dsc.p1.x = p1.x;
  dsc.p1.y = p1.y;
  dsc.p2.x = p2.x;
  dsc.p2.y = p2.y;
  lv_draw_line(&layer, &dsc);

  lv_canvas_finish_layer(g_canvas, &layer);
}

void DrawPoint(lv_event_t* event) {
  const lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_PRESSING) {
    lv_indev_t* indev = lv_indev_get_act();
    if (indev == nullptr) {
      return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);

    if (g_has_last_point) {
      DrawLineOnCanvas(g_last_point, point);
    }
    g_last_point = point;
    g_has_last_point = true;
    g_need_clear_canvas = true;
    g_last_touch_us = esp_timer_get_time();
  } else if (code == LV_EVENT_RELEASED) {
    g_has_last_point = false;
  }
}

void TouchpadRead(lv_indev_t* indev, lv_indev_data_t* data) {
  if (g_touch.read() && g_touch.getPointNum() > 0) {
    TP_Point point = g_touch.getPoint(0);
    data->state = LV_INDEV_STATE_PR;
    data->point.x = std::min<int>(point.x, t_panel::device::st7701::kWidth - 1);
    data->point.y =
        std::min<int>(point.y, t_panel::device::st7701::kHeight - 1);
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

bool CreateDrawCanvas() {
  const size_t canvas_buffer_size = t_panel::device::st7701::kWidth *
                                    t_panel::device::st7701::kHeight *
                                    kBytesPerPixel;
  g_canvas_buffer =
      heap_caps_malloc(canvas_buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (g_canvas_buffer == nullptr) {
    printf("Allocate canvas buffer failed\n");
    return false;
  }

  lv_obj_t* screen = lv_display_get_screen_active(g_display);
  lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

  g_canvas = lv_canvas_create(screen);
  if (g_canvas == nullptr) {
    printf("Create canvas failed\n");
    return false;
  }

  lv_canvas_set_buffer(g_canvas, g_canvas_buffer,
      t_panel::device::st7701::kWidth, t_panel::device::st7701::kHeight,
      LV_COLOR_FORMAT_RGB565);
  ClearCanvas();
  lv_obj_center(g_canvas);
  lv_obj_add_flag(g_canvas, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(g_canvas, DrawPoint, LV_EVENT_ALL, nullptr);

  return true;
}

void LvglTask(void* arg) {
  printf("LvglTask start\n");

  while (true) {
    _lock_acquire(&g_lvgl_api_lock);
    if (g_need_clear_canvas &&
        (esp_timer_get_time() - g_last_touch_us > kCanvasClearTimeoutUs)) {
      ClearCanvas();
    }
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

  g_display = lv_display_create(
      t_panel::device::st7701::kWidth, t_panel::device::st7701::kHeight);
  if (g_display == nullptr) {
    printf("Create LVGL display failed\n");
    return false;
  }
  lv_display_set_user_data(g_display, panel_handle);
  lv_display_set_color_format(g_display, LV_COLOR_FORMAT_RGB565);
  lv_timer_t* refresh_timer = lv_display_get_refr_timer(g_display);
  if (refresh_timer != nullptr) {
    lv_timer_set_period(refresh_timer, kLvglRefreshPeriodMs);
  }

  const size_t draw_buffer_size =
      t_panel::device::st7701::kWidth * kLvglDrawBufferLines * kBytesPerPixel;
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

  lv_display_set_buffers(g_display, draw_buffer1, draw_buffer2,
      draw_buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(g_display, LvglFlush);

  esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
  callbacks.on_color_trans_done = NotifyLvglFlushReady;
  if (esp_lcd_rgb_panel_register_event_callbacks(
          panel_handle, &callbacks, g_display) != ESP_OK) {
    printf("Register RGB panel callbacks failed\n");
    return false;
  }

  lv_indev_t* indev = lv_indev_create();
  if (indev == nullptr) {
    printf("Create LVGL input device failed\n");
    return false;
  }
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, TouchpadRead);

  if (!CreateDrawCanvas()) {
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
  if (esp_timer_start_periodic(tick_timer, kLvglTickPeriodMs * 1000) !=
      ESP_OK) {
    printf("Start LVGL tick timer failed\n");
    return false;
  }

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
  if (!InitTouch(xl9535.get())) {
    printf("Init touch failed\n");
    return;
  }
  printf("Init ST7701 start\n");
  if (!st7701_driver::InitSt7701(xl9535.get(), &panel_handle)) {
    printf("Init ST7701 failed\n");
    return;
  }
  if (esp_lcd_rgb_panel_set_pclk(panel_handle, kTouchDrawPixelClockHz) !=
      ESP_OK) {
    printf("Set touch draw RGB PCLK failed\n");
    return;
  }
  printf("Set touch draw RGB PCLK to %d Hz\n", kTouchDrawPixelClockHz);
  printf("Init ST7701 done\n");
  if (!st7701_driver::InitBacklight()) {
    printf("Init backlight failed\n");
    return;
  }

  if (!InitLvgl(panel_handle)) {
    printf("Init LVGL failed\n");
    return;
  }

  st7701_driver::StartBacklightGradient(100, kBacklightFadeTimeMs);
}
