/*
 * @Description: XL95x5 series drivers
 * @version: V1.0.0
 * @Author: LILYGO
 * @Date: 2023-06-30 10:50:38
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-09-16 10:32:20
 * @License: GPL 3.0
 */
#pragma once

#include <Wire.h>

// Default configuration
#define XL95x5_IIC_ADDRESS 0X20
#define XL95x5_SDA 17
#define XL95x5_SCL 18
#define XL95x5_INTERRUPTPIN 2

// XL95x5 register address configuration
enum
{
    XL95x5_INPUT_PORT_0_REG = 0X00,
    XL95x5_INPUT_PORT_1_REG,
    XL95x5_OUTPUT_PORT_0_REG,
    XL95x5_OUTPUT_PORT_1_REG,
    XL95x5_INVERSION_PORT_0_REG,
    XL95x5_INVERSION_PORT_1_REG,
    XL95x5_CONFIG_PORT_0_REG,
    XL95x5_CONFIG_PORT_1_REG,
};

// GPIO port configuration of XL95x5
enum
{
    XL95x5_PORT_0 = 0,
    XL95x5_PORT_1,
    XL95x5_GPIO_NUM_0 = 0,
    XL95x5_GPIO_NUM_1,
    XL95x5_GPIO_NUM_2,
    XL95x5_GPIO_NUM_3,
    XL95x5_GPIO_NUM_4,
    XL95x5_GPIO_NUM_5,
    XL95x5_GPIO_NUM_6,
    XL95x5_GPIO_NUM_7,
    XL95x5_GPIO_NUM_8 = 10,
    XL95x5_GPIO_NUM_9,
    XL95x5_GPIO_NUM_10,
    XL95x5_GPIO_NUM_11,
    XL95x5_GPIO_NUM_12,
    XL95x5_GPIO_NUM_13,
    XL95x5_GPIO_NUM_14,
    XL95x5_GPIO_NUM_15,
};

class XL95x5
{
public:
    XL95x5(uint8_t address, int sda, int scl);
    XL95x5(uint8_t address, int sda, int scl, uint8_t interruptPin, void (*interruptFunction)());
    ~XL95x5(){};

    void begin(bool A0 = 0, bool A1 = 0, bool A2 = 0);
    void pinMode(uint8_t pin, uint8_t mode);
    void portMode(uint8_t port, uint8_t mode);

    void digitalWrite(uint8_t pin, uint8_t val);
    int32_t digitalRead(uint8_t pin);
    void read_all_reg();

protected:
    void writeRegister(uint8_t reg, uint8_t *data, uint8_t len);
    uint8_t readRegister(uint8_t reg, uint8_t *data, uint8_t len);

    TwoWire *_wire;
    uint8_t _address = XL95x5_IIC_ADDRESS;
    int _sda = XL95x5_SDA;
    int _scl = XL95x5_SCL;
    bool _found_flag = false;
    bool _Interrupt_flag = false;
    uint8_t _interruptPin = 2;
    void (*_interruptFunction)(){};
};