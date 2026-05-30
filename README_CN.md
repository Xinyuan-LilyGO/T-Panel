<h1 align="center">T-Panel</h1>

## [英文](./README.md) | **中文**

<!-- [![Release](https://img.shields.io/github/v/release/Xinyuan-LilyGO/T-Panel?style=flat-square)](https://github.com/Xinyuan-LilyGO/T-Panel/releases) -->
[![License](https://img.shields.io/github/license/Xinyuan-LilyGO/T-Panel?style=flat-square)](./LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.3%2B-ff6f00?style=flat-square)](https://github.com/espressif/esp-idf)
[![C++](https://img.shields.io/badge/C%2B%2B-17%2B-00599c?style=flat-square)](https://isocpp.org/)

<p align="center">
  <img src="image/13.jpg" alt="T-Panel 产品图" width="720">
</p>

## 概述

T-Panel 是一款紧凑型智能家居控制面板，主控采用 **ESP32-S3**，搭载 **480 x 480 RGB 触摸屏**。产品集成本地 UI 渲染、SD 卡存储、远距离现场总线通信，以及 **ESP32-H2** 无线协处理器接口。

## 目录

- [概述](#概述)
- [硬件版本](#硬件版本)
- [预览](#预览)
- [支持框架](#支持框架)
- [快速开始](#快速开始)
- [硬件模块](#硬件模块)
- [引脚总览](#引脚总览)
- [项目资料](#项目资料)
- [常见问题](#常见问题)

## 硬件版本

| 版本 | 日期 | 说明 |
| :---: | :---: | --- |
| T-Panel V1.0-V1.1 | 2023-11-23 | 初始硬件版本 |
| T-Panel V1.2 | 2023-12-11 | RS485 模块改为可替换 RS485/CAN 模块 |
| T-Panel V1.3 | 2024-05-08 | 结构更新 |

## 预览

<p align="center">
  <img src="image/12.jpg" alt="T-Panel 正面图" width="32%">
  <img src="image/13.jpg" alt="T-Panel 侧面图" width="32%">
  <img src="image/14.jpg" alt="T-Panel 背面图" width="32%">
</p>

## 支持框架

| 框架 | 状态 | 版本 |
| --- | --- | --- |
| ESP-IDF | 推荐 | `>= v5.5.4` |

## 快速开始

### 使用 ESP-IDF 构建

请先安装 ESP-IDF。环境安装与配置可以参考官方说明：
[ESP-IDF 入门指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/index.html)

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash monitor
```

在 `menuconfig` 中选择下面的示例程序，然后重新构建工程。

```text
Example Configuration
`-- Select the example to build
```

| 示例 | 说明 |
| --- | --- |
| [`screen`](./main/examples/screen) | 基础 RGB LCD 点屏示例 |
| [`screen_lvgl`](./main/examples/screen_lvgl) | LVGL 9.5 显示启动示例 |
| [`screen_lvgl_touch_draw`](./main/examples/screen_lvgl_touch_draw) | LVGL 触摸绘图示例 |
| [`sd`](./main/examples/sd) | SD 卡挂载和文件系统测试 |
| [`rs485_can`](./main/examples/rs485_can) | 现场总线传输测试示例 |
| [`general_test`](./main/examples/general_test) | 综合出厂测试 UI |

以下是已经编译好的固件。

烧录预编译固件时，可参考乐鑫官方 [ESP 固件在线烧录平台说明](https://docs.espressif.com/projects/esp-techpedia/zh_CN/latest/esp-friends/get-started/try-firmware/try-firmware-platform.html)。

| 固件 | 烧录地址 | 说明 |
| --- | --- | --- |
| [`general_test`](<./firmware/[t-panel_v1.2][general_test]>) | `0x0` | T-Panel V1.2 `general_test` 出厂测试固件 |

## 硬件模块

### 1. MCU

- MCU1: ESP32-S3
- PSRAM: 8 MB
- Flash: 16 MB
- 相关资料：
  - [Espressif 技术文档](https://www.espressif.com/zh-hans/support/documents/technical-documents)

- MCU2: ESP32-H2-MINI-1
- Flash: 4 MB
- 相关资料：
  - [ESP32-H2-MINI-1 数据手册](https://www.espressif.com/sites/default/files/documentation/esp32-h2-mini-1_mini-1u_datasheet_cn.pdf)

### 2. 显示屏

- 型号：YDP395BT001
- 尺寸：3.95 英寸
- 分辨率：480 x 480
- 类型：IPS
- 驱动 IC：ST7701S
- 总线：SPI 初始化 + RGB 数据总线
- 相关资料：
  - [YDP395BT001-V2](./docs/YDP395BT001-V2.pdf)
  - [ST7701S](./docs/ST7701S_SPEC_V1.4.pdf)

### 3. 触摸

- 芯片：CST3240
- 总线：I2C
- 相关资料：
  - [HYNITRON-CST3240_V1.0](./docs/HYNITRON-CST3240_V1.0.pdf)

### 4. IO 扩展

- 芯片：XL9535
- 总线：I2C
- 用途：LCD 复位/控制、SPI 控制和辅助 GPIO
- 相关资料：
  - [XL9535](./docs/XL9535.pdf)

### 5. SD 卡槽

- 总线：SPI
- 文件系统：FATFS

### 6. RS485 / CAN

- RS485 总线：UART
- CAN 总线：TWAI
- 相关资料：
  - [EBYTE-RSM485MT5V](./docs/EBYTE-RSM485MT5V.pdf)
  - [TD501MCANFD_MORNSUN](./docs/TD501MCANFD_MORNSUN.pdf)

> [!IMPORTANT]
> T-Panel V1.0-V1.1 默认搭载 RS485；T-Panel V1.2-V1.3 使用可替换 RS485/CAN 模块。

## 引脚总览

引脚定义请参考配置文件：[`t_panel_config.h`](./libraries/private_library/t_panel_config.h)

## 项目资料

| 文档 | 说明 |
| --- | --- |
| [`T-Panel_V1.2.pdf`](./project/T-Panel_V1.2.pdf) | 硬件项目 PDF |
| [`structure`](./structure) | 外壳结构资料 |

## 常见问题

<details>
<summary>Q. 为什么我的板子一直烧录失败？</summary>

A. 请按住 `BOOT` 按键，然后重新下载程序。

</details>
