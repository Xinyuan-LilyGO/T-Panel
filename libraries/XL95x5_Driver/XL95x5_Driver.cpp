/*
 * @Description: XL95x5 series drivers
 * @version: V1.0.0
 * @Author: LILYGO
 * @Date: 2023-06-30 10:50:38
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-09-16 09:29:59
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include "XL95x5_driver.h"
#include "pin_config.h"

/**
 * @brief Initialize the xl95x5 constructor
 * @param address XL95x5 address
 * @param sda XL95x5 SDA
 * @param scl XL95x5 SCL
 * @return
 * @Date 2023-07-01 11:09:26
 */
XL95x5::XL95x5(uint8_t address, int sda, int scl)
{
    _wire = &Wire;

    _address = address;
    _sda = sda;
    _scl = scl;
};

/**
 * @brief Initialize the xl95x5 constructor
 * @param address XL95x5 address
 * @param sda XL95x5 SDA
 * @param scl XL95x5 SCL
 * @param interruptPin The IO port of XL95X5 triggers an interrupt
 * @param void (*interruptFunction)() XL95x5 interrupt function
 * @return
 * @Date 2023-07-01 10:53:21
 */
XL95x5::XL95x5(uint8_t address, int sda, int scl, uint8_t interruptPin, void (*interruptFunction)())
{
    _wire = &Wire;

    _address = address;
    _sda = sda;
    _scl = scl;

    _Interrupt_flag = true;
    _interruptPin = interruptPin;
    _interruptFunction = interruptFunction;
};

void XL95x5::writeRegister(uint8_t reg, uint8_t *data, uint8_t len)
{
    _wire->beginTransmission(_address);
    _wire->write(reg);
    for (uint8_t i = 0; i < len; i++)
    {
        _wire->write(data[i]);
    }
    _wire->endTransmission();
}

uint8_t XL95x5::readRegister(uint8_t reg, uint8_t *data, uint8_t len)
{
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission();
    _wire->requestFrom(_address, len);
    uint8_t index = 0;
    while (index < len)
        data[index++] = _wire->read();
    return 0;
}

void XL95x5::begin(bool A0, bool A1, bool A2)
{
    _address = XL95x5_IIC_ADDRESS | (A2 << 3) | (A1 << 2) | (A0 << 1);
    // _wire = wire;
    _wire->begin((int)_sda, (int)_scl);
    _found_flag = true;
    _wire->beginTransmission(_address);
    if (!_wire->endTransmission())
    {
        Serial.println("Found XL95x5");

        // If using interrupt set interrupt value to pin
        if (_Interrupt_flag)
        {
            pinMode(_interruptPin, INPUT_PULLUP);
            attachInterrupt(_interruptPin, (*_interruptFunction), FALLING);
        }
    }
    else
    {
        Serial.println("XL95x5 not found");
        _found_flag = false;
    }
}

void XL95x5::pinMode(uint8_t pin, uint8_t mode)
{
    if (_found_flag)
    {
        uint8_t data = 0;
        if (pin > 7)
        {
            readRegister(XL95x5_CONFIG_PORT_1_REG, &data, 1);
            if (mode == OUTPUT)
            {
                data = data & (~(1 << (pin - 10)));
            }
            else
            {
                data = data | (1 << (pin - 10));
            }
            writeRegister(XL95x5_CONFIG_PORT_1_REG, &data, 1);
        }
        else
        {
            readRegister(XL95x5_CONFIG_PORT_0_REG, &data, 1);
            if (mode == OUTPUT)
            {
                data = data & (~(1 << pin));
            }
            else
            {
                data = data | (1 << pin);
            }
            writeRegister(XL95x5_CONFIG_PORT_0_REG, &data, 1);
        }
    }
    else
    {
        Serial.println("XL95x5 not found");
    }
}

/**
 * @brief Set the mode for the entire port
 * @param port Port tags (0 or 1)
 * @param mode Port mode (OUTPUT or INPUT)
 * @return
 * @Date 2023-07-01 11:50:56
 */
void XL95x5::portMode(uint8_t port, uint8_t mode)
{
    uint8_t data = 0;
    switch (port)
    {
    case 0:
        readRegister(XL95x5_CONFIG_PORT_0_REG, &data, 1);
        if (mode == OUTPUT)
        {
            data = 0x00;
        }
        else
        {
            data = 0xFF;
        }
        writeRegister(XL95x5_CONFIG_PORT_0_REG, &data, 1);
        break;
    case 1:
        readRegister(XL95x5_CONFIG_PORT_1_REG, &data, 1);
        if (mode == OUTPUT)
        {
            data = 0x00;
        }
        else
        {
            data = 0xFF;
        }
        writeRegister(XL95x5_CONFIG_PORT_1_REG, &data, 1);
        break;

    default:
        break;
    }
}

void XL95x5::digitalWrite(uint8_t pin, uint8_t val)
{
    if (_found_flag)
    {
        uint8_t data = 0;
        uint8_t reg_data = 0;
        if (pin > 7)
        {
            readRegister(XL95x5_OUTPUT_PORT_1_REG, &reg_data, 1);
            reg_data = reg_data & (~(1 << (pin - 10)));
            data = reg_data | val << (pin - 10);
            writeRegister(XL95x5_OUTPUT_PORT_1_REG, &data, 1);
        }
        else
        {
            readRegister(XL95x5_OUTPUT_PORT_0_REG, &reg_data, 1);
            reg_data = reg_data & (~(1 << pin));
            data = reg_data | val << pin;
            writeRegister(XL95x5_OUTPUT_PORT_0_REG, &data, 1);
        }
    }
    else
    {
        Serial.println("XL95x5 not found");
    }
}

/**
 * @brief XL95x5 Read IO Port Function (Only Available for Input Mode Pin)
 * @param pin XL95x5 IO
 * @return
 * @Date 2023-07-01 13:31:45
 */
int32_t XL95x5::digitalRead(uint8_t pin)
{
    if (_found_flag)
    {
        int state = 0;
        uint8_t data = 0;
        if (pin > 7)
        {
            readRegister(XL95x5_INPUT_PORT_1_REG, &data, 1);
            state = data >> (pin - 10) & 0x01 ? 1 : 0; // IO port displacement data judgment
        }
        else
        {
            readRegister(XL95x5_INPUT_PORT_0_REG, &data, 1);
            state = data >> pin & 0x01 ? 1 : 0; // IO port displacement data judgment
        }

        return state;
    }
    else
    {
        Serial.println("XL95x5 not found");
    }
    return 0;
}

void XL95x5::read_all_reg()
{
    uint8_t data;
    for (uint8_t i = 0; i < 8; i++)
    {
        readRegister(i, &data, 1);
        Serial.printf("0x%02x : 0x%02X \r\n", i, data);
    }
}