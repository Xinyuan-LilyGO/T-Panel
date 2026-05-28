#pragma once

#include <cstdint>

#include "cpp_bus_driver_library.h"

namespace t_panel {
namespace gpio {
namespace sd {
inline constexpr int kCs = 38;
inline constexpr int kSclk = 36;
inline constexpr int kMosi = 35;
inline constexpr int kMiso = 37;
}  // namespace sd

namespace i2c {
inline constexpr int kSda = 17;
inline constexpr int kScl = 18;
}  // namespace i2c

namespace esp32h2 {
inline constexpr int kTx = 48;
inline constexpr int kRx = 47;
inline constexpr int kEn = 34;
inline constexpr int kBoot = 33;
}  // namespace esp32h2

namespace rs485 {
inline constexpr int kTx = 16;
inline constexpr int kRx = 15;
}  // namespace rs485

namespace can {
inline constexpr int kTx = 16;
inline constexpr int kRx = 15;
}  // namespace can

namespace st7701 {
inline constexpr int kVsync = 40;
inline constexpr int kHsync = 39;
inline constexpr int kPclk = 41;
inline constexpr int kBacklight = 14;
inline constexpr int kB0 = 1;
inline constexpr int kB1 = 2;
inline constexpr int kB2 = 3;
inline constexpr int kB3 = 4;
inline constexpr int kB4 = 5;
inline constexpr int kG0 = 6;
inline constexpr int kG1 = 7;
inline constexpr int kG2 = 8;
inline constexpr int kG3 = 9;
inline constexpr int kG4 = 10;
inline constexpr int kG5 = 11;
inline constexpr int kR0 = 12;
inline constexpr int kR1 = 13;
inline constexpr int kR2 = 42;
inline constexpr int kR3 = 46;
inline constexpr int kR4 = 45;
}  // namespace st7701

namespace cst3240 {
inline constexpr int kSda = i2c::kSda;
inline constexpr int kScl = i2c::kScl;
inline constexpr int kInt = 21;
}  // namespace cst3240

namespace xl95x5 {
inline constexpr auto kSt7701SpiCs = cpp_bus_driver::Xl95x5::Pin::kIo17;
inline constexpr auto kSt7701SpiSclk = cpp_bus_driver::Xl95x5::Pin::kIo15;
inline constexpr auto kSt7701SpiMosi = cpp_bus_driver::Xl95x5::Pin::kIo16;
inline constexpr auto kTouchRst = cpp_bus_driver::Xl95x5::Pin::kIo4;
inline constexpr auto kRs485Con = cpp_bus_driver::Xl95x5::Pin::kIo7;
inline constexpr auto kLcdRst = cpp_bus_driver::Xl95x5::Pin::kIo5;
inline constexpr auto kEsp32h2Io12 = cpp_bus_driver::Xl95x5::Pin::kIo1;
inline constexpr auto kEsp32h2Io4 = cpp_bus_driver::Xl95x5::Pin::kIo2;
inline constexpr auto kEsp32h2Io5 = cpp_bus_driver::Xl95x5::Pin::kIo3;
}  // namespace xl95x5
}  // namespace gpio

namespace device {
namespace st7701 {
inline constexpr int kWidth = 480;
inline constexpr int kHeight = 480;
inline constexpr int kPixelClockHz = 11000000;
inline constexpr int kPclkActiveNeg = 0;
inline constexpr int kBounceBufferHeight = 10;
inline constexpr int kFrameBufferCount = 1;
inline constexpr int kHsyncPulseWidth = 2;
inline constexpr int kHsyncBackPorch = 0;
inline constexpr int kHsyncFrontPorch = 20;
inline constexpr int kVsyncPulseWidth = 8;
inline constexpr int kVsyncBackPorch = 1;
inline constexpr int kVsyncFrontPorch = 30;
}  // namespace st7701

namespace xl95x5 {
inline constexpr uint8_t kI2cAddress = 0x20;
}  // namespace xl95x5

namespace cst3240 {
inline constexpr uint8_t kI2cAddress = 0x5A;
}  // namespace cst3240
}  // namespace device
}  // namespace t_panel
