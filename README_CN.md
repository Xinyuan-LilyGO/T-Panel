<!--
 * @Description: None
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-09-11 16:13:14
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-11-24 15:08:32
 * @License: GPL 3.0
-->
<h1 align = "center">T-Panel</h1>

## **[English](./README.md) | 中文**

## 版本迭代:
| Version                               | Update date                       |
| :-------------------------------: | :-------------------------------: |
| T-Panel_V1.0                      | 2023-11-23                         |

## 购买链接

| Product                     | SOC           |  FLASH  |  PSRAM   | Link                   |
| :------------------------: | :-----------: |:-------: | :---------: | :------------------: |
| T-Panel_V1.0   | ESP32S3 |   -M   | -M  |  [暂未售卖]()  |

## 目录
- [描述](#描述)
- [预览](#预览)
- [模块](#模块)
- [快速开始](#快速开始)
- [引脚总览](#引脚总览)
- [常见问题](#常见问题)
- [项目](#项目)
- [资料](#资料)
- [依赖库](#依赖库)

## 描述

T-Panel是一款拥有480x480RGB像素大屏的智能家居产品。板载高性能ESP32S3芯片控制屏幕，可与ESP32H2相互通信交换数据。产品还搭载RS485芯片可实现对其他设备进行远距离传输，宽电压输入使得产品兼容性更高。

## 预览

### PCB板

### 渲染图


## 模块

### 1. 控制芯片

* 主芯片：ESP32-S3
* PSRAM：-M 
* FLASH：-M
* 其他说明：更多资料请访问[乐鑫官方ESP32­-S3 数据手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_cn.pdf)

* 副芯片：ESP32-H2-MINI-1
* FLASH：4MB(4Mb)
* 其他说明：更多资料请访问[乐鑫官方ESP32­-MINI-1数据手册](https://www.espressif.com/sites/default/files/documentation/esp32-h2-mini-1_mini-1u_datasheet_cn.pdf)


### 2. 480x480RGB像素显示屏幕

* 驱动：ST7701S
* 兼容库：Arduino_GFX、lvgl
* 使用总线通信协议：SPI+RGB
* 其他说明：使用XL95x5 IO扩展芯片进行SPI传输初始化屏幕后使用RGB协议进行屏幕色彩绘制

### 3. SD卡槽

### 4. IO扩展模块

* 芯片：XL9535
* 使用总线通信协议：IIC通信
* 其他说明：主要用于初始化RGB屏幕

### 5. RS485

* 芯片：RS485 
* 使用总线通信协议：uart通信
* 其他说明：主要用于初始化RGB屏幕

## 快速开始

### 注意事项：目前ESP32H2只能使用Arduino IDE进行编程

### PlatformIO
1. 安装[VisualStudioCode](https://code.visualstudio.com/Download)，根据你的系统类型选择安装。

2. 打开VisualStudioCode软件侧边栏的“扩展”（或者使用<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>X</kbd>打开扩展），搜索“PlatformIO IDE”扩展并下载。

3. 在安装扩展的期间，你可以前往GitHub下载程序，你可以通过点击带绿色字样的“<> Code”下载主分支程序，也通过侧边栏下载“Releases”版本程序。

4. 扩展安装完成后，打开侧边栏的资源管理器（或者使用<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>E</kbd>打开），点击“打开文件夹”，找到刚刚你下载的项目代码（整个文件夹），点击“添加”，此时项目文件就添加到你的工作区了。

5. 打开项目文件中的“platformio.ini”（添加文件夹成功后PlatformIO会自动打开对应文件夹的“platformio.ini”）,在“[platformio]”目录下取消注释选择你需要烧录的示例程序（以“default_envs = xxx”为标头），然后点击左下角的“<kbd>[√](image/4.png)</kbd>”进行编译，如果编译无误，将单片机连接电脑，点击左下角“<kbd>[→](image/5.png)</kbd>”即可进行烧录。

### Arduino
1. 安装[Arduino](https://www.arduino.cc/en/software)，根据你的系统类型选择安装。

2. 打开项目文件夹的“example”目录，选择示例项目文件夹，打开以“.ino”结尾的文件即可打开Arduino IDE项目工作区。

3. 打开右上角“工具”菜单栏->选择“开发板”->“开发板管理器”，找到或者搜索“esp32”，下载作者名为“Espressif Systems”的开发板文件。接着返回“开发板”菜单栏，选择“ESP32 Arduino”开发板下的开发板类型，选择的开发板类型由“platformio.ini”文件中以[env]目录下的“board = xxx”标头为准，如果没有对应的开发板，则需要自己手动添加项目文件夹下“board”目录下的开发板。

4. 打开菜单栏“[文件](image/6.png)”->“[首选项](image/6.png)”，找到“[项目文件夹位置](image/7.png)”这一栏，将项目目录下的“libraries”文件夹里的所有库文件连带文件夹复制粘贴到这个目录下的“libraries”里边。

5. 在 "工具 "菜单中选择正确的设置，如下表所示。

#### ESP32-S3
| Setting                               | Value                                 |
| :-------------------------------: | :-------------------------------: |
| Upload Speed                     | 921600                               |
| USB Mode                           | Hardware CDC and JTAG     |
| USB CDC On Boot                | Enabled                             |
| USB Firmware MSC On Boot | Disabled                             |
| USB DFU On Boot                | Disabled                             |
| CPU Frequency                   | 240MHz (WiFi)                    |
| Flash Mode                         | QIO 80MHz                         |
| Flash Size                           | 16MB (128Mb)                     |
| Core Debug Level                | None                                 |
| PSRAM                                | QSPI PSRAM                         |
| Arduino Runs On                  | Core 1                               |
| Events Run On                     | Core 1                               |

#### ESP32-H2
| Setting                               | Value                                 |
| :-------------------------------: | :-------------------------------: |
| Upload Speed                     | 921600                               |
| USB CDC On Boot                | Disabled                             |
| CPU Frequency                   | 64MHz                              |
| Flash Mode                         | QIO                                   |
| Flash Size                           | 4MB (32Mb)                     |
| Core Debug Level                | None                                 |

6. 选择正确的端口。

7. 点击右上角“<kbd>[√](image/8.png)</kbd>”进行编译，如果编译无误，将单片机连接电脑，点击右上角“<kbd>[→](image/9.png)</kbd>”即可进行烧录。

### firmware烧录
1. 打开项目文件“tools”找到ESP32烧录工具，打开。

2. 选择正确的烧录芯片以及烧录方式点击“OK”，如图所示根据步骤1->2->3->4->5即可烧录程序，如果烧录不成功，请按住“BOOT-0”键再下载烧录。

3. 烧录文件在项目文件根目录“[firmware](./firmware/)”文件下，里面有对firmware文件版本的说明，选择合适的版本下载即可。

<p align="center" width="100%">
    <img src="image/10.png" alt="example">
    <img src="image/11.png" alt="example">
</p>


## 引脚总览

| 显示屏引脚       | ESP32S3引脚|XL95X5引脚      |
| :------------------: | :------------------:| :------------------:|
| VSYNC              | ESP32S3_IO40                  |       NULL         |
| HSYNC              | ESP32S3_IO39                  |       NULL         |
| PCLK                | ESP32S3_IO41                  |       NULL         |
| B0                    | ESP32S3_IO1                   |       NULL         |
| B1                    | ESP32S3_IO2                  |       NULL         |
| B2                    | ESP32S3_IO3                  |       NULL         |
| B3                    | ESP32S3_IO4                  |       NULL         |
| B4                    | ESP32S3_IO5                  |       NULL         |
| G0                    | ESP32S3_IO6                  |       NULL         |
| G1                    | ESP32S3_IO7                  |       NULL         |
| G2                    | ESP32S3_IO8                  |       NULL         |
| G3                    | ESP32S3_IO9                  |       NULL         |
| G4                    | ESP32S3_IO10                 |       NULL         |
| G5                    | ESP32S3_IO11                 |       NULL         |
| R0                    | ESP32S3_IO12                 |       NULL         |
| R1                    | ESP32S3_IO13                 |       NULL         |
| R2                    | ESP32S3_IO42                 |       NULL         |
| R3                    | ESP32S3_IO46                 |       NULL         |
| R4                    |ESP32S3_ IO45                 |       NULL         |
| BL                    | ESP32S3_IO14                 |       NULL         |
| CS                    |        NULL         |  XL95X5_IO17                 |
| SCLK                |         NULL         | XL95X5_IO15                  | 
| MOSI                |         NULL         | XL95X5_IO16                  | 
| LCD_RST              |         NULL          |       XL95X5_IO5         |

| 显示屏触摸引脚       | ESP32S3引脚|XL95X5引脚      |
| :------------------: | :------------------:| :------------------:|
| TOUCH_INT         | ESP32S3_IO21                 |       NULL         |
| TOUCH_RST         |         NULL         |       XL95X5_IO4         |

| SD卡引脚           | ESP32S3引脚      |
| :------------------: | :------------------:|
| CS                     | IO38                  |
| SCLK                  | IO36                    |
| MOSI                  | IO35                  |
| MISO                  | IO37                    |

| RS485引脚           | ESP32S3引脚      | XL95X5引脚      |
| :------------------: | :------------------:| :------------------:|
| TX                     | ESP32S3_IO15                  |NULL           |
| RX                     | ESP32S3_IO16                  |NULL           |
| CON                  |          NULL           | XL95X5_IO7        |

| 功能     | ESP32H2引脚     | ESP32S3引脚      | XL95X5引脚      |
| :-------: | :----------: |  :------------------:|:------------------:|
| 启动ESP32H2的BOOT模式  | ESP32H2_IO9     | ESP32S3_IO33      |        NULL         |
| 复位ESP32H2              | ESP32H2_引脚序号8      | ESP32S3_IO34     |        NULL         |
| Uart  | ESP32H2_TX_IO24     | ESP32S3_RX_IO48     |        NULL         |
| Uart  | ESP32H2_RX_IO23        | ESP32S3_TX_IO47   |        NULL         |
| IO  | ESP32H2_IO12                   |      NULL         |        XL95X5_IO1         |
| IO  | ESP32H2_IO4                     |       NULL        |        XL95X5_IO2         |
| IO  | ESP32H2_IO5                     |        NULL       |        XL95X5_IO3         |

## 常见问题

* Q. 看了以上教程我还是不会搭建编程环境怎么办？
* A. 如果看了以上教程还不懂如何搭建环境的可以参考[LilyGo-Document](https://github.com/Xinyuan-LilyGO/LilyGo-Document)文档说明来搭建。

<br />

* Q. 为什么打开Arduino IDE时他会提醒我是否要升级库文件？我应该升级还是不升级？
* A. 选择不升级库文件，不同版本的库文件可能不会相互兼容所以不建议升级库文件。

<br />

* Q. 为什么我的板子上“Uart”接口没有输出串口数据，是不是坏了用不了啊？
* A. 因为项目文件默认配置将USB接口作为Uart0串口输出作为调试，“Uart”接口连接的是Uart0，不经配置自然是不会输出任何数据的。<br />PlatformIO用户请打开项目文件“platformio.ini”，将“build_flags = xxx”下的选项“-DARDUINO_USB_CDC_ON_BOOT=true”修改成“-DARDUINO_USB_CDC_ON_BOOT=false”即可正常使用外部“Uart”接口。<br />Arduino用户打开菜单“工具”栏，选择USB CDC On Boot: “Disabled”即可正常使用外部“Uart”接口。

<br />

* Q. 为什么我在Arduion上找不到ESP32H2这个芯片的库呢？
* A. 目前ESP32H2这款芯片的Arduino库正处于测试阶段，您可以去GitHub上的[arduino-esp32](https://github.com/espressif/arduino-esp32)下载3.0.0分支版本以上的
package_esp32_dev_index.json文件，导入Arduino IDE即可看到ESP32H2芯片库

<br />

* Q. 为什么我的板子一直烧录失败呢？
* A. 请按住“BOOT-0”按键重新下载程序。

## 项目
* [T-Panel_V1.0](./project/T-Panel_V1.0.pdf)

## 资料
* [Espressif](https://www.espressif.com/en/support/documents/technical-documents)
* [XL9535](./information/XL9535.pdf)
* [YDP395BT001-V2](./information/YDP395BT001-V2.pdf)
* [HYNITRON-CST3240_V1.0](./information/HYNITRON-CST3240_V1.0.pdf)
* [EBYTE-RSM485MT5V](./information/EBYTE-RSM485MT5V.pdf)
* [ST7701S_SPEC_V1.4](./information/ST7701S_SPEC_V1.4.pdf)

## 依赖库
* [Arduino_GFX-1.3.7](https://github.com/moononournation/Arduino_GFX)
* [TouchLib](https://github.com/mmMicky/TouchLib)
* [XL95x5_Driver]()
