/*
 * @Description: None
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-08-16 14:24:03
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-02-29 18:04:39
 * @License: GPL 3.0
 */
#pragma once

// SD
#define SD_CS 38
#define SD_SCLK 36
#define SD_MOSI 35
#define SD_MISO 37

// IIC
#define IIC_SDA 17
#define IIC_SCL 18

// ESP32H2
#define ESP32H2_TX 48
#define ESP32H2_RX 47

// 选择你的板子版本以及通信模块
//  #define T_Panel_V1_0_RS485
 #define T_Panel_V1_2_RS485
// #define T_Panel_V1_2_CAN

#if defined T_Panel_V1_0_RS485
#define RS485_TX 15
#define RS485_RX 16
#endif

#if defined T_Panel_V1_2_RS485
#define RS485_TX 16
#define RS485_RX 15
#endif

#if defined T_Panel_V1_2_CAN
#define CAN_TX 16
#define CAN_RX 15
#endif

// YDP395BT001-V2
#define LCD_WIDTH 480
#define LCD_HEIGHT 480
#define LCD_VSYNC 40
#define LCD_HSYNC 39
#define LCD_PCLK 41
#define LCD_B0 1
#define LCD_B1 2
#define LCD_B2 3
#define LCD_B3 4
#define LCD_B4 5
#define LCD_G0 6
#define LCD_G1 7
#define LCD_G2 8
#define LCD_G3 9
#define LCD_G4 10
#define LCD_G5 11
#define LCD_R0 12
#define LCD_R1 13
#define LCD_R2 42
#define LCD_R3 46
#define LCD_R4 45
#define LCD_BL 14

// CST3240
#define CST3240_ADDRESS 0x5A
#define TOUCH_SDA 17
#define TOUCH_SCL 18
#define TOUCH_INT 21
#define TOUCH_RST 4

// XL95x5
#define XL95X5_CS 17
#define XL95X5_SCLK 15
#define XL95X5_MOSI 16
#define XL95X5_TOUCH_RST 4
#define XL95X5_RS485_CON 7
#define XL95X5_LCD_RST 5
#define XL95X5_ESP32H2_IO12 1
#define XL95X5_ESP32H2_IO4 2
#define XL95X5_ESP32H2_IO5 3

// ESP32H2
#define ESP32H2_EN 34
#define ESP32H2_BOOT 33
