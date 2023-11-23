/*
 * @Description(CN): 
 *      T-Panel板载ESP32S3与ESP32H2串口通信例程
 * 
 * @Description(EN): 
 *      T-Panel onboard ESP32S3 and ESP32H2 serial communication routine.
 * 
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-06-21 18:42:16
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-11-22 17:29:23
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include <HardwareSerial.h>
#include "pin_config.h"

HardwareSerial SerialPort(2); // use UART2
char DATA1 = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo1");

    // 初始化串口，并重新定义引脚
    // 参数包括串行通信的波特率、串行模式、使用的 RX 引脚和 TX 引脚。
    SerialPort.begin(115200, SERIAL_8N1, ESP32H2_RX, ESP32H2_TX);
}

void loop()
{
    SerialPort.println("Ciallo2");
    delay(1000);
    // reply only when you receive data:
    if (SerialPort.available() > 0)
    {
        // read the incoming byte:
        DATA1 = SerialPort.read();
        // say what you got:
        Serial.print(DATA1);
    }
}
