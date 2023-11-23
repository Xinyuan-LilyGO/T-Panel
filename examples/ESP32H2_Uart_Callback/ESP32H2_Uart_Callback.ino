/*
 * @Description: ESP32H2芯片串口回调函数
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-11-23 11:25:29
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-11-23 15:16:41
 * @License: GPL 3.0
 */
#include <Arduino.h>

char DATA1[100] = {0};

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    if (Serial.available() > 0)
    {
        Serial.read(DATA1, sizeof(DATA1));
        Serial.printf("%s", DATA1);
    }
}