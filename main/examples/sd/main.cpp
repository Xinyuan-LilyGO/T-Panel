#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <string>
#include <sys/lock.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cpp_bus_driver_library.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdmmc_cmd.h"
#include "sd_protocol_defs.h"
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
constexpr int kXl9535I2cFreqHz = 400000;
constexpr int kBytesPerPixel = 2;
constexpr int kSdTaskStackSize = 8 * 1024;
constexpr int kSdTaskPriority = 1;
constexpr int kSdPollPeriodMs = 1000;
constexpr int kSdSpiMaxFreqKhz = 10000;
constexpr int kMaxTreeDepth = 4;
constexpr int kMaxTreeEntries = 32;
constexpr char kMountPoint[] = "/sdcard";

_lock_t g_lvgl_api_lock;
lv_display_t* g_display = nullptr;
lv_obj_t* g_status_label = nullptr;
bool g_spi_bus_ready = false;
bool g_sd_mounted = false;
sdmmc_card_t* g_sd_card = nullptr;

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

void SetStatusText(const std::string& text) {
  printf("%s\n", text.c_str());

  _lock_acquire(&g_lvgl_api_lock);
  if (g_status_label != nullptr) {
    lv_label_set_text(g_status_label, text.c_str());
  }
  _lock_release(&g_lvgl_api_lock);
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

  lv_obj_t* screen = lv_display_get_screen_active(g_display);
  lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

  g_status_label = lv_label_create(screen);
  if (g_status_label == nullptr) {
    printf("Create status label failed\n");
    return false;
  }
  lv_obj_set_width(g_status_label, t_panel::device::st7701::kWidth - 32);
  lv_label_set_long_mode(g_status_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(g_status_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(g_status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(g_status_label, LV_ALIGN_TOP_LEFT, 16, 16);
  lv_label_set_text(g_status_label, "SD card monitor starting...");

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

  if (xTaskCreate(LvglTask, "LvglTask", kLvglTaskStackSize, nullptr,
          kLvglTaskPriority, nullptr) != pdPASS) {
    printf("Create LVGL task failed\n");
    return false;
  }

  return true;
}

bool InitSdSpiBus() {
  if (g_spi_bus_ready) {
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
  esp_err_t result =
      spi_bus_initialize(static_cast<spi_host_device_t>(host.slot),
          &bus_config, SDSPI_DEFAULT_DMA);
  if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
    printf("Init SD SPI bus failed: %s\n", esp_err_to_name(result));
    return false;
  }

  g_spi_bus_ready = true;
  return true;
}

const char* CardTypeName(const sdmmc_card_t* card) {
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

void AppendDirectoryTree(
    const std::string& path, int depth, int* entry_count, std::string* out) {
  if (depth > kMaxTreeDepth || *entry_count >= kMaxTreeEntries) {
    return;
  }

  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }

  while (*entry_count < kMaxTreeEntries) {
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
      AppendDirectoryTree(child_path, depth + 1, entry_count, out);
    }
  }

  closedir(dir);
}

std::string BuildMountedStatusText() {
  std::string text;
  text.reserve(4096);

  char header[192] = {};
  const uint64_t card_size_mb =
      static_cast<uint64_t>(g_sd_card->csd.capacity) *
      g_sd_card->csd.sector_size / (1024ULL * 1024ULL);
  std::snprintf(header, sizeof(header),
      "SD card inserted\nType: %s\nSize: %llu MB\nMount: %s\n\nDirectory:\n",
      CardTypeName(g_sd_card), static_cast<unsigned long long>(card_size_mb),
      kMountPoint);
  text += header;

  int entry_count = 0;
  AppendDirectoryTree(kMountPoint, 0, &entry_count, &text);
  if (entry_count == 0) {
    text += "(empty)\n";
  } else if (entry_count >= kMaxTreeEntries) {
    text += "... output truncated\n";
  }

  return text;
}

bool MountSdCard() {
  if (!InitSdSpiBus()) {
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
  esp_err_t result = esp_vfs_fat_sdspi_mount(
      kMountPoint, &host, &slot_config, &mount_config, &card);
  if (result != ESP_OK) {
    printf("Mount SD card failed: %s\n", esp_err_to_name(result));
    return false;
  }

  g_sd_card = card;
  g_sd_mounted = true;
  sdmmc_card_print_info(stdout, g_sd_card);
  SetStatusText(BuildMountedStatusText());
  return true;
}

void UnmountSdCard(const char* reason) {
  if (g_sd_mounted && g_sd_card != nullptr) {
    esp_vfs_fat_sdcard_unmount(kMountPoint, g_sd_card);
  }
  g_sd_card = nullptr;
  g_sd_mounted = false;

  std::string text = "SD card not mounted\n";
  if (reason != nullptr) {
    text += "Status: ";
    text += reason;
    text += "\n";
  }
  text += "\nInsert SD card to list directory tree.";
  SetStatusText(text);
}

bool MountedCardStillReadable() {
  if (!g_sd_mounted || g_sd_card == nullptr) {
    return false;
  }

  if (sdmmc_get_status(g_sd_card) != ESP_OK) {
    return false;
  }

  DIR* dir = opendir(kMountPoint);
  if (dir == nullptr) {
    return false;
  }
  closedir(dir);
  return true;
}

void SdTask(void* arg) {
  UnmountSdCard("Waiting for card");

  while (true) {
    if (!g_sd_mounted) {
      if (!MountSdCard()) {
        SetStatusText("SD card not detected\n\nWaiting for insertion...");
      }
    } else if (!MountedCardStillReadable()) {
      UnmountSdCard("Card removed or read failed");
    }

    vTaskDelay(pdMS_TO_TICKS(kSdPollPeriodMs));
  }
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
  if (!InitLvgl(panel_handle)) {
    printf("Init LVGL failed\n");
    return;
  }

  st7701_driver::StartBacklightGradient(100, kBacklightFadeTimeMs);

  if (xTaskCreate(
          SdTask, "SdTask", kSdTaskStackSize, nullptr, kSdTaskPriority,
          nullptr) != pdPASS) {
    SetStatusText("Create SD monitor task failed");
    return;
  }
}
