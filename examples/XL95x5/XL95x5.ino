/*
 * @Description: None
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-06-30 10:54:12
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-11-22 17:27:40
 * @License: GPL 3.0
 */
#include "Arduino.h"
#include <Wire.h>
#include "pin_config.h"
#include "XL95x5_Driver.h"

// XL95x5 initializes the configuration
XL95x5 Class_XL95x5(XL95x5_IIC_ADDRESS, XL95x5_SDA, XL95x5_SCL);

void setup()
{
    Serial.begin(115200);

    Class_XL95x5.begin();
    Class_XL95x5.read_all_reg(); // Read all registers

    Class_XL95x5.portMode(XL95x5_PORT_0, OUTPUT); // Configure the XL95x5 full port mode
    Class_XL95x5.portMode(XL95x5_PORT_1, OUTPUT);
}

void loop()
{
    Class_XL95x5.digitalWrite(XL95x5_GPIO_NUM_1, HIGH);
    delay(1000);
    Class_XL95x5.digitalWrite(XL95x5_GPIO_NUM_1, LOW);
    delay(1000);
}