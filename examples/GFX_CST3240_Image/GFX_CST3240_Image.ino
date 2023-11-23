/*
 * @Description: GFX_CST3240_Image.cpp
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-09-15 15:41:30
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-11-23 15:17:22
 * @License: GPL 3.0
 */
// #define TOUCH_MODULES_GT911
// #define TOUCH_MODULES_CST_SELF
#define TOUCH_MODULES_CST_MUTUAL
// #define TOUCH_MODULES_ZTW622
// #define TOUCH_MODULES_L58
// #define TOUCH_MODULES_FT3267
// #define TOUCH_MODULES_FT5x06

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "TouchLib.h"
#include "material_16Bit.h"
#include "pin_config.h"

#define PIN_IIC_SCL 18
#define PIN_IIC_SDA 17
#define PIN_TOUCH_INT 21
#define PIN_TOUCH_RES 4

#define TOUCH_READ_FROM_INTERRNUPT 1

TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, 0x5A);

Arduino_DataBus *bus = new Arduino_XL9535SWSPI(IIC_SDA /* SDA */, IIC_SCL /* SCL */, -1 /* XL PWD */,
                                               XL95X5_CS /* XL CS */, XL95X5_SCLK /* XL SCK */, XL95X5_MOSI /* XL MOSI */);
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    -1 /* DE */, LCD_VSYNC /* VSYNC */, LCD_HSYNC /* HSYNC */, LCD_PCLK /* PCLK */,
    LCD_B0 /* B0 */, LCD_B1 /* B1 */, LCD_B2 /* B2 */, LCD_B3 /* B3 */, LCD_B4 /* B4 */,
    LCD_G0 /* G0 */, LCD_G1 /* G1 */, LCD_G2 /* G2 */, LCD_G3 /* G3 */, LCD_G4 /* G4 */, LCD_G5 /* G5 */,
    LCD_R0 /* R0 */, LCD_R1 /* R1 */, LCD_R2 /* R2 */, LCD_R3 /* R3 */, LCD_R4 /* R4 */,
    1 /* hsync_polarity */, 20 /* hsync_front_porch */, 1 /* hsync_pulse_width */, 1 /* hsync_back_porch */,
    1 /* vsync_polarity */, 30 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    10 /* pclk_active_neg */, 6000000L /* prefer_speed */, false /* useBigEndian */,
    0 /* de_idle_high*/, 0 /* pclk_idle_high */);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    LCD_WIDTH /* width */, LCD_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, -1 /* RST */, st7701_type9_init_operations, sizeof(st7701_type9_init_operations));

bool Get_int = 0;
uint8_t Image_Flag = 0;
bool Touch_Flag = 0;
uint8_t Touch_Num = 0;

uint8_t Last_X = 0;
uint8_t Last_Y = 0;

void scan_i2c_device(TwoWire &i2c)
{
    Serial.println("Scanning for I2C devices ...");
    Serial.print("      ");
    for (int i = 0; i < 0x10; i++)
    {
        Serial.printf("0x%02X|", i);
    }
    uint8_t error;
    for (int j = 0; j < 0x80; j += 0x10)
    {
        Serial.println();
        Serial.printf("0x%02X |", j);
        for (int i = 0; i < 0x10; i++)
        {
            Wire.beginTransmission(i | j);
            error = Wire.endTransmission();
            if (error == 0)
                Serial.printf("0x%02X|", i | j);
            else
                Serial.print(" -- |");
        }
    }
    Serial.println();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    pinMode(14, OUTPUT);
    digitalWrite(14, HIGH);

    pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
    digitalWrite(PIN_TOUCH_INT, HIGH);

    attachInterrupt(
        PIN_TOUCH_INT,
        []
        {
            Get_int = 1;
            Serial.println("get_int");
        },
        FALLING); // Triggered every 1ms

    Wire.begin(17 /* SDA */, 18 /* SCL */);
    // scan_i2c_device(Wire);

    gfx->begin();

    gfx->fillScreen(PINK);

    gfx->XL_digitalWrite(PIN_TOUCH_RES, 0);
    delay(200);
    gfx->XL_digitalWrite(PIN_TOUCH_RES, 1);
    delay(200);

    touch.init();
    touch.read();
    TP_Point t = touch.getPoint(0);
    Last_X = t.x;
    Last_Y = t.y;

    gfx->setCursor(240, 240);
    gfx->setTextColor(RED);
    gfx->setTextSize(2);
    gfx->printf("Ciallo");
}

void loop()
{
    if (Get_int)
    {
        Get_int = 0;
        touch.read();
        Touch_Num = touch.getPointNum();
        if (Touch_Num > 0)
        {
            Touch_Flag = 1;
            Touch_Num = 0;
        }
    }
    if (Touch_Flag == 1)
    {
        Touch_Flag = 0;
        TP_Point t = touch.getPoint(0);
        if (Last_X != t.x && Last_Y != t.y)
        {
            gfx->setCursor(t.x, t.y);

            switch (Image_Flag)
            {
            case 0:
                gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)gImage_1, 480, 480); // RGB
                Serial.printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                gfx->printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                break;
            case 1:
                gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)gImage_2, 480, 480); // RGB
                Serial.printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                gfx->printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                break;
            case 2:
                gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)gImage_3, 480, 480); // RGB
                Serial.printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                gfx->printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                break;
            case 3:
                gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)gImage_4, 480, 480); // RGB
                Serial.printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                gfx->printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                break;
            case 4:
                gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)gImage_5, 480, 480); // RGB
                Serial.printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                gfx->printf("[1] point x: %d  point y: %d \r\n", t.x, t.y);
                break;

            default:
                break;
            }
            Serial.printf("6");
            Image_Flag++;
            if (Image_Flag > 4)
            {
                Image_Flag = 0;
            }

            Last_X = t.x;
            Last_Y = t.y;
        }
    }
}
