/*
 * @Description: Integrated LVGL factory test for T-Panel
 * @Author: LILYGO_L
 * @Date: 2026-05-29
 * @License: GPL 3.0
 */
#include <sys/lock.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <dirent.h>

#define TOUCH_MODULES_CST_MUTUAL

#include "TouchLib.h"
#include "cpp_bus_driver_library.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/twai.h"
#include "driver/uart.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_startup_images.h"
#include "nvs_flash.h"
#include "sd_protocol_defs.h"
#include "sdmmc_cmd.h"
#include "st7701_driver.h"
#include "t_panel_config.h"

namespace {

constexpr int kLvglTickPeriodMs = 1;
constexpr int kLvglTaskStackSize = 14 * 1024;
constexpr int kLvglTaskPriority = 2;
constexpr int kLvglRefreshPeriodMs = 20;
constexpr int kLvglTaskMinDelayMs = kLvglRefreshPeriodMs;
constexpr int kLvglTaskMaxDelayMs = 500;
constexpr int kLvglDrawBufferLines = 20;
constexpr int kBacklightFadeTimeMs = 800;
constexpr int kI2cFreqHz = 400000;
constexpr int kBytesPerPixel = 2;

constexpr uart_port_t kRs485UartPort = UART_NUM_2;
constexpr int kRs485BaudRate = 115200;
constexpr int kRs485BufferSize = 4096;
constexpr int kRs485PayloadSize = 1024;
constexpr int kRs485TxDoneTimeoutMs = 200;
constexpr char kRs485TestChar = 'R';

constexpr int kCanDataLength = 8;
constexpr uint32_t kCanTestId = 0x0F1;
constexpr char kCanTestChar = 'C';
constexpr uint32_t kCanAlertFlags =
    TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR |
    TWAI_ALERT_RX_DATA | TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_BUS_OFF |
    TWAI_ALERT_BUS_RECOVERED;

constexpr int kTestPrintIntervalMs = 1000;
constexpr int kWifiConnectTimeoutMs = 10000;
constexpr char kWifiSsid[] = "LilyGo-AABB";
constexpr char kWifiPassword[] = "xinyuandianzi";
constexpr int kBlePairTimeoutMs = 20000;
constexpr int kBlePollMs = 500;
constexpr int kBleSuccessHoldMs = 2000;
constexpr int kSdTaskStackSize = 8 * 1024;
constexpr int kSdTaskPriority = 1;
constexpr int kSdSpiMaxFreqKhz = 10000;
constexpr int kMaxSdTreeDepth = 4;
constexpr int kMaxSdTreeEntries = 32;
constexpr char kSdMountPoint[] = "/sdcard";
constexpr int kPageCount = 6;
constexpr int kDotHitSize = 42;
constexpr int kDotGap = 4;
constexpr int kDotRowWidth = kDotHitSize * kPageCount + kDotGap * (kPageCount - 1);
constexpr int kIndicatorHeight = 72;
constexpr int kPageWidth = t_panel::device::st7701::kWidth;
constexpr int kPageHeight = t_panel::device::st7701::kHeight;
constexpr int kCardWidth = 410;
constexpr int kCardHeight = 330;
constexpr int kPrimaryStatusPanelHeight = 190;
constexpr int kTouchStatusPanelHeight = 250;
constexpr int kBusStatusPanelHeight = 96;
constexpr int kDisplayTestEnterPixelClockHz = 11000000;
constexpr uint32_t kColorWindow = 0xF4F6FB;
constexpr uint32_t kColorSurface = 0xFFFBFE;
constexpr uint32_t kColorSurfaceContainer = 0xF0F3FA;
constexpr uint32_t kColorSurfaceContainerHigh = 0xE9EDF5;
constexpr uint32_t kColorPrimary = 0x2F6BFF;
constexpr uint32_t kColorPrimaryPressed = 0x1D56D6;
constexpr uint32_t kColorPrimaryContainer = 0xDCE7FF;
constexpr uint32_t kColorOnSurface = 0x1D1B20;
constexpr uint32_t kColorOnSurfaceVariant = 0x5E6470;
constexpr uint32_t kColorOutline = 0xD6DAE3;

enum class Page : int {
  kDisplay = 0,
  kTouch,
  kWifiTime,
  kRs485Can,
  kSd,
  kBle,
};

enum class LinkMode {
  kIdle,
  kRs485Send,
  kRs485Receive,
  kCanSend,
  kCanReceive,
};

enum class BleState {
  kIdle,
  kWaiting,
  kPassed,
  kFailed,
};

_lock_t g_lvgl_api_lock;

std::shared_ptr<cpp_bus_driver::HardwareI2c1> g_i2c_bus;
std::unique_ptr<cpp_bus_driver::Xl95x5> g_xl9535;
std::unique_ptr<cpp_bus_driver::HardwareI2c1> g_touch_i2c;
TouchLib g_touch;

lv_display_t* g_display = nullptr;
esp_lcd_panel_handle_t g_panel_handle = nullptr;
lv_obj_t* g_root = nullptr;
lv_obj_t* g_tileview = nullptr;
lv_obj_t* g_tiles[kPageCount] = {};
lv_obj_t* g_page_dots[kPageCount] = {};
lv_obj_t* g_page_name_label = nullptr;
lv_obj_t* g_display_label = nullptr;
lv_obj_t* g_fullscreen_display = nullptr;
lv_obj_t* g_touch_status = nullptr;
lv_obj_t* g_wifi_status = nullptr;
lv_obj_t* g_link_status = nullptr;
lv_obj_t* g_sd_status = nullptr;
lv_obj_t* g_ble_status = nullptr;

Page g_active_page = Page::kDisplay;
int g_display_pattern = 0;
bool g_wifi_ready = false;
bool g_wifi_started = false;
TaskHandle_t g_wifi_task_handle = nullptr;
TaskHandle_t g_link_task_handle = nullptr;
volatile bool g_link_stop = false;
LinkMode g_link_mode = LinkMode::kIdle;
bool g_sd_spi_bus_ready = false;
bool g_sd_mounted = false;
sdmmc_card_t* g_sd_card = nullptr;
TaskHandle_t g_sd_task_handle = nullptr;
TaskHandle_t g_ble_task_handle = nullptr;
BleState g_ble_state = BleState::kIdle;
lv_point_t g_touch_last = {};
volatile bool g_touch_int_flag = false;

const lv_image_dsc_t* const kWallpaperTable[] = {
    &kLvglStartupImage1,
    &kLvglStartupImage2,
    &kLvglStartupImage3,
};

void SetLabel(lv_obj_t* label, const std::string& text) {
  printf("%s\n", text.c_str());
  _lock_acquire(&g_lvgl_api_lock);
  if (label != nullptr) {
    lv_label_set_text(label, text.c_str());
  }
  _lock_release(&g_lvgl_api_lock);
}

void SetLabelUnlocked(lv_obj_t* label, const char* text) {
  printf("%s\n", text);
  if (label != nullptr) {
    lv_label_set_text(label, text);
  }
}

void SetLabelFmt(lv_obj_t* label, const char* fmt, ...) {
  char buffer[512] = {};
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  SetLabel(label, buffer);
}

bool InitXl9535() {
  g_i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c1>(
      t_panel::gpio::i2c::kSda, t_panel::gpio::i2c::kScl, I2C_NUM_0);
  g_xl9535 = std::make_unique<cpp_bus_driver::Xl95x5>(
      g_i2c_bus, t_panel::device::xl95x5::kI2cAddress);

  if (!g_xl9535->Init(kI2cFreqHz)) {
    printf("Init XL9535 failed\n");
    return false;
  }
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

bool InitTouch() {
  using Xl95x5Mode = cpp_bus_driver::Xl95x5::Mode;

  if (!g_xl9535->SetGpioMode(
          t_panel::gpio::xl95x5::kTouchRst, Xl95x5Mode::kOutput)) {
    printf("Set touch reset mode failed\n");
    return false;
  }
  g_xl9535->GpioWrite(t_panel::gpio::xl95x5::kTouchRst, 0);
  vTaskDelay(pdMS_TO_TICKS(200));
  g_xl9535->GpioWrite(t_panel::gpio::xl95x5::kTouchRst, 1);
  vTaskDelay(pdMS_TO_TICKS(200));

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

void IRAM_ATTR TouchInterruptHandler(void* arg) {
  g_touch_int_flag = true;
}

bool InitTouchInterrupt() {
  gpio_config_t io_config = {};
  io_config.pin_bit_mask = 1ULL << t_panel::gpio::cst3240::kInt;
  io_config.mode = GPIO_MODE_INPUT;
  io_config.pull_up_en = GPIO_PULLUP_ENABLE;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.intr_type = GPIO_INTR_NEGEDGE;
  esp_err_t err = gpio_config(&io_config);
  if (err != ESP_OK) {
    printf("Config touch interrupt failed: %s\n", esp_err_to_name(err));
    return false;
  }

  err = gpio_install_isr_service(0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    printf("Install GPIO ISR service failed: %s\n", esp_err_to_name(err));
    return false;
  }

  err = gpio_isr_handler_add(
      static_cast<gpio_num_t>(t_panel::gpio::cst3240::kInt),
      TouchInterruptHandler, nullptr);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    printf("Add touch interrupt handler failed: %s\n", esp_err_to_name(err));
    return false;
  }
  err = gpio_intr_enable(static_cast<gpio_num_t>(t_panel::gpio::cst3240::kInt));
  if (err != ESP_OK) {
    printf("Enable touch interrupt failed: %s\n", esp_err_to_name(err));
    return false;
  }

  g_touch_int_flag = false;
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

void UpdateTouchStatusUnlocked(
    const TP_Point& point, uint8_t point_count, bool pressed) {
  if (g_touch_status == nullptr) {
    return;
  }

  char buffer[192] = {};
  std::snprintf(buffer, sizeof(buffer),
      "Touch: %s\nPoints: %u\nX: %d\nY: %d\nPressure: %d\nState: %u",
      pressed ? "pressed" : "released", static_cast<unsigned>(point_count),
      point.x, point.y, point.pressure, static_cast<unsigned>(point.state));
  lv_label_set_text(g_touch_status, buffer);
}

void TouchpadRead(lv_indev_t* indev, lv_indev_data_t* data) {
  if (g_touch_int_flag) {
    g_touch_int_flag = false;
    g_touch.read();
    TP_Point point(0, 0, 0, 0, 0, 0);
    const uint8_t point_count = g_touch.getPointNum();
    if (point_count > 0) {
      point = g_touch.getPoint(0);
    }
    const bool pressed =
        point_count == 1 && point.pressure > 0 && point.state != 0;
    UpdateTouchStatusUnlocked(point, point_count, pressed);
    if (pressed) {
      data->state = LV_INDEV_STATE_PR;
      data->point.x =
          std::min<int>(point.x, t_panel::device::st7701::kWidth - 1);
      data->point.y =
          std::min<int>(point.y, t_panel::device::st7701::kHeight - 1);

      g_touch_last = data->point;
      return;
    }
  }

  data->state = LV_INDEV_STATE_REL;
  data->point = g_touch_last;
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

lv_obj_t* CreateButton(
    lv_obj_t* parent, const char* text, lv_event_cb_t cb, void* user_data) {
  lv_obj_t* button = lv_button_create(parent);
  lv_obj_set_height(button, 40);
  lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_pad_left(button, 18, 0);
  lv_obj_set_style_pad_right(button, 18, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(kColorPrimary), 0);
  lv_obj_set_style_transform_width(button, -3, LV_STATE_PRESSED);
  lv_obj_set_style_transform_height(button, -3, LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(
      button, lv_color_hex(kColorPrimaryPressed), LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(button, 10, 0);
  lv_obj_set_style_shadow_opa(button, LV_OPA_20, 0);
  lv_obj_set_style_shadow_offset_y(button, 4, 0);
  lv_obj_set_style_shadow_width(button, 2, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_offset_y(button, 1, LV_STATE_PRESSED);
  lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, user_data);

  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_center(label);
  return button;
}

lv_obj_t* CreateWrappedLabel(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_set_width(label, LV_PCT(100));
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(kColorOnSurfaceVariant), 0);
  lv_label_set_text(label, text);
  return label;
}

lv_obj_t* CreateCardTitle(lv_obj_t* parent, const char* text) {
  lv_obj_t* header = lv_obj_create(parent);
  lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_height(header, 34);
  lv_obj_set_style_pad_all(header, 0, 0);
  lv_obj_set_style_pad_column(header, 10, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
  lv_obj_set_layout(header, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_t* accent = lv_obj_create(header);
  lv_obj_remove_flag(accent, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(accent, 5, 24);
  lv_obj_set_style_radius(accent, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(accent, 0, 0);
  lv_obj_set_style_bg_color(accent, lv_color_hex(kColorPrimary), 0);

  lv_obj_t* label = lv_label_create(header);
  lv_obj_set_width(label, 320);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(kColorOnSurface), 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
  lv_label_set_text(label, text);
  return label;
}

lv_obj_t* CreateStatusPanel(
    lv_obj_t* parent, const char* text, int height, bool scrollable = false) {
  lv_obj_t* panel = lv_obj_create(parent);
  if (scrollable) {
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_AUTO);
  } else {
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);
  }
  lv_obj_set_width(panel, LV_PCT(100));
  lv_obj_set_height(panel, height);
  lv_obj_set_style_radius(panel, 12, 0);
  lv_obj_set_style_pad_left(panel, 14, 0);
  lv_obj_set_style_pad_right(panel, 14, 0);
  lv_obj_set_style_pad_top(panel, 12, 0);
  lv_obj_set_style_pad_bottom(panel, 12, 0);
  lv_obj_set_style_bg_color(panel, lv_color_hex(kColorSurfaceContainerHigh), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(kColorOutline), 0);

  lv_obj_t* label = CreateWrappedLabel(panel, text);
  lv_obj_set_style_text_color(label, lv_color_hex(kColorOnSurfaceVariant), 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_line_space(label, 4, 0);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
  return label;
}

lv_obj_t* CreatePageCard(lv_obj_t* tile) {
  lv_obj_t* card = lv_obj_create(tile);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(card, kCardWidth, kCardHeight);
  lv_obj_set_style_radius(card, 18, 0);
  lv_obj_set_style_pad_left(card, 20, 0);
  lv_obj_set_style_pad_right(card, 20, 0);
  lv_obj_set_style_pad_top(card, 16, 0);
  lv_obj_set_style_pad_bottom(card, 24, 0);
  lv_obj_set_style_pad_row(card, 10, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(kColorSurface), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(kColorOutline), 0);
  lv_obj_set_style_shadow_width(card, 14, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
  lv_obj_set_style_shadow_offset_y(card, 5, 0);
  lv_obj_set_layout(card, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);
  return card;
}

const char* ApplyDisplayPatternTo(lv_obj_t* target) {
  if (target == nullptr) {
    return "";
  }

  lv_obj_clean(target);
  lv_obj_set_style_bg_opa(target, LV_OPA_COVER, 0);
  const char* name = "";

  switch (g_display_pattern) {
    case 0:
      lv_obj_set_style_bg_color(target, lv_palette_main(LV_PALETTE_RED), 0);
      name = "Red";
      break;
    case 1:
      lv_obj_set_style_bg_color(target, lv_palette_main(LV_PALETTE_GREEN), 0);
      name = "Green";
      break;
    case 2:
      lv_obj_set_style_bg_color(target, lv_palette_main(LV_PALETTE_BLUE), 0);
      name = "Blue";
      break;
    case 3:
      lv_obj_set_style_bg_color(target, lv_color_white(), 0);
      name = "White";
      break;
    case 4:
      lv_obj_set_style_bg_color(target, lv_color_black(), 0);
      name = "Black";
      break;
    default: {
      const size_t image_index = static_cast<size_t>(g_display_pattern - 5);
      lv_obj_set_style_bg_color(target, lv_color_black(), 0);
      lv_obj_t* image = lv_image_create(target);
      lv_image_set_src(image, kWallpaperTable[image_index]);
      lv_obj_center(image);
      name = (image_index == 0) ? "Wallpaper 1"
                                : (image_index == 1) ? "Wallpaper 2"
                                                     : "Wallpaper 3";
      break;
    }
  }

  return name;
}

void ExitFullscreenDisplayTest() {
  if (g_panel_handle != nullptr) {
    esp_err_t err = esp_lcd_rgb_panel_set_pclk(
        g_panel_handle, t_panel::device::st7701::kPixelClockHz);
    if (err == ESP_OK) {
      printf("Display test exit RGB PCLK set to %d Hz\n",
          t_panel::device::st7701::kPixelClockHz);
    } else {
      printf("Set display test exit RGB PCLK to %d Hz failed: %s\n",
          t_panel::device::st7701::kPixelClockHz, esp_err_to_name(err));
    }
  }

  if (g_fullscreen_display != nullptr) {
    lv_obj_delete(g_fullscreen_display);
    g_fullscreen_display = nullptr;
  }
}

void FullscreenDisplayEvent(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  if (g_display_pattern >= 7) {
    ExitFullscreenDisplayTest();
    return;
  }

  g_display_pattern++;
  ApplyDisplayPatternTo(g_fullscreen_display);
}

void EnterFullscreenDisplayTest() {
  if (g_root == nullptr) {
    return;
  }
  if (g_panel_handle != nullptr) {
    esp_err_t err = esp_lcd_rgb_panel_set_pclk(
        g_panel_handle, kDisplayTestEnterPixelClockHz);
    if (err == ESP_OK) {
      printf("Display test RGB PCLK set to %d Hz\n",
          kDisplayTestEnterPixelClockHz);
    } else {
      printf("Set display test RGB PCLK to %d Hz failed: %s\n",
          kDisplayTestEnterPixelClockHz, esp_err_to_name(err));
    }
  }

  if (g_fullscreen_display != nullptr) {
    lv_obj_delete(g_fullscreen_display);
  }

  g_display_pattern = 0;
  g_fullscreen_display = lv_obj_create(g_root);
  lv_obj_remove_flag(g_fullscreen_display, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(g_fullscreen_display, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(g_fullscreen_display, kPageWidth, kPageHeight);
  lv_obj_set_pos(g_fullscreen_display, 0, 0);
  lv_obj_set_style_radius(g_fullscreen_display, 0, 0);
  lv_obj_set_style_border_width(g_fullscreen_display, 0, 0);
  lv_obj_set_style_pad_all(g_fullscreen_display, 0, 0);
  lv_obj_add_event_cb(
      g_fullscreen_display, FullscreenDisplayEvent, LV_EVENT_CLICKED, nullptr);
  ApplyDisplayPatternTo(g_fullscreen_display);
  lv_obj_move_foreground(g_fullscreen_display);
}

void StartDisplayTestEvent(lv_event_t* event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    EnterFullscreenDisplayTest();
  }
}

void WifiTask(void* arg) {
  SetLabel(g_wifi_status, "WiFi: init...");

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    SetLabelFmt(g_wifi_status, "NVS init failed: %s", esp_err_to_name(err));
    g_wifi_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  if (!g_wifi_ready) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wifi_init_config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      SetLabelFmt(
          g_wifi_status, "WiFi init failed: %s", esp_err_to_name(err));
      g_wifi_task_handle = nullptr;
      vTaskDelete(nullptr);
      return;
    }
    g_wifi_ready = true;
  }

  esp_wifi_set_mode(WIFI_MODE_STA);
  err = esp_wifi_start();
  if (err == ESP_OK || err == ESP_ERR_WIFI_CONN) {
    g_wifi_started = true;
  }
  esp_wifi_disconnect();

  SetLabel(g_wifi_status, "WiFi: scanning...");
  uint16_t ap_count = 0;
  wifi_scan_config_t scan_config = {};
  err = esp_wifi_scan_start(&scan_config, true);
  if (err == ESP_OK) {
    esp_wifi_scan_get_ap_num(&ap_count);
  }

  wifi_config_t wifi_config = {};
  std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
      kWifiSsid, sizeof(wifi_config.sta.ssid));
  std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
      kWifiPassword, sizeof(wifi_config.sta.password));
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  SetLabelFmt(g_wifi_status, "WiFi: %u APs found\nConnecting to %s...",
      static_cast<unsigned>(ap_count), kWifiSsid);
  err = esp_wifi_connect();
  if (err != ESP_OK) {
    SetLabelFmt(g_wifi_status, "WiFi connect failed: %s", esp_err_to_name(err));
    g_wifi_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  const int64_t start_us = esp_timer_get_time();
  wifi_ap_record_t ap_info = {};
  while ((esp_timer_get_time() - start_us) <
         static_cast<int64_t>(kWifiConnectTimeoutMs) * 1000) {
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }

  if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
    SetLabelFmt(g_wifi_status,
        "WiFi: connection timeout\nSSID: %s\nAPs found: %u",
        kWifiSsid, static_cast<unsigned>(ap_count));
    g_wifi_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  setenv("TZ", "CST-8", 1);
  tzset();
  if (!esp_sntp_enabled()) {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.nist.gov");
    esp_sntp_init();
  }

  tm time_info = {};
  bool time_ok = false;
  for (int i = 0; i < 20; ++i) {
    time_t now = time(nullptr);
    localtime_r(&now, &time_info);
    if (time_info.tm_year >= (2024 - 1900)) {
      time_ok = true;
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  if (time_ok) {
    char time_text[96] = {};
    std::strftime(time_text, sizeof(time_text), "%Y-%m-%d %H:%M:%S",
        &time_info);
    SetLabelFmt(g_wifi_status,
        "WiFi: connected\nSSID: %s\nRSSI: %d dBm\nTime: %s",
        kWifiSsid, ap_info.rssi, time_text);
  } else {
    SetLabelFmt(g_wifi_status,
        "WiFi: connected\nSSID: %s\nRSSI: %d dBm\nNTP time failed",
        kWifiSsid, ap_info.rssi);
  }

  g_wifi_task_handle = nullptr;
  vTaskDelete(nullptr);
}

void StartWifiTest(lv_event_t* event) {
  if (g_wifi_task_handle != nullptr) {
    SetLabelUnlocked(g_wifi_status, "WiFi test is already running...");
    return;
  }
  xTaskCreate(WifiTask, "WifiTask", 8 * 1024, nullptr, 3,
      &g_wifi_task_handle);
}

bool SetRs485Transmit(bool enable) {
  if (g_xl9535 == nullptr) {
    return false;
  }
  return g_xl9535->GpioWrite(
      t_panel::gpio::xl95x5::kRs485Con, enable ? 1 : 0);
}

const char* CanStateName(int state) {
  switch (state) {
    case TWAI_STATE_STOPPED:
      return "stopped";
    case TWAI_STATE_RUNNING:
      return "running";
    case TWAI_STATE_BUS_OFF:
      return "bus off";
    case TWAI_STATE_RECOVERING:
      return "recovering";
    default:
      return "unknown";
  }
}

std::string DescribeCanAlerts(uint32_t alerts) {
  std::string text;
  auto append = [&text](const char* name) {
    if (!text.empty()) {
      text += ",";
    }
    text += name;
  };

  if (alerts & TWAI_ALERT_TX_FAILED) {
    append("TX_FAILED");
  }
  if (alerts & TWAI_ALERT_ERR_PASS) {
    append("ERR_PASS");
  }
  if (alerts & TWAI_ALERT_BUS_ERROR) {
    append("BUS_ERROR");
  }
  if (alerts & TWAI_ALERT_RX_QUEUE_FULL) {
    append("RX_FULL");
  }
  if (alerts & TWAI_ALERT_BUS_OFF) {
    append("BUS_OFF");
  }
  if (alerts & TWAI_ALERT_BUS_RECOVERED) {
    append("RECOVERED");
  }
  return text.empty() ? "none" : text;
}

bool InitRs485(std::string* error) {
  if (g_xl9535 == nullptr) {
    *error = "XL9535 is not ready";
    return false;
  }
  if (!g_xl9535->SetGpioMode(t_panel::gpio::xl95x5::kRs485Con,
          cpp_bus_driver::Xl95x5::Mode::kOutput)) {
    *error = "RS485 direction pin config failed";
    return false;
  }
  if (!SetRs485Transmit(false)) {
    *error = "RS485 direction pin write failed";
    return false;
  }

  uart_config_t uart_config = {};
  uart_config.baud_rate = kRs485BaudRate;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  esp_err_t err = uart_driver_install(kRs485UartPort, kRs485BufferSize,
      kRs485BufferSize, 0, nullptr, 0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    *error = std::string("UART driver install failed: ") + esp_err_to_name(err);
    return false;
  }
  err = uart_param_config(kRs485UartPort, &uart_config);
  if (err != ESP_OK) {
    *error = std::string("UART param config failed: ") + esp_err_to_name(err);
    return false;
  }
  err = uart_set_pin(kRs485UartPort, t_panel::gpio::rs485::kTx,
      t_panel::gpio::rs485::kRx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (err != ESP_OK) {
    *error = std::string("UART pin config failed: ") + esp_err_to_name(err);
    return false;
  }
  return true;
}

void DeinitRs485() {
  SetRs485Transmit(false);
  uart_driver_delete(kRs485UartPort);
}

bool ValidateData(const uint8_t* data, size_t len, uint8_t expected) {
  for (size_t i = 0; i < len; ++i) {
    if (data[i] != expected) {
      return false;
    }
  }
  return true;
}

bool InitCan(std::string* error) {
  if (g_wifi_started) {
    esp_wifi_disconnect();
    esp_wifi_stop();
    g_wifi_started = false;
    printf("WiFi stopped before CAN test to free interrupt resources\n");
  }

  twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(
      static_cast<gpio_num_t>(t_panel::gpio::can::kTx),
      static_cast<gpio_num_t>(t_panel::gpio::can::kRx), TWAI_MODE_NORMAL);
  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err =
      twai_driver_install(&general_config, &timing_config, &filter_config);
  if (err != ESP_OK) {
    if (err == ESP_ERR_INVALID_STATE) {
      twai_driver_uninstall();
    }
    *error = std::string("TWAI driver install failed: ") + esp_err_to_name(err);
    printf("%s\n", error->c_str());
    return false;
  }
  err = twai_start();
  if (err != ESP_OK) {
    twai_driver_uninstall();
    *error = std::string("TWAI start failed: ") + esp_err_to_name(err);
    printf("%s\n", error->c_str());
    return false;
  }
  err = twai_reconfigure_alerts(kCanAlertFlags, nullptr);
  if (err != ESP_OK) {
    twai_stop();
    twai_driver_uninstall();
    *error = std::string("TWAI alerts config failed: ") + esp_err_to_name(err);
    printf("%s\n", error->c_str());
    return false;
  }
  return true;
}

void DeinitCan() {
  twai_stop();
  twai_driver_uninstall();
}

void LinkTask(void* arg) {
  const LinkMode mode = g_link_mode;
  const bool is_send =
      mode == LinkMode::kRs485Send || mode == LinkMode::kCanSend;
  const bool is_rs485 =
      mode == LinkMode::kRs485Send || mode == LinkMode::kRs485Receive;
  const char* bus_name = is_rs485 ? "RS485" : "CAN";
  const char* dir_name = is_send ? "send" : "receive";
  std::string init_error;

  if (is_rs485 && !InitRs485(&init_error)) {
    SetLabelFmt(g_link_status, "%s %s init error\n%s", bus_name, dir_name,
        init_error.c_str());
    g_link_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }
  if (!is_rs485 && !InitCan(&init_error)) {
    SetLabelFmt(g_link_status, "%s %s init error\n%s", bus_name, dir_name,
        init_error.c_str());
    g_link_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  size_t total_size = 0;
  size_t bytes_this_time = 0;
  int64_t last_print_us = esp_timer_get_time();
  int64_t start_us = last_print_us;
  std::string rs485_payload(kRs485PayloadSize, kRs485TestChar);
  std::string runtime_error;
  std::string last_error;

  if (mode == LinkMode::kRs485Send) {
    if (!SetRs485Transmit(true)) {
      runtime_error = "RS485 direction pin TX enable failed";
      g_link_stop = true;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  } else if (mode == LinkMode::kRs485Receive) {
    if (!SetRs485Transmit(false)) {
      runtime_error = "RS485 direction pin RX enable failed";
      g_link_stop = true;
    }
  }

  while (!g_link_stop) {
    if (mode == LinkMode::kRs485Send) {
      int len = uart_write_bytes(kRs485UartPort, rs485_payload.data(),
          rs485_payload.size());
      if (len > 0) {
        bytes_this_time += len;
        total_size += len;
        esp_err_t err = uart_wait_tx_done(
            kRs485UartPort, pdMS_TO_TICKS(kRs485TxDoneTimeoutMs));
        if (static_cast<size_t>(len) != rs485_payload.size()) {
          last_error = "RS485 partial write";
        }
        if (err == ESP_ERR_TIMEOUT) {
          last_error = "RS485 TX wait timeout";
        } else if (err != ESP_OK) {
          runtime_error =
              std::string("RS485 TX wait failed: ") + esp_err_to_name(err);
          break;
        }
      } else {
        runtime_error = "RS485 write failed";
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    } else if (mode == LinkMode::kRs485Receive) {
      size_t rx_len = 0;
      esp_err_t err = uart_get_buffered_data_len(kRs485UartPort, &rx_len);
      if (err != ESP_OK) {
        runtime_error =
            std::string("RS485 RX length failed: ") + esp_err_to_name(err);
        break;
      }
      if (rx_len > 0) {
        std::unique_ptr<uint8_t[]> buffer(new uint8_t[rx_len]);
        const int read_len = uart_read_bytes(kRs485UartPort, buffer.get(),
            rx_len, pdMS_TO_TICKS(20));
        if (read_len > 0) {
          if (!ValidateData(
                  buffer.get(), read_len, static_cast<uint8_t>(kRs485TestChar))) {
            runtime_error = "RS485 RX data corruption";
            break;
          }
          bytes_this_time += read_len;
          total_size += read_len;
        } else if (read_len < 0) {
          runtime_error = "RS485 read failed";
          break;
        }
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    } else if (mode == LinkMode::kCanSend) {
      uint32_t alerts = 0;
      esp_err_t err = twai_read_alerts(&alerts, 0);
      if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
        runtime_error =
            std::string("TWAI alert read failed: ") + esp_err_to_name(err);
        break;
      }
      if (alerts & (TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
                       TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL |
                       TWAI_ALERT_BUS_OFF)) {
        last_error = std::string("CAN alerts: ") + DescribeCanAlerts(alerts);
      }
      twai_status_info_t status = {};
      err = twai_get_status_info(&status);
      if (err != ESP_OK) {
        runtime_error =
            std::string("TWAI status read failed: ") + esp_err_to_name(err);
        break;
      }
      if (status.state == TWAI_STATE_RUNNING) {
        twai_message_t message = {};
        message.identifier = kCanTestId;
        message.data_length_code = kCanDataLength;
        std::memset(message.data, kCanTestChar, kCanDataLength);
        err = twai_transmit(&message, 0);
        if (err == ESP_OK) {
          bytes_this_time += kCanDataLength;
          total_size += kCanDataLength;
        } else {
          runtime_error =
              std::string("CAN transmit failed: ") + esp_err_to_name(err);
          break;
        }
      } else if (status.state == TWAI_STATE_BUS_OFF) {
        last_error = "CAN bus off, recovering";
        err = twai_initiate_recovery();
        if (err != ESP_OK) {
          runtime_error =
              std::string("CAN recovery failed: ") + esp_err_to_name(err);
          break;
        }
      } else {
        last_error = std::string("CAN state: ") + CanStateName(status.state);
      }
      vTaskDelay(pdMS_TO_TICKS(1));
    } else if (mode == LinkMode::kCanReceive) {
      uint32_t alerts = 0;
      esp_err_t err = twai_read_alerts(&alerts, pdMS_TO_TICKS(50));
      if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
        runtime_error =
            std::string("TWAI alert read failed: ") + esp_err_to_name(err);
        break;
      }
      if (alerts & (TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
                       TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL |
                       TWAI_ALERT_BUS_OFF)) {
        last_error = std::string("CAN alerts: ") + DescribeCanAlerts(alerts);
      }
      if (alerts & TWAI_ALERT_BUS_OFF) {
        err = twai_initiate_recovery();
        if (err != ESP_OK) {
          runtime_error =
              std::string("CAN recovery failed: ") + esp_err_to_name(err);
          break;
        }
      }
      if (alerts & TWAI_ALERT_BUS_RECOVERED) {
        err = twai_start();
        if (err != ESP_OK) {
          runtime_error =
              std::string("CAN restart failed: ") + esp_err_to_name(err);
          break;
        }
      }
      twai_message_t rx_message = {};
      while (twai_receive(&rx_message, 0) == ESP_OK) {
        if (rx_message.rtr || rx_message.extd ||
            rx_message.identifier != kCanTestId) {
          continue;
        }
        if (rx_message.data_length_code == kCanDataLength &&
            ValidateData(rx_message.data, rx_message.data_length_code,
                static_cast<uint8_t>(kCanTestChar))) {
          bytes_this_time += rx_message.data_length_code;
          total_size += rx_message.data_length_code;
        } else {
          runtime_error = "CAN RX data corruption";
          break;
        }
      }
      if (!runtime_error.empty()) {
        break;
      }
    }

    const int64_t now_us = esp_timer_get_time();
    if (now_us - last_print_us >= kTestPrintIntervalMs * 1000) {
      const float elapsed_s =
          static_cast<float>(now_us - last_print_us) / 1000000.0f;
      const float total_s =
          static_cast<float>(now_us - start_us) / 1000000.0f;
      const float speed =
          elapsed_s > 0.0f ? bytes_this_time / 1024.0f / elapsed_s : 0.0f;
      if (last_error.empty()) {
        SetLabelFmt(g_link_status,
            "%s %s running\nLast speed: %.2f KB/s\nTotal: %.2f KB\nTime: %.1f s",
            bus_name, dir_name, speed, total_size / 1024.0f, total_s);
      } else {
        SetLabelFmt(g_link_status,
            "%s %s running\nLast speed: %.2f KB/s\nTotal: %.2f KB\n%s",
            bus_name, dir_name, speed, total_size / 1024.0f,
            last_error.c_str());
      }
      bytes_this_time = 0;
      last_print_us = now_us;
    }
  }

  if (is_rs485) {
    DeinitRs485();
  } else {
    DeinitCan();
  }

  g_link_stop = false;
  g_link_mode = LinkMode::kIdle;
  if (!runtime_error.empty()) {
    SetLabelFmt(g_link_status, "%s %s error\n%s\nTotal: %.2f KB", bus_name,
        dir_name, runtime_error.c_str(), total_size / 1024.0f);
  } else if (!last_error.empty()) {
    SetLabelFmt(g_link_status, "%s %s stopped\nTotal: %.2f KB\nLast: %s",
        bus_name, dir_name, total_size / 1024.0f, last_error.c_str());
  } else {
    SetLabelFmt(g_link_status, "%s %s stopped\nTotal: %.2f KB", bus_name,
        dir_name, total_size / 1024.0f);
  }
  g_link_task_handle = nullptr;
  vTaskDelete(nullptr);
}

void StopLinkTest() {
  if (g_link_task_handle == nullptr) {
    g_link_mode = LinkMode::kIdle;
    g_link_stop = false;
    return;
  }
  g_link_stop = true;
}

void StartLinkTest(LinkMode mode) {
  if (g_link_task_handle != nullptr) {
    g_link_stop = true;
    SetLabelUnlocked(g_link_status,
        "Stopping previous bus test...\nTap the next mode again after it stops.");
    return;
  }

  g_link_stop = false;
  g_link_mode = mode;
  xTaskCreate(LinkTask, "LinkTask", 7 * 1024, nullptr, 5,
      &g_link_task_handle);
}

void LinkButtonEvent(lv_event_t* event) {
  LinkMode mode =
      static_cast<LinkMode>(reinterpret_cast<intptr_t>(lv_event_get_user_data(
          event)));
  if (mode == LinkMode::kIdle) {
    StopLinkTest();
  } else {
    StartLinkTest(mode);
  }
}

bool InitSdSpiBus(std::string* error) {
  if (g_sd_spi_bus_ready) {
    return true;
  }

  spi_bus_config_t bus_config = {};
  bus_config.mosi_io_num = t_panel::gpio::sd::kMosi;
  bus_config.miso_io_num = t_panel::gpio::sd::kMiso;
  bus_config.sclk_io_num = t_panel::gpio::sd::kSclk;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  bus_config.max_transfer_sz = 4000;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  esp_err_t err = spi_bus_initialize(static_cast<spi_host_device_t>(host.slot),
      &bus_config, SDSPI_DEFAULT_DMA);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    *error = std::string("SPI bus init failed: ") + esp_err_to_name(err);
    return false;
  }

  g_sd_spi_bus_ready = true;
  return true;
}

const char* SdCardTypeName(const sdmmc_card_t* card) {
  if (card == nullptr) {
    return "Unknown";
  }
  if (card->is_sdio) {
    return "SDIO";
  }
  if (card->is_mmc) {
    return "MMC";
  }
  return (card->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC";
}

void AppendSdDirectoryTree(
    const std::string& path, int depth, int* entry_count, std::string* out) {
  if (depth > kMaxSdTreeDepth || *entry_count >= kMaxSdTreeEntries) {
    return;
  }

  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }

  while (*entry_count < kMaxSdTreeEntries) {
    dirent* entry = readdir(dir);
    if (entry == nullptr) {
      break;
    }
    if (std::strcmp(entry->d_name, ".") == 0 ||
        std::strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    std::string child_path = path;
    if (child_path.back() != '/') {
      child_path += "/";
    }
    child_path += entry->d_name;

    struct stat stat_buffer = {};
    const bool stat_ok = stat(child_path.c_str(), &stat_buffer) == 0;
    const bool is_dir = stat_ok && S_ISDIR(stat_buffer.st_mode);

    out->append(depth * 2, ' ');
    out->append(is_dir ? "[D] " : "[F] ");
    out->append(entry->d_name);
    if (!is_dir && stat_ok) {
      char size_text[32] = {};
      std::snprintf(size_text, sizeof(size_text), " (%ld B)",
          static_cast<long>(stat_buffer.st_size));
      out->append(size_text);
    }
    out->append("\n");
    (*entry_count)++;

    if (is_dir) {
      AppendSdDirectoryTree(child_path, depth + 1, entry_count, out);
    }
  }

  closedir(dir);
}

std::string BuildSdMountedStatusText() {
  std::string text;
  text.reserve(4096);

  const uint64_t card_size_mb =
      static_cast<uint64_t>(g_sd_card->csd.capacity) *
      g_sd_card->csd.sector_size / (1024ULL * 1024ULL);
  char header[192] = {};
  std::snprintf(header, sizeof(header),
      "SD card mounted\nType: %s\nSize: %llu MB\nMount: %s\n\nDirectory:\n",
      SdCardTypeName(g_sd_card), static_cast<unsigned long long>(card_size_mb),
      kSdMountPoint);
  text += header;

  int entry_count = 0;
  AppendSdDirectoryTree(kSdMountPoint, 0, &entry_count, &text);
  if (entry_count == 0) {
    text += "(empty)\n";
  } else if (entry_count >= kMaxSdTreeEntries) {
    text += "... output truncated\n";
  }

  return text;
}

bool MountSdCard(std::string* error) {
  if (!InitSdSpiBus(error)) {
    return false;
  }

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = kSdSpiMaxFreqKhz;

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.host_id = static_cast<spi_host_device_t>(host.slot);
  slot_config.gpio_cs = static_cast<gpio_num_t>(t_panel::gpio::sd::kCs);

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 8;
  mount_config.allocation_unit_size = 16 * 1024;
  mount_config.disk_status_check_enable = true;

  sdmmc_card_t* card = nullptr;
  esp_err_t err = esp_vfs_fat_sdspi_mount(
      kSdMountPoint, &host, &slot_config, &mount_config, &card);
  if (err != ESP_OK) {
    *error = std::string("SD mount failed: ") + esp_err_to_name(err);
    return false;
  }

  g_sd_card = card;
  g_sd_mounted = true;
  sdmmc_card_print_info(stdout, g_sd_card);
  return true;
}

void UnmountSdCard() {
  if (g_sd_mounted && g_sd_card != nullptr) {
    esp_vfs_fat_sdcard_unmount(kSdMountPoint, g_sd_card);
  }
  g_sd_card = nullptr;
  g_sd_mounted = false;
}

bool SdCardStillReadable() {
  if (!g_sd_mounted || g_sd_card == nullptr) {
    return false;
  }
  if (sdmmc_get_status(g_sd_card) != ESP_OK) {
    return false;
  }

  DIR* dir = opendir(kSdMountPoint);
  if (dir == nullptr) {
    return false;
  }
  closedir(dir);
  return true;
}

void SdTask(void* arg) {
  SetLabel(g_sd_status, "SD card scanning...");

  if (g_sd_mounted && !SdCardStillReadable()) {
    UnmountSdCard();
  }

  std::string error;
  if (!g_sd_mounted && !MountSdCard(&error)) {
    SetLabelFmt(g_sd_status, "SD card test failed\n%s\nCheck card and wiring.",
        error.c_str());
  } else {
    SetLabel(g_sd_status, BuildSdMountedStatusText());
  }

  g_sd_task_handle = nullptr;
  vTaskDelete(nullptr);
}

void StartSdTest(lv_event_t* event) {
  if (g_sd_task_handle != nullptr) {
    SetLabelUnlocked(g_sd_status, "SD card test is already running...");
    return;
  }

  if (xTaskCreate(SdTask, "SdTask", kSdTaskStackSize, nullptr, kSdTaskPriority,
          &g_sd_task_handle) != pdPASS) {
    SetLabelUnlocked(g_sd_status, "Create SD card test task failed");
  }
}

void InitEsp32h2Pins() {
  gpio_config_t io_config = {};
  io_config.pin_bit_mask = (1ULL << t_panel::gpio::esp32h2::kEn) |
                           (1ULL << t_panel::gpio::esp32h2::kBoot);
  io_config.mode = GPIO_MODE_OUTPUT;
  io_config.pull_up_en = GPIO_PULLUP_DISABLE;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&io_config);

  gpio_set_level(static_cast<gpio_num_t>(t_panel::gpio::esp32h2::kBoot), 1);
  gpio_set_level(static_cast<gpio_num_t>(t_panel::gpio::esp32h2::kEn), 1);

  g_xl9535->SetGpioMode(t_panel::gpio::xl95x5::kEsp32h2Io12,
      cpp_bus_driver::Xl95x5::Mode::kOutput);
  g_xl9535->GpioWrite(t_panel::gpio::xl95x5::kEsp32h2Io12, 0);
  g_xl9535->SetGpioMode(t_panel::gpio::xl95x5::kEsp32h2Io4,
      cpp_bus_driver::Xl95x5::Mode::kInput);
  g_xl9535->SetGpioMode(t_panel::gpio::xl95x5::kEsp32h2Io5,
      cpp_bus_driver::Xl95x5::Mode::kInput);
}

void ResetEsp32h2() {
  gpio_set_level(static_cast<gpio_num_t>(t_panel::gpio::esp32h2::kEn), 0);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(static_cast<gpio_num_t>(t_panel::gpio::esp32h2::kEn), 1);
  vTaskDelay(pdMS_TO_TICKS(1000));
}

void BleTask(void* arg) {
  g_ble_state = BleState::kWaiting;
  g_xl9535->GpioWrite(t_panel::gpio::xl95x5::kEsp32h2Io12, 0);
  ResetEsp32h2();

  const int64_t start_us = esp_timer_get_time();
  while (esp_timer_get_time() - start_us <
         static_cast<int64_t>(kBlePairTimeoutMs) * 1000) {
    const bool io4_high =
        g_xl9535->GpioRead(t_panel::gpio::xl95x5::kEsp32h2Io4) != 0;
    const bool io5_high =
        g_xl9535->GpioRead(t_panel::gpio::xl95x5::kEsp32h2Io5) != 0;
    const int remain_s = (kBlePairTimeoutMs -
                             static_cast<int>((esp_timer_get_time() - start_us) /
                                 1000)) /
                         1000;

    SetLabelFmt(g_ble_status,
        "ESP32-H2 BLE\nName: T-Panel_V1.2_ESP32-H2\nState: waiting pairing\nIO4: %s  IO5: %s\nTimeout: %d s",
        io4_high ? "HIGH" : "LOW", io5_high ? "HIGH" : "LOW", remain_s);
    if (io4_high && io5_high) {
      g_ble_state = BleState::kPassed;
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(kBlePollMs));
  }

  if (g_ble_state == BleState::kPassed) {
    SetLabel(g_ble_status,
        "ESP32-H2 BLE\nName: T-Panel_V1.2_ESP32-H2\nBLE pairing successful");
    vTaskDelay(pdMS_TO_TICKS(kBleSuccessHoldMs));
  } else {
    g_ble_state = BleState::kFailed;
    g_xl9535->GpioWrite(t_panel::gpio::xl95x5::kEsp32h2Io12, 1);
    SetLabel(g_ble_status,
        "ESP32-H2 BLE\nName: T-Panel_V1.2_ESP32-H2\nBLE pairing failed");
  }

  g_ble_task_handle = nullptr;
  vTaskDelete(nullptr);
}

void StartBleTest(lv_event_t* event) {
  if (g_ble_task_handle != nullptr) {
    SetLabelUnlocked(g_ble_status, "ESP32-H2 BLE test is already running...");
    return;
  }
  xTaskCreate(BleTask, "BleTask", 5 * 1024, nullptr, 3, &g_ble_task_handle);
}

void BuildDisplayPage(lv_obj_t* parent) {
  lv_obj_t* card = CreatePageCard(parent);
  CreateCardTitle(card, "Display test");
  g_display_label = CreateStatusPanel(card,
      "Open full-screen color and wallpaper checks. Tap the screen to advance.",
      kPrimaryStatusPanelHeight);
  lv_obj_t* button =
      CreateButton(card, "Start LCD picture test", StartDisplayTestEvent,
          nullptr);
  lv_obj_set_width(button, 230);
}

void BuildTouchPage(lv_obj_t* parent) {
  lv_obj_t* card = CreatePageCard(parent);
  CreateCardTitle(card, "Touch test");
  g_touch_status = CreateStatusPanel(card,
      "Touch the screen to show coordinates, pressure and touch state.",
      kTouchStatusPanelHeight);
}

void BuildWifiPage(lv_obj_t* parent) {
  lv_obj_t* card = CreatePageCard(parent);
  CreateCardTitle(card, "WiFi and time");
  g_wifi_status =
      CreateStatusPanel(card, "Tap Start to scan WiFi and get NTP time.",
          kPrimaryStatusPanelHeight);
  CreateButton(card, "Start WiFi time test", StartWifiTest, nullptr);
}

void BuildLinkPage(lv_obj_t* parent) {
  lv_obj_t* card = CreatePageCard(parent);
  CreateCardTitle(card, "RS485 and CAN");
  g_link_status =
      CreateStatusPanel(card, "Select a module and direction.",
          kBusStatusPanelHeight);
  lv_obj_t* grid = lv_obj_create(card);
  lv_obj_set_width(grid, LV_PCT(100));
  lv_obj_set_height(grid, 124);
  lv_obj_set_style_pad_all(grid, 0, 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
  lv_obj_set_layout(grid, LV_LAYOUT_GRID);
  static int32_t cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static int32_t rows[] = {
      LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(grid, cols, rows);
  lv_obj_set_style_pad_row(grid, 8, 0);
  lv_obj_set_style_pad_column(grid, 8, 0);

  lv_obj_t* b = CreateButton(grid, "RS485 send", LinkButtonEvent,
      reinterpret_cast<void*>(static_cast<intptr_t>(LinkMode::kRs485Send)));
  lv_obj_set_height(b, 32);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0, 1);
  b = CreateButton(grid, "RS485 receive", LinkButtonEvent,
      reinterpret_cast<void*>(static_cast<intptr_t>(LinkMode::kRs485Receive)));
  lv_obj_set_height(b, 32);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 0, 1);
  b = CreateButton(grid, "CAN send", LinkButtonEvent,
      reinterpret_cast<void*>(static_cast<intptr_t>(LinkMode::kCanSend)));
  lv_obj_set_height(b, 32);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 1, 1);
  b = CreateButton(grid, "CAN receive", LinkButtonEvent,
      reinterpret_cast<void*>(static_cast<intptr_t>(LinkMode::kCanReceive)));
  lv_obj_set_height(b, 32);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 1, 1);
  b = CreateButton(grid, "Stop", LinkButtonEvent,
      reinterpret_cast<void*>(static_cast<intptr_t>(LinkMode::kIdle)));
  lv_obj_set_height(b, 32);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 2, 1);
}

void BuildSdPage(lv_obj_t* parent) {
  lv_obj_t* card = CreatePageCard(parent);
  CreateCardTitle(card, "SD card");
  g_sd_status = CreateStatusPanel(card,
      "Tap Start to mount the SD card and print the directory tree.",
      kPrimaryStatusPanelHeight, true);
  lv_obj_t* button =
      CreateButton(card, "Start SD card scan", StartSdTest, nullptr);
  lv_obj_set_width(button, 230);
}

void BuildBlePage(lv_obj_t* parent) {
  lv_obj_t* card = CreatePageCard(parent);
  CreateCardTitle(card, "ESP32-H2 BLE");
  g_ble_status = CreateStatusPanel(card,
      "Name: T-Panel_V1.2_ESP32-H2\nTap Start to reset ESP32-H2 and wait for pairing.",
      kPrimaryStatusPanelHeight);
  CreateButton(card, "Start BLE pairing test", StartBleTest, nullptr);
}

void UpdatePageIndicator() {
  static constexpr const char* kPageNames[kPageCount] = {
      "LCD", "Touch", "WiFi", "Bus", "SD", "BLE"};
  for (int i = 0; i < kPageCount; ++i) {
    if (g_page_dots[i] == nullptr) {
      continue;
    }
    const bool selected = i == static_cast<int>(g_active_page);
    lv_obj_t* dot = lv_obj_get_child(g_page_dots[i], 0);
    if (dot == nullptr) {
      continue;
    }
    lv_obj_set_size(dot, selected ? 18 : 13, selected ? 18 : 13);
    lv_obj_set_style_bg_color(g_page_dots[i],
        selected ? lv_color_hex(kColorPrimaryContainer)
                 : lv_color_hex(kColorSurfaceContainer),
        0);
    lv_obj_set_style_bg_color(dot,
        selected ? lv_color_hex(kColorPrimary) : lv_color_hex(0xAEB6C2),
        0);
  }

  if (g_page_name_label != nullptr) {
    lv_label_set_text(g_page_name_label, kPageNames[static_cast<int>(
                                           g_active_page)]);
  }
}

int TileIndexFromObject(lv_obj_t* tile) {
  for (int i = 0; i < kPageCount; ++i) {
    if (g_tiles[i] == tile) {
      return i;
    }
  }
  return 0;
}

void TileviewEvent(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED ||
      g_tileview == nullptr) {
    return;
  }

  g_active_page = static_cast<Page>(
      TileIndexFromObject(lv_tileview_get_tile_active(g_tileview)));
  UpdatePageIndicator();
}

void ShowPage(Page page, bool animate = true) {
  g_active_page = page;
  if (g_tileview != nullptr) {
    lv_tileview_set_tile_by_index(g_tileview, static_cast<uint32_t>(page), 0,
        animate ? LV_ANIM_ON : LV_ANIM_OFF);
  }
  UpdatePageIndicator();
}

void DotButtonEvent(lv_event_t* event) {
  int page = static_cast<int>(reinterpret_cast<intptr_t>(
      lv_event_get_user_data(event)));
  ShowPage(static_cast<Page>(page));
}

void CreateUi() {
  lv_theme_default_init(g_display, lv_palette_main(LV_PALETTE_BLUE),
      lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_14);

  g_root = lv_display_get_screen_active(g_display);
  lv_obj_set_style_bg_color(g_root, lv_color_hex(kColorWindow), 0);
  lv_obj_set_style_bg_opa(g_root, LV_OPA_COVER, 0);
  lv_obj_set_scrollbar_mode(g_root, LV_SCROLLBAR_MODE_OFF);
  lv_obj_remove_flag(g_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(g_root, 0, 0);
  lv_obj_set_layout(g_root, LV_LAYOUT_GRID);

  static int32_t root_cols[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static int32_t root_rows[] = {
      46, LV_GRID_FR(1), kIndicatorHeight, LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(g_root, root_cols, root_rows);
  lv_obj_set_style_pad_row(g_root, 4, 0);

  lv_obj_t* header = lv_obj_create(g_root);
  lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_height(header, LV_PCT(100));
  lv_obj_set_style_pad_all(header, 0, 0);
  lv_obj_set_style_pad_row(header, 0, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
  lv_obj_set_layout(header, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);
  lv_obj_set_grid_cell(
      header, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  lv_obj_t* title = lv_label_create(header);
  lv_label_set_text(title, "T-Panel General Test");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(kColorOnSurface), 0);

  g_tileview = lv_tileview_create(g_root);
  lv_obj_set_width(g_tileview, LV_PCT(100));
  lv_obj_set_height(g_tileview, LV_PCT(100));
  lv_obj_set_style_pad_all(g_tileview, 0, 0);
  lv_obj_set_style_bg_opa(g_tileview, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_tileview, 0, 0);
  lv_obj_set_scrollbar_mode(g_tileview, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_grid_cell(
      g_tileview, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_add_event_cb(g_tileview, TileviewEvent, LV_EVENT_VALUE_CHANGED,
      nullptr);

  for (int i = 0; i < kPageCount; ++i) {
    lv_dir_t dirs = static_cast<lv_dir_t>(LV_DIR_LEFT | LV_DIR_RIGHT);
    if (i == 0) {
      dirs = LV_DIR_RIGHT;
    } else if (i == kPageCount - 1) {
      dirs = LV_DIR_LEFT;
    }
    g_tiles[i] = lv_tileview_add_tile(g_tileview, i, 0, dirs);
    lv_obj_remove_flag(g_tiles[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(g_tiles[i], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_tiles[i], 0, 0);
    lv_obj_set_style_pad_all(g_tiles[i], 0, 0);
    lv_obj_set_scrollbar_mode(g_tiles[i], LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(g_tiles[i], LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_tiles[i], LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g_tiles[i], LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  }

  BuildDisplayPage(g_tiles[static_cast<int>(Page::kDisplay)]);
  BuildTouchPage(g_tiles[static_cast<int>(Page::kTouch)]);
  BuildWifiPage(g_tiles[static_cast<int>(Page::kWifiTime)]);
  BuildLinkPage(g_tiles[static_cast<int>(Page::kRs485Can)]);
  BuildSdPage(g_tiles[static_cast<int>(Page::kSd)]);
  BuildBlePage(g_tiles[static_cast<int>(Page::kBle)]);

  lv_obj_t* indicator = lv_obj_create(g_root);
  lv_obj_set_width(indicator, LV_PCT(100));
  lv_obj_set_height(indicator, kIndicatorHeight);
  lv_obj_remove_flag(indicator, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(indicator, 0, 0);
  lv_obj_set_style_pad_row(indicator, 4, 0);
  lv_obj_set_style_border_width(indicator, 0, 0);
  lv_obj_set_style_bg_opa(indicator, LV_OPA_TRANSP, 0);
  lv_obj_set_layout(indicator, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(indicator, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      indicator, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);
  lv_obj_set_grid_cell(
      indicator, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_END, 2, 1);

  g_page_name_label = lv_label_create(indicator);
  lv_obj_set_style_text_color(
      g_page_name_label, lv_color_hex(kColorOnSurface), 0);

  lv_obj_t* dot_row = lv_obj_create(indicator);
  lv_obj_remove_flag(dot_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(dot_row, kDotRowWidth, kDotHitSize);
  lv_obj_set_style_pad_all(dot_row, 0, 0);
  lv_obj_set_style_pad_column(dot_row, kDotGap, 0);
  lv_obj_set_style_border_width(dot_row, 0, 0);
  lv_obj_set_style_bg_opa(dot_row, LV_OPA_TRANSP, 0);
  lv_obj_set_layout(dot_row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(dot_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(dot_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  for (int i = 0; i < kPageCount; ++i) {
    g_page_dots[i] = lv_obj_create(dot_row);
    lv_obj_add_flag(g_page_dots[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(g_page_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(g_page_dots[i], kDotHitSize, kDotHitSize);
    lv_obj_set_style_radius(g_page_dots[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(g_page_dots[i], 0, 0);
    lv_obj_add_event_cb(g_page_dots[i], DotButtonEvent, LV_EVENT_CLICKED,
        reinterpret_cast<void*>(static_cast<intptr_t>(i)));

    lv_obj_t* dot = lv_obj_create(g_page_dots[i]);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(dot, 13, 13);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_center(dot);
  }

  g_active_page = Page::kDisplay;
  ShowPage(Page::kDisplay, false);
  UpdatePageIndicator();
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

  CreateUi();

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

  esp_lcd_panel_handle_t panel_handle = nullptr;
  if (!InitXl9535()) {
    return;
  }
  if (!InitTouch()) {
    printf("Init touch failed\n");
    return;
  }
  if (!InitTouchInterrupt()) {
    printf("Init touch interrupt failed\n");
    return;
  }
  InitEsp32h2Pins();

  printf("Init ST7701 start\n");
  if (!st7701_driver::InitSt7701(g_xl9535.get(), &panel_handle)) {
    printf("Init ST7701 failed\n");
    return;
  }
  g_panel_handle = panel_handle;
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
