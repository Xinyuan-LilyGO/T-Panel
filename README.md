<!--
 * @Description: None
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-09-11 16:13:14
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-11-24 15:34:20
 * @License: GPL 3.0
-->
<h1 align = "center">T-Panel</h1>

## **English | [中文](./README_CN.md)**

## Version iteration:
| Version                              | Update date                       |
| :-------------------------------: | :-------------------------------: |
| T-Panel_V1.0                      | 2023-11-23                         |

## PurchaseLink

| Product                     | SOC           |  FLASH  |  PSRAM   | Link                   |
| :------------------------: | :-----------: |:-------: | :---------: | :------------------: |
| T-Panel_V1.0   | ESP32S3 |   -M   | -M  | [Not yet sold]()  |

## Directory
- [Describe](#describe)
- [Preview](#preview)
- [Module](#module)
- [QuickStart](#quickstart)
- [PinOverview](#pinoverview)
- [FAQ](#faq)
- [Project](#project)
- [Information](#information)
- [DependentLibraries](#dependentlibraries)

## Describe

 T-Panel is an intelligent home product featuring a 480x480 RGB pixel large screen. It is controlled by the high-performance ESP32S3 chip onboard, which can communicate and exchange data with ESP32H2. The product also incorporates an RS485 chip for long-distance transmission of other devices, and its wide voltage input enhances compatibility.

## Preview

### PCB board

### Rendering

## Module

### 1.MCU Chip

* Chip: ESP32-S3
* PSRAM: -MB
* FLASH: -MB
* Others: For more information, please visit[Espressif Official ESP32-S3datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_cn.pdf)

* Chip: ESP32-H2-MINI-1
* FLASH: 4MB
* Others: For more information, please visit[Espressif Official ESP32­-MINI-1datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-h2-mini-1_mini-1u_datasheet_cn.pdf)

### 2. 480x480RGB pixel display screen

* Drive: ST7701S
* Compatibility library: Arduino_GFX、lvgl
* Using bus communication protocol: SPI+RGB
* Other instructions: Use XL95x5 IO expansion chip for SPI transmission, initialize the screen, and use RGB protocol for screen color drawing

### 3. SD card slot

### 4. IO expansion module

* Chip: XL9535
* Using bus communication protocol: IIC communication
* Other instructions: mainly used to initialize RGB screens

### 5. RS485

* Chip: RS485 
* Using bus communication protocol: uart communication
* Other instructions: mainly used to initialize RGB screens


## QuickStart

### Attention: Currently, ESP32H2 can only be programmed using the Arduino IDE

### PlatformIO
1. Install[VisualStudioCode](https://code.visualstudio.com/Download),Choose installation based on your system type.

2. Open the "Extension" section of the Visual Studio Code software sidebar(Alternatively, use "<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>X</kbd>" to open the extension),Search for the "PlatformIO IDE" extension and download it.

3. During the installation of the extension, you can go to GitHub to download the program. You can download the main branch by clicking on the "<> Code" with green text, or you can download the program versions from the "Releases" section in the sidebar.

4. After the installation of the extension is completed, open the Explorer in the sidebar(Alternatively, use "<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>E</kbd>" go open it),Click on "Open Folder," locate the project code you just downloaded (the entire folder), and click "Add." At this point, the project files will be added to your workspace.

5. Open the "platformio.ini" file in the project folder (PlatformIO will automatically open the "platformio.ini" file corresponding to the added folder). Under the "[platformio]" section, uncomment and select the example program you want to burn (it should start with "default_envs = xxx") Then click "<kbd>[√](image/4.png)</kbd>" in the bottom left corner to compile,If the compilation is correct, connect the microcontroller to the computer and click "<kbd>[→](image/5.png)</kbd>" in the bottom left corner to download the program.

### Arduino
1. Install[Arduino](https://www.arduino.cc/en/software),Choose installation based on your system type.

2. Open the "example" directory within the project folder, select the example project folder, and open the file ending with ".ino" to open the Arduino IDE project workspace.

3. Open the "Tools" menu at the top right -> Select "Board" -> "Board Manager." Find or search for "esp32" and download the board files from the author named "Espressif Systems." Then, go back to the "Board" menu and select the development board type under "ESP32 Arduino." The selected development board type should match the one specified in the "platformio.ini" file under the [env] section with the header "board = xxx." If there is no corresponding development board, you may need to manually add the development board from the "board" directory within your project folder.

4. Open menu bar "[File](image/6.png)" -> "[Preferences](image/6.png)" ,Find "[Sketchbook location](image/7.png)"  here,copy and paste all library files and folders from the "libraries" folder in the project directory into the "libraries" folder in this directory.

5. Select the correct settings in the Tools menu, as shown in the table below.

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

6. Select the correct port.

7. Click "<kbd>[√](image/8.png)</kbd>" in the upper right corner to compile,If the compilation is correct, connect the microcontroller to the computer,Click "<kbd>[→](image/9.png)</kbd>" in the upper right corner to download.

### firmware download
1. Open the project file "tools" and locate the ESP32 burning tool. Open it.

2. Select the correct burning chip and burning method, then click "OK." As shown in the picture, follow steps 1->2->3->4->5 to burn the program. If the burning is not successful, press and hold the "BOOT-0" button and then download and burn again.

3. Burn the file in the root directory of the project file "[firmware](./firmware/)" file,There is a description of the firmware file version inside, just choose the appropriate version to download.

<p align="center" width="100%">
    <img src="image/10.png" alt="example">
    <img src="image/11.png" alt="example">
</p>


## PinOverview

| Display Pin       | ESP32S3 Pin|XL95X5 Pin    |
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
| RST              |         NULL          |       XL95X5_IO5         |

| Display Screen Touch Pin       | ESP32S3 Pin|XL95X5 Pin      |
| :------------------: | :------------------:| :------------------:|
| INT         | ESP32S3_IO21                 |       NULL         |
| RST         |         NULL         |       XL95X5_IO4         |

| SD Pin           | ESP32S3 Pin      |
| :------------------: | :------------------:|
| CS                     | IO38                  |
| SCLK                  | IO36                    |
| MOSI                  | IO35                  |
| MISO                  | IO37                    |

| RS485 Pin           | ESP32S3 Pin      | XL95X5 Pin      |
| :------------------: | :------------------:| :------------------:|
| TX                     | ESP32S3_IO15                  |NULL           |
| RX                     | ESP32S3_IO16                  |NULL           |
| CON                  |          NULL           | XL95X5_IO7        |

| Function     | ESP32H2 Pin     | ESP32S3 Pin      | XL95X5 Pin      |
| :-------: | :----------: |  :------------------:|:------------------:|
| Activate The BOOT Mode Of ESP32H2  | ESP32H2_IO9     | ESP32S3_IO33      |        NULL         |
| Reset ESP32H2              | ESP32H2_Pin Number 8      | ESP32S3_IO34     |        NULL         |
| Uart  | ESP32H2_TX_IO24     | ESP32S3_RX_IO48     |        NULL         |
| Uart  | ESP32H2_RX_IO23        | ESP32S3_TX_IO47   |        NULL         |
| IO  | ESP32H2_IO12                   |      NULL         |        XL95X5_IO1         |
| IO  | ESP32H2_IO4                     |       NULL        |        XL95X5_IO2         |
| IO  | ESP32H2_IO5                     |        NULL       |        XL95X5_IO3         |

## FAQ

* Q. After reading the above tutorials, I still don't know how to build a programming environment. What should I do?
* A. If you still don't understand how to build an environment after reading the above tutorials, you can refer to the [LilyGo-Document](https://github.com/Xinyuan-LilyGO/LilyGo-Document) document instructions to build it.

<br />

* Q. Why does Arduino IDE prompt me to update library files when I open it? Should I update them or not?
* A. Choose not to update library files. Different versions of library files may not be mutually compatible, so it is not recommended to update library files.

<br />

* Q. Why is there no serial data output on the "Uart" interface on my board? Is it defective and unusable?
* A. The default project configuration uses the USB interface as Uart0 serial output for debugging purposes. The "Uart" interface is connected to Uart0, so it won't output any data without configuration.<br />For PlatformIO users, please open the project file "platformio.ini" and modify the option under "build_flags = xxx" from "-D ARDUINO_USB_CDC_ON_BOOT=true" to "-D ARDUINO_USB_CDC_ON_BOOT=false" to enable external "Uart" interface.<br />For Arduino users, open the "Tools" menu and select "USB CDC On Boot: Disabled" to enable the external "Uart" interface.

<br />

* Q. Why can't I find the ESP32H2 chip library on Arduino?
* A. The ESP32H2 chip library for Arduino is currently in the testing phase. You can download the "package_esp32_dev_index.json" file from the [arduino-esp32](https://github.com/espressif/arduino-esp32) GitHub repository for versions 3.0.0 and above, and import it into the Arduino IDE to see the ESP32H2 chip library.

<br />

* Q. Why is my board continuously failing to download the program?
* A. Please hold down the "BOOT-0" button and try downloading the program again.

## Project
* [T-Panel_V1.0](./project/T-Panel_V1.0.pdf)

## Information
* [Espressif](https://www.espressif.com/en/support/documents/technical-documents)
* [XL9535](./information/XL9535.pdf)
* [YDP395BT001-V2](./information/YDP395BT001-V2.pdf)
* [HYNITRON-CST3240_V1.0](./information/HYNITRON-CST3240_V1.0.pdf)
* [EBYTE-RSM485MT5V](./information/EBYTE-RSM485MT5V.pdf)
* [ST7701S_SPEC_V1.4](./information/ST7701S_SPEC_V1.4.pdf)

## DependentLibraries
* [Arduino_GFX-1.3.7](https://github.com/moononournation/Arduino_GFX)
* [TouchLib](https://github.com/mmMicky/TouchLib)
* [XL95x5_Driver]()