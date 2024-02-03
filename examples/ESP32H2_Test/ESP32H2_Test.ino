/*
 * @Description: ESP32H2_Test
 * @version: V1.0.2
 * @Author: LILYGO_L
 * @Date: 2023-11-23 11:25:29
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-01-31 14:47:57
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>

#define SERVICE_UUID "b408e1a0-3d8a-11ed-b878-0242ac120002"         // 服务UUID
#define Characteristic_UUID1 "dfd84064-3d8a-11ed-b878-0242ac120002" // 特征UUID1
#define Characteristic_UUID2 "de045162-3d97-11ed-b878-0242ac120002" // 特征UUID2

uint8_t DATA1[256] = {0};

// 构造函数,构造一个BLE特性(uuid为Characteristic_UUID1,properties为通知类
//  (自动接收信息(还有一个读取类需要手动读取信息))+写入类(发送信息))
BLECharacteristic G_Characteristic1(Characteristic_UUID1, BLECharacteristic::PROPERTY_WRITE);
BLECharacteristic G_Characteristic2(Characteristic_UUID2, BLECharacteristic::PROPERTY_NOTIFY);

bool DeviceConnected = false; // BLE连接是否成功的标志位

// 服务器新设备连接是否成功回调
class ServerCallbacks1 : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        DeviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        DeviceConnected = false;
    }
};

// 服务接收数据特性回调
class ServiceCallbacks1 : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
    }
};

void setup()
{
    Serial.begin(115200);

    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(12, INPUT_PULLDOWN);

    digitalWrite(4, LOW);
    digitalWrite(5, LOW);

    BLEDevice::init("T-Panel_V1.2_ESP32-H2");       // 创建设备
    BLEServer *pServer = BLEDevice::createServer(); // 设置为服务器
    pServer->setCallbacks(new ServerCallbacks1());

    BLEService *pService = pServer->createService(SERVICE_UUID); // 使用上面的服务UUID创建诺干服务

    pService->addCharacteristic(&G_Characteristic1);         // 增加一个上面定义的发送消息特性,否则客户端连接不上服务器
    G_Characteristic1.setCallbacks(new ServiceCallbacks1()); // 添加客户端的设备回调,这样BLE服务器才可以接收到客户端(手机)发的数据
    G_Characteristic1.setValue("Ciallo1");                   // BLE服务器向客户端发送数据(此值可以是传感器要发送的测量数据)

    pService->addCharacteristic(&G_Characteristic2); // 增加一个上面定义的发送消息特性,否则客户端连接不上服务器
    G_Characteristic2.addDescriptor(new BLE2902());  // 添加客户端的设备描述,这样客户端(手机)才可以收到BLE服务器发送的数据
    G_Characteristic2.setValue("Ciallo2");           // BLE服务器向客户端发送数据(此值可以是传感器要发送的测量数据)

    // 开始蓝牙服务
    pService->start();

    // 服务器开始广播
    pServer->getAdvertising()->start();

    Serial.println("BLE: Waiting a client connection to notify..."); // 提示消息

    while (DeviceConnected == false)
    {
        Serial.println("BLE: Waiting to pairing BLE device");
        delay(1000);

        if (digitalRead(12) == HIGH)
        {
            break;
        }
    }

    delay(500);
    if (DeviceConnected == false)
    {
        Serial.println("BLE: BLE device pairing failure");
    }
    else
    {
        Serial.println("BLE: BLE device pairing successful");
    }

    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
}

void loop()
{
    while (Serial.available() > 0)
    {
        Serial.read(DATA1, sizeof(DATA1));
        Serial.printf("%s", DATA1);
    }
}