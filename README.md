<h1 align="center">T-Panel</h1>

## **English** | [Chinese](./README_CN.md)

<!-- [![Release](https://img.shields.io/github/v/release/Xinyuan-LilyGO/T-Panel?style=flat-square)](https://github.com/Xinyuan-LilyGO/T-Panel/releases) -->
[![License](https://img.shields.io/github/license/Xinyuan-LilyGO/T-Panel?style=flat-square)](./LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.4%2B-ff6f00?style=flat-square)](https://github.com/espressif/esp-idf)
[![C++](https://img.shields.io/badge/C%2B%2B-17%2B-00599c?style=flat-square)](https://isocpp.org/)

<p align="center">
  <img src="image/13.jpg" alt="T-Panel product photo" width="720">
</p>

## Overview

T-Panel is a compact smart-home control panel with an **ESP32-S3** main controller and a **480 x 480 RGB touch display**. It integrates local UI rendering, SD card storage, long-distance field-bus communication, and an **ESP32-H2** wireless coprocessor interface.

## Directory

- [Overview](#overview)
- [Hardware Versions](#hardware-versions)
- [Preview](#preview)
- [Supported Frameworks](#supported-frameworks)
- [Quick Start](#quick-start)
- [Hardware Modules](#hardware-modules)
- [Pin Overview](#pin-overview)
- [Project Materials](#project-materials)
- [FAQ](#faq)

## Hardware Versions

| Version | Date | Description |
| :---: | :---: | --- |
| T-Panel V1.0-V1.1 | 2023-11-23 | Initial hardware version |
| T-Panel V1.2 | 2023-12-11 | RS485 module changed to replaceable RS485/CAN module |
| T-Panel V1.3 | 2024-05-08 | Structure update |

## Preview

<p align="center">
  <img src="image/12.jpg" alt="T-Panel front view" width="32%">
  <img src="image/13.jpg" alt="T-Panel side view" width="32%">
  <img src="image/14.jpg" alt="T-Panel back view" width="32%">
</p>

## Supported Frameworks

| Framework | Status | Version |
| --- | --- | --- |
| ESP-IDF | Recommended | `>= v5.5.4` |

## Quick Start

### Build With ESP-IDF

Install ESP-IDF first. For environment setup, refer to the official guide:
[ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html)

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash monitor
```

Select one of the following examples in `menuconfig`, then rebuild the project.

```text
Example Configuration
`-- Select the example to build
```

| Example | Description |
| --- | --- |
| [`screen`](./main/examples/screen) | Basic RGB LCD bring-up example |
| [`screen_lvgl`](./main/examples/screen_lvgl) | LVGL 9.5 display startup example |
| [`screen_lvgl_touch_draw`](./main/examples/screen_lvgl_touch_draw) | LVGL touch drawing example |
| [`sd`](./main/examples/sd) | SD card mount and file-system test |
| [`rs485_can`](./main/examples/rs485_can) | Field-bus transfer test example |
| [`general_test`](./main/examples/general_test) | Integrated factory test UI |

The following firmware is prebuilt.

To flash prebuilt firmware, refer to Espressif's official [ESP firmware online flashing platform guide](https://docs.espressif.com/projects/esp-techpedia/zh_CN/latest/esp-friends/get-started/try-firmware/try-firmware-platform.html).

| Firmware | Flash Address | Description |
| --- | --- | --- |
| [`general_test`](<./firmware/[t-panel_v1.2][general_test]>) | `0x0` | T-Panel V1.2 `general_test` factory test firmware |

## Hardware Modules

### 1. MCU

- MCU1: ESP32-S3
- PSRAM: 8 MB
- Flash: 16 MB
- Related documents:
  - [Espressif technical documents](https://www.espressif.com/en/support/documents/technical-documents)

- MCU2: ESP32-H2-MINI-1
- Flash: 4 MB
- Related documents:
  - [ESP32-H2-MINI-1 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-h2-mini-1_mini-1u_datasheet_en.pdf)

### 2. Display

- Model: YDP395BT001
- Size: 3.95 inch
- Resolution: 480 x 480
- Type: IPS
- Driver IC: ST7701S
- Bus: SPI initialization + RGB data bus
- Related documents:
  - [YDP395BT001-V2](./docs/YDP395BT001-V2.pdf)
  - [ST7701S](./docs/ST7701S_SPEC_V1.4.pdf)

### 3. Touch

- Chip: CST3240
- Bus: I2C
- Related documents:
  - [HYNITRON-CST3240_V1.0](./docs/HYNITRON-CST3240_V1.0.pdf)

### 4. IO Expansion

- Chip: XL9535
- Bus: I2C
- Usage: LCD reset/control, SPI control and auxiliary GPIO
- Related documents:
  - [XL9535](./docs/XL9535.pdf)

### 5. SD Card Slot

- Bus: SPI
- File system: FATFS

### 6. RS485 / CAN

- RS485 bus: UART
- CAN bus: TWAI
- Related documents:
  - [EBYTE-RSM485MT5V](./docs/EBYTE-RSM485MT5V.pdf)
  - [TD501MCANFD_MORNSUN](./docs/TD501MCANFD_MORNSUN.pdf)

> [!IMPORTANT]
> T-Panel V1.0-V1.1 ships with RS485 by default; T-Panel V1.2-V1.3 uses a replaceable RS485/CAN module.

## Pin Overview

Pin definitions are provided in [`t_panel_config.h`](./libraries/private_library/t_panel_config.h).

## Project Materials

| Document | Description |
| --- | --- |
| [`T-Panel_V1.2.pdf`](./project/T-Panel_V1.2.pdf) | Hardware project PDF |
| [`structure`](./structure) | Enclosure structure materials |

## FAQ

<details>
<summary>Q. Why does my board continuously fail to flash?</summary>

A. Please hold down the `BOOT` button and try downloading the program again.

</details>
