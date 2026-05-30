#include "st7701_driver.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "driver/gpio.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "t_panel_config.h"

namespace st7701_driver {
namespace {

constexpr uint32_t kSclHalfPeriodUs = 1;

using Xl95x5Pin = cpp_bus_driver::Xl95x5::Pin;
using Xl95x5Mode = cpp_bus_driver::Xl95x5::Mode;

struct InitCommand {
  uint8_t command;
  std::array<uint8_t, 16> data;
  size_t data_size;
  uint32_t delay_ms;
};

struct ThreeWireSpi {
  cpp_bus_driver::Xl95x5* xl9535;
  uint8_t port1_shadow;
  uint8_t cs_mask;
  uint8_t scl_mask;
  uint8_t sda_mask;
};

static constexpr InitCommand kInitSequence[] = {
    {0xFF, {0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, {0x08}, 1, 0},
    {0xFF, {0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, {0x3B, 0x00}, 2, 0},
    {0xC1, {0x0B, 0x02}, 2, 0},
    {0xC2, {0x30, 0x02, 0x37}, 3, 0},
    // {0xC3, {0x80}, 1, 0},  // 同步模式
    {0xCC, {0x10}, 1, 0},
    {0xB0,
        {0x00, 0x0F, 0x16, 0x0E, 0x11, 0x07, 0x09, 0x09, 0x08, 0x23, 0x05, 0x11,
            0x0F, 0x28, 0x2D, 0x18},
        16, 0},
    {0xB1,
        {0x00, 0x0F, 0x16, 0x0E, 0x11, 0x07, 0x09, 0x08, 0x09, 0x23, 0x05, 0x11,
            0x0F, 0x28, 0x2D, 0x18},
        16, 0},
    {0xFF, {0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, {0x4D}, 1, 0},
    {0xB1, {0x33}, 1, 0},
    {0xB2, {0x87}, 1, 0},
    {0xB5, {0x4B}, 1, 0},
    {0xB7, {0x8C}, 1, 0},
    {0xB8, {0x20}, 1, 0},
    {0xC1, {0x78}, 1, 0},
    {0xC2, {0x78}, 1, 0},
    {0xD0, {0x88}, 1, 0},
    {0xE0, {0x00, 0x00, 0x02}, 3, 0},
    {0xE1, {0x02, 0xF0, 0x00, 0x00, 0x03, 0xF0, 0x00, 0x00, 0x00, 0x44, 0x44},
        11, 0},
    {0xE2,
        {0x10, 0x10, 0x40, 0x40, 0xF2, 0xF0, 0x00, 0x00, 0xF2, 0xF0, 0x00,
            0x00},
        12, 0},
    {0xE3, {0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, {0x44, 0x44}, 2, 0},
    {0xE5,
        {0x07, 0xEF, 0xF0, 0xF0, 0x09, 0xF1, 0xF0, 0xF0, 0x03, 0xF3, 0xF0, 0xF0,
            0x05, 0xED, 0xF0, 0xF0},
        16, 0},
    {0xE6, {0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, {0x44, 0x44}, 2, 0},
    {0xE8,
        {0x08, 0xF0, 0xF0, 0xF0, 0x0A, 0xF2, 0xF0, 0xF0, 0x04, 0xF4, 0xF0, 0xF0,
            0x06, 0xEE, 0xF0, 0xF0},
        16, 0},
    {0xEB, {0x00, 0x00, 0xE4, 0xE4, 0x44, 0x88, 0x40}, 7, 0},
    {0xEC, {0x78, 0x00}, 2, 0},
    {0xED,
        {0x20, 0xF9, 0x87, 0x76, 0x65, 0x54, 0x4F, 0xFF, 0xFF, 0xF4, 0x45, 0x56,
            0x67, 0x78, 0x9F, 0x02},
        16, 0},
    {0xEF, {0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}, 6, 0},
    {0x3A, {0x55}, 1, 0},
    {0x36, {0x00}, 1, 0},
    {0x11, {}, 0, 120},
    {0x29, {}, 0, 20},
};

void DelayUs(uint32_t delay_us) {
  if (delay_us >= 1000) {
    vTaskDelay(pdMS_TO_TICKS(delay_us / 1000));
  } else if (delay_us > 0) {
    esp_rom_delay_us(delay_us);
  }
}

bool Xl9535Write(
    cpp_bus_driver::Xl95x5* xl9535, Xl95x5Pin pin, uint32_t level) {
  return xl9535->GpioWrite(pin, level ? 1 : 0);
}

uint8_t Port1Mask(Xl95x5Pin pin) {
  return 1 << (static_cast<uint8_t>(pin) - 10);
}

bool SpiWritePort1(ThreeWireSpi* spi) {
  return spi->xl9535->GpioWrite(Xl95x5Pin::kIoPort1, spi->port1_shadow);
}

bool SpiSetLine(ThreeWireSpi* spi, uint8_t mask, bool level) {
  if (level) {
    spi->port1_shadow |= mask;
  } else {
    spi->port1_shadow &= ~mask;
  }

  return SpiWritePort1(spi);
}

bool SpiWriteByte(ThreeWireSpi* spi, int dc_bit, uint8_t data) {
  const uint8_t data_bits = (dc_bit >= 0) ? 9 : 8;
  uint16_t data_temp = data;

  for (uint8_t i = 0; i < data_bits; i++) {
    const bool is_dc_bit = data_bits == 9 && i == 0;
    const bool sda_level = is_dc_bit ? dc_bit : (data_temp & 0x80);
    if (!is_dc_bit) {
      data_temp <<= 1;
    }

    if (sda_level) {
      spi->port1_shadow |= spi->sda_mask;
    } else {
      spi->port1_shadow &= ~spi->sda_mask;
    }
    spi->port1_shadow &= ~spi->scl_mask;
    if (!SpiWritePort1(spi)) {
      return false;
    }
    DelayUs(kSclHalfPeriodUs);
    spi->port1_shadow |= spi->scl_mask;
    if (!SpiWritePort1(spi)) {
      return false;
    }
    DelayUs(kSclHalfPeriodUs);
  }

  return true;
}

bool SpiWritePackage(ThreeWireSpi* spi, bool is_command, uint8_t data) {
  const int dc_bit = is_command ? 0 : 1;

  if (!SpiSetLine(spi, spi->cs_mask, false)) {
    return false;
  }
  DelayUs(kSclHalfPeriodUs);
  if (!SpiWriteByte(spi, dc_bit, data)) {
    return false;
  }
  spi->port1_shadow &= ~spi->scl_mask;
  spi->port1_shadow &= ~spi->sda_mask;
  if (!SpiWritePort1(spi)) {
    return false;
  }
  DelayUs(kSclHalfPeriodUs);
  if (!SpiSetLine(spi, spi->cs_mask, true)) {
    return false;
  }
  DelayUs(kSclHalfPeriodUs);

  return true;
}

bool SendCommand(ThreeWireSpi* spi, const InitCommand& init_command) {
  if (!SpiWritePackage(spi, true, init_command.command)) {
    return false;
  }
  for (size_t i = 0; i < init_command.data_size; i++) {
    if (!SpiWritePackage(spi, false, init_command.data[i])) {
      return false;
    }
  }
  if (init_command.delay_ms > 0) {
    vTaskDelay(pdMS_TO_TICKS(init_command.delay_ms));
  }

  return true;
}

cpp_bus_driver::Pwm& Backlight() {
  static cpp_bus_driver::Pwm backlight(t_panel::gpio::st7701::kBacklight);
  return backlight;
}

void ResetLcd(cpp_bus_driver::Xl95x5* xl9535) {
  Xl9535Write(xl9535, t_panel::gpio::xl95x5::kLcdRst, 1);
  vTaskDelay(pdMS_TO_TICKS(10));
  Xl9535Write(xl9535, t_panel::gpio::xl95x5::kLcdRst, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  Xl9535Write(xl9535, t_panel::gpio::xl95x5::kLcdRst, 1);
  vTaskDelay(pdMS_TO_TICKS(120));
}

bool ConfigureSt7701Pins(cpp_bus_driver::Xl95x5* xl9535) {
  if (!xl9535->SetGpioMode(Xl95x5Pin::kIoPort1, Xl95x5Mode::kOutput)) {
    printf("Set XL9535 ST7701 SPI port mode failed\n");
    return false;
  }
  if (!xl9535->GpioWrite(Xl95x5Pin::kIoPort1, 0xFF)) {
    printf("Set XL9535 ST7701 SPI port level failed\n");
    return false;
  }
  if (!xl9535->SetGpioMode(
          t_panel::gpio::xl95x5::kLcdRst, Xl95x5Mode::kOutput)) {
    printf("Set XL9535 LCD reset mode failed\n");
    return false;
  }
  if (!xl9535->GpioWrite(t_panel::gpio::xl95x5::kLcdRst, 1)) {
    printf("Set XL9535 LCD reset level failed\n");
    return false;
  }

  return true;
}

bool InitSt7701BySpi(cpp_bus_driver::Xl95x5* xl9535) {
  ThreeWireSpi spi = {
      .xl9535 = xl9535,
      .port1_shadow = 0xFF,
      .cs_mask = Port1Mask(t_panel::gpio::xl95x5::kSt7701SpiCs),
      .scl_mask = Port1Mask(t_panel::gpio::xl95x5::kSt7701SpiSclk),
      .sda_mask = Port1Mask(t_panel::gpio::xl95x5::kSt7701SpiMosi),
  };

  spi.port1_shadow |= spi.cs_mask;
  spi.port1_shadow &= ~spi.scl_mask;
  spi.port1_shadow &= ~spi.sda_mask;
  if (!SpiWritePort1(&spi)) {
    printf("Set ST7701 SPI idle level failed\n");
    return false;
  }

  for (const auto& init_command : kInitSequence) {
    if (!SendCommand(&spi, init_command)) {
      printf("Send ST7701 init command 0x%02X failed\n", init_command.command);
      return false;
    }
  }

  return true;
}

bool InitRgbPanel(esp_lcd_panel_handle_t* panel_handle) {
  esp_lcd_rgb_panel_config_t rgb_config = {
      .clk_src = LCD_CLK_SRC_DEFAULT,
      .timings =
          {
              .pclk_hz = t_panel::device::st7701::kPixelClockHz,
              .h_res = t_panel::device::st7701::kWidth,
              .v_res = t_panel::device::st7701::kHeight,
              .hsync_pulse_width = t_panel::device::st7701::kHsyncPulseWidth,
              .hsync_back_porch = t_panel::device::st7701::kHsyncBackPorch,
              .hsync_front_porch = t_panel::device::st7701::kHsyncFrontPorch,
              .vsync_pulse_width = t_panel::device::st7701::kVsyncPulseWidth,
              .vsync_back_porch = t_panel::device::st7701::kVsyncBackPorch,
              .vsync_front_porch = t_panel::device::st7701::kVsyncFrontPorch,
              .flags =
                  {
                      .hsync_idle_low = 0,
                      .vsync_idle_low = 0,
                      .de_idle_high = 0,
                      .pclk_active_neg =
                          t_panel::device::st7701::kPclkActiveNeg,
                      .pclk_idle_high = 0,
                  },
          },
      .data_width = 16,
      .bits_per_pixel = 16,
      .num_fbs = t_panel::device::st7701::kFrameBufferCount,
      .bounce_buffer_size_px = t_panel::device::st7701::kWidth *
                               t_panel::device::st7701::kBounceBufferHeight,
      .sram_trans_align = 8,
      .psram_trans_align = 64,
      .hsync_gpio_num = t_panel::gpio::st7701::kHsync,
      .vsync_gpio_num = t_panel::gpio::st7701::kVsync,
      .de_gpio_num = GPIO_NUM_NC,
      .pclk_gpio_num = t_panel::gpio::st7701::kPclk,
      .disp_gpio_num = GPIO_NUM_NC,
      .data_gpio_nums =
          {
              t_panel::gpio::st7701::kB0,
              t_panel::gpio::st7701::kB1,
              t_panel::gpio::st7701::kB2,
              t_panel::gpio::st7701::kB3,
              t_panel::gpio::st7701::kB4,
              t_panel::gpio::st7701::kG0,
              t_panel::gpio::st7701::kG1,
              t_panel::gpio::st7701::kG2,
              t_panel::gpio::st7701::kG3,
              t_panel::gpio::st7701::kG4,
              t_panel::gpio::st7701::kG5,
              t_panel::gpio::st7701::kR0,
              t_panel::gpio::st7701::kR1,
              t_panel::gpio::st7701::kR2,
              t_panel::gpio::st7701::kR3,
              t_panel::gpio::st7701::kR4,
          },
      .flags =
          {
              .fb_in_psram = 1,
          },
  };

  if (esp_lcd_new_rgb_panel(&rgb_config, panel_handle) != ESP_OK) {
    printf("New RGB panel failed\n");
    return false;
  }
  if (esp_lcd_panel_reset(*panel_handle) != ESP_OK) {
    printf("Reset RGB panel failed\n");
    return false;
  }
  if (esp_lcd_panel_init(*panel_handle) != ESP_OK) {
    printf("Init RGB panel failed\n");
    return false;
  }
  return true;
}

}  // namespace

bool InitSt7701(
    cpp_bus_driver::Xl95x5* xl9535, esp_lcd_panel_handle_t* panel_handle) {
  if (xl9535 == nullptr || panel_handle == nullptr) {
    printf("Invalid ST7701 init argument\n");
    return false;
  }

  if (!ConfigureSt7701Pins(xl9535)) {
    return false;
  }
  ResetLcd(xl9535);
  if (!InitSt7701BySpi(xl9535)) {
    return false;
  }
  if (!InitRgbPanel(panel_handle)) {
    return false;
  }
  return true;
}

bool InitBacklight() {
  if (!Backlight().Init(
          ledc_timer_t::LEDC_TIMER_0, ledc_channel_t::LEDC_CHANNEL_0, 2000)) {
    printf("Init ST7701 backlight failed\n");
    return false;
  }

  return true;
}

bool SetBacklight(uint8_t duty) {
  if (!Backlight().SetDuty(duty)) {
    printf("Set ST7701 backlight failed\n");
    return false;
  }

  return true;
}

bool StartBacklightGradient(uint8_t target_duty, int32_t time_ms) {
  if (!Backlight().StartGradientTime(target_duty, time_ms)) {
    printf("Start ST7701 backlight gradient failed\n");
    return false;
  }

  return true;
}

}  // namespace st7701_driver
