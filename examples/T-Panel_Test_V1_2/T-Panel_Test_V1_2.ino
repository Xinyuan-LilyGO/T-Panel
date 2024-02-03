/*
 * @Description: T-Panel_V1.2出厂测试程序
 * @version: V1.0.2
 * @Author: LILYGO_L
 * @Date: 2023-09-16 10:47:28
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-01-19 11:14:52
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
#include <SPI.h>
#include <SD.h>
#include <HardwareSerial.h>
#include "TouchLib.h"
#include "Material_16Bit_480x480px.h"
#include "pin_config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/twai.h>

#define WIFI_SSID "xinyuandianzi"
#define WIFI_PASSWORD "AA15994823428"
// #define WIFI_SSID "LilyGo-AABB"
// #define WIFI_PASSWORD "xinyuandianzi"

#define WIFI_CONNECT_WAIT_MAX (5000)

#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET_SEC 8 * 3600 // Time zone setting function, written as 8 * 3600 in East Eighth Zone (UTC/GMT+8:00)
#define DAY_LIGHT_OFFSET_SEC 0  // Fill in 3600 for daylight saving time, otherwise fill in 0

bool Get_int = 0;
uint8_t Image_Flag = 0;
bool Touch_Flag = 0;
uint8_t Touch_Num = 0;

bool BLE_Pairing_Flag = 0;
uint8_t BLE_Pairing_Timeout_Count = 0;

uint8_t Last_X = 0;
uint8_t Last_Y = 0;

HardwareSerial SerialPort1(1); // use UART1

#if defined T_Panel_V1_2_RS485
HardwareSerial SerialPort2(2); // use UART2
#endif

char DATA1[100] = {0};

uint8_t GFX_Print_Uart_Cursor = 0;
bool Uart_Start = 0;
uint32_t Uart_Count = 0;

size_t cycleInterval = 0;

bool wifi_connection_failure_flag = false;
uint32_t wifi_time_delay = 0;

TouchLib touch(Wire, IIC_SDA, IIC_SCL, CST3240_Address);

Arduino_DataBus *bus = new Arduino_XL9535SWSPI(IIC_SDA /* SDA */, IIC_SCL /* SCL */, -1 /* XL PWD */,
                                               XL95X5_CS /* XL CS */, XL95X5_SCLK /* XL SCK */, XL95X5_MOSI /* XL MOSI */);
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    -1 /* DE */, LCD_VSYNC /* VSYNC */, LCD_HSYNC /* HSYNC */, LCD_PCLK /* PCLK */,
    LCD_B0 /* B0 */, LCD_B1 /* B1 */, LCD_B2 /* B2 */, LCD_B3 /* B3 */, LCD_B4 /* B4 */,
    LCD_G0 /* G0 */, LCD_G1 /* G1 */, LCD_G2 /* G2 */, LCD_G3 /* G3 */, LCD_G4 /* G4 */, LCD_G5 /* G5 */,
    LCD_R0 /* R0 */, LCD_R1 /* R1 */, LCD_R2 /* R2 */, LCD_R3 /* R3 */, LCD_R4 /* R4 */,
    1 /* hsync_polarity */, 20 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 0 /* hsync_back_porch */,
    1 /* vsync_polarity */, 30 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 1 /* vsync_back_porch */,
    10 /* pclk_active_neg */, 6000000L /* prefer_speed */, false /* useBigEndian */,
    0 /* de_idle_high*/, 0 /* pclk_idle_high */);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    LCD_WIDTH /* width */, LCD_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, -1 /* RST */, st7701_type9_init_operations, sizeof(st7701_type9_init_operations));

#if defined T_Panel_V1_2_CAN
void CAN_Drive_Initialization()
{
    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        Serial.println("Driver installed");
    }
    else
    {
        Serial.println("Failed to install driver");
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK)
    {
        Serial.println("Driver started");
    }
    else
    {
        Serial.println("Failed to start driver");
    }

    // 配置
    uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS |
                                TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
                                TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_DATA |
                                TWAI_ALERT_RX_QUEUE_FULL;

    if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
    {
        Serial.println("CAN Alerts reconfigured");
    }
    else
    {
        Serial.println("Failed to reconfigure alerts");
    }
}

void Twai_Send_Message()
{
    // Configure message to transmit
    twai_message_t message;
    message.identifier = 0x0F6;
    message.data_length_code = 1;
    message.data[0] = Uart_Count;

    // Queue message for transmission
    if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
    {
        printf("Message queued for transmission\n");
    }
    else
    {
        printf("Failed to queue message for transmission\n");
    }
}

void Twai_Receive_Message(twai_message_t &message)
{
    // Process received message
    if (message.extd)
    {
        Serial.println("Message is in Extended Format");
    }
    else
    {
        Serial.println("Message is in Standard Format");
    }
    Serial.printf("ID: %x\nByte:", message.identifier);
    if (!(message.rtr))
    {
        for (int i = 0; i < message.data_length_code; i++)
        {
            Serial.printf(" %d = %02x,", i, message.data[i]);
        }
        Serial.println("");
    }
}
#endif

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

void BLE_Test(void)
{
    gfx->setTextColor(CYAN);
    gfx->setCursor(80, 50);
    gfx->setTextSize(3);
    gfx->print("ESP32-H2 BLE Test");

    gfx->setCursor(0, 100);
    gfx->setTextSize(2);
    gfx->print("BLE Name: T-Panel_V1.2_ESP32-H2");

    gfx->setCursor(0, 250);
    gfx->print("ESP32-H2 BLE: ");

    gfx->fillRoundRect(80, 350, 280, 70, 10, ORANGE);
    gfx->setTextColor(WHITE);
    gfx->setCursor(100, 380);
    gfx->setTextSize(3);
    gfx->print("Click To Skip");

    while ((gfx->XL_digitalRead(XL95X5_ESP32H2_IO4) == LOW) ||
           (gfx->XL_digitalRead(XL95X5_ESP32H2_IO5) == LOW))
    {
        touch.read();
        TP_Point t = touch.getPoint(0);

        if ((t.x > 80 && t.x < 360) && (t.y > 350 && t.y < 420))
        {
            BLE_Pairing_Flag = false;
            break;
        }

        if (millis() > cycleInterval)
        {
            gfx->fillRect(170, 250, 310, 100, BLACK);
            gfx->setTextColor(ORANGE);
            gfx->setCursor(170, 250);
            gfx->setTextSize(2);
            gfx->print("Waiting For Pairing ");

            BLE_Pairing_Timeout_Count++;
            if (BLE_Pairing_Timeout_Count >= 20)
            {
                BLE_Pairing_Flag = false;
                break;
            }
            else
            {
                BLE_Pairing_Flag = true;
            }
            cycleInterval = millis() + 1000;
        }

        delay(500);
    }

    if (BLE_Pairing_Flag == true)
    {
        gfx->fillRect(170, 250, 310, 100, BLACK);
        gfx->setTextColor(GREEN);
        gfx->setCursor(170, 250);
        gfx->setTextSize(2);
        gfx->print("BLE Pairing Successful");
    }
    else
    {
        gfx->fillRect(170, 250, 310, 100, BLACK);
        gfx->setTextColor(RED);
        gfx->setCursor(170, 250);
        gfx->setTextSize(2);
        gfx->print("BLE Pairing Failed");

        gfx->XL_digitalWrite(XL95X5_ESP32H2_IO12, HIGH);
    }

    delay(2000);
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        gfx->fillRoundRect(10, 10, 200, 80, 15, WHITE);
        gfx->setCursor(20, 40);
        gfx->setTextSize(1);
        gfx->setTextColor(RED);
        gfx->print("Failed to obtain time!");
        return;
    }
    Serial.println("Get time success");
    Serial.println(&timeinfo, "%A,%B %d %Y %H:%M:%S"); // Format Output
    gfx->fillRoundRect(10, 10, 200, 80, 15, WHITE);
    gfx->setCursor(20, 20);
    gfx->setTextSize(2);
    gfx->setTextColor(PINK);
    gfx->print(&timeinfo, " %Y");
    gfx->setCursor(20, 40);
    gfx->print(&timeinfo, "%B %d");
    gfx->setCursor(20, 60);
    gfx->print(&timeinfo, "%H:%M:%S");
}

void wifi_test(void)
{
    String text;
    int wifi_num = 0;

    gfx->setTextSize(2);
    gfx->setTextColor(GREEN);

    Serial.println("\nScanning wifi");
    gfx->printf("Scanning wifi\n");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    wifi_num = WiFi.scanNetworks();
    if (wifi_num == 0)
    {
        text = "\nWiFi scan complete !\nNo wifi discovered.\n";
    }
    else
    {
        text = "\nWiFi scan complete !\n";
        text += wifi_num;
        text += " wifi discovered.\n\n";

        for (int i = 0; i < wifi_num; i++)
        {
            text += (i + 1);
            text += ": ";
            text += WiFi.SSID(i);
            text += " (";
            text += WiFi.RSSI(i);
            text += ")";
            text += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
            delay(10);
        }
    }

    Serial.println(text);
    gfx->println(text);

    text = "Connecting to ";
    Serial.print("Connecting to ");
    gfx->printf("Connecting to\n");
    text += WIFI_SSID;
    text += "\n";

    Serial.print(WIFI_SSID);
    gfx->printf("%s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t last_tick = millis();
    uint32_t i = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        gfx->printf(".");
        text += ".";
        delay(100);

        if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX)
        {
            wifi_connection_failure_flag = true;
            break;
        }
    }

    if (!wifi_connection_failure_flag)
    {
        text += "\nThe connection was successful ! \nTakes ";
        Serial.print("\nThe connection was successful ! \nTakes ");
        gfx->printf("\nThe connection was successful ! \nTakes ");
        text += millis() - last_tick;
        Serial.print(millis() - last_tick);
        gfx->print(millis() - last_tick);
        text += " ms\n";
        Serial.println(" ms\n");
        gfx->printf(" ms\n");

        gfx->setTextSize(3);
        gfx->setTextColor(GREEN);
        gfx->printf("\n\n\nWifi test passed!");
    }
    else
    {
        gfx->setTextSize(3);
        gfx->setTextColor(RED);
        gfx->printf("\n\n\nWifi test error!\n");
    }
}

void SD_Test(void)
{
    uint8_t SelfLocking_Flag = 0;
    uint8_t cardType = 0;
    uint64_t cardSize = 0;
    uint8_t numSectors = 0;

    while (!SD.begin(SD_CS, SPI, 40000000) && SelfLocking_Flag != 3) // SD boots
    {
        SelfLocking_Flag++;
        gfx->setTextSize(2);
        gfx->setTextColor(GREEN);
        Serial.println("Detecting SD card");
        gfx->printf("Detecting SD card\n");

        gfx->setTextColor(RED);
        Serial.println("SD card initialization failed !");
        gfx->printf("SD card initialization failed !\n");
        delay(100);

        Serial.println(".");
        gfx->printf(".");
        delay(100);

        Serial.println(".");
        gfx->printf(".");
        delay(100);

        Serial.println(".");
        gfx->printf(".");
        delay(100);

        Serial.println(".");
        gfx->printf(".");
        delay(100);

        Serial.println(".");
        gfx->printf(".");
        delay(100);

        Serial.println(".");
        gfx->printf(".");
        delay(100);

        gfx->printf("\n");
    }
    if (SelfLocking_Flag < 3)
    {
        delay(50); // Wait for the SD card
        gfx->setTextSize(2);
        gfx->setTextColor(GREEN);

        Serial.println("SD card initialization successful !");
        gfx->printf("SD card initialization successful !\n");
        delay(100);

        cardType = SD.cardType();
        cardSize = SD.cardSize() / (1024 * 1024);
        numSectors = SD.numSectors();
        switch (cardType)
        {
        case CARD_NONE:
            Serial.println("No SD card attached");
            gfx->printf("No SD card attached\n");

            gfx->setTextSize(3);
            gfx->setTextColor(RED);
            gfx->printf("\n\n\nSD card test error!\n");
            delay(100);

            break;
        case CARD_MMC:
            Serial.print("SD Card Type: ");
            Serial.println("MMC");
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            gfx->printf("SD Card Type: MMC\nSD Card Size: %lluMB\n", cardSize);

            gfx->setTextSize(3);
            gfx->setTextColor(GREEN);
            gfx->printf("\n\n\nSD card test passed!\n");
            delay(100);

            break;
        case CARD_SD:
            Serial.print("SD Card Type: ");
            Serial.println("SDSC");
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            gfx->printf("SD Card Type: SDSC\nSD Card Size: %lluMB\n", cardSize);

            gfx->setTextSize(3);
            gfx->setTextColor(GREEN);
            gfx->printf("\n\n\nSD card tested correct!\n");
            delay(100);

            break;
        case CARD_SDHC:
            Serial.print("SD Card Type: ");
            Serial.println("SDHC");
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            gfx->printf("SD Card Type: SDHC\nSD Card Size: %lluMB\n", cardSize);

            gfx->setTextSize(3);
            gfx->setTextColor(GREEN);
            gfx->printf("\n\n\nSD card tested correct!\n");
            delay(100);

            break;
        default:
            Serial.println("UNKNOWN");
            gfx->printf("UNKNOWN");

            gfx->setTextSize(3);
            gfx->setTextColor(RED);
            gfx->printf("\n\n\nSD card test error!\n");
            delay(100);

            break;
        }
    }
    else
    {
        gfx->setTextSize(3);
        gfx->setTextColor(RED);
        gfx->printf("\n\n\nSD card test error!\n");
    }
    SD.end();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    SerialPort1.begin(115200, SERIAL_8N1, ESP32H2_RX, ESP32H2_TX);

#if defined T_Panel_V1_2_RS485
    SerialPort2.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);
#endif

#if defined T_Panel_V1_2_CAN
    CAN_Drive_Initialization();
#endif

    pinMode(ESP32H2_BOOT, OUTPUT);
    digitalWrite(ESP32H2_BOOT, HIGH); // ESP32H2的Boot引脚置高

    pinMode(ESP32H2_EN, OUTPUT);
    digitalWrite(ESP32H2_EN, LOW);

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);

    pinMode(TOUCH_INT, INPUT_PULLUP);
    digitalWrite(TOUCH_INT, HIGH);

    attachInterrupt(
        TOUCH_INT,
        []
        {
            Get_int = 1;
            Serial.println("get_int");
        },
        FALLING); // Triggered every 1ms

    ledcAttachPin(LCD_BL, 1);
    ledcSetup(1, 1000, 8);
    ledcWrite(1, 0);

    Wire.begin(IIC_SDA, IIC_SCL);
    // scan_i2c_device(Wire);

    gfx->begin();
    gfx->fillScreen(WHITE);

    gfx->setCursor(180, 200);
    gfx->setTextSize(4);
    gfx->setTextColor(PINK);
    gfx->printf("Ciallo");

    for (int j = 0; j < 2; j++)
    {
        delay(2000);
        for (int i = 0; i <= 255; i++)
        {
            ledcWrite(1, i);
            delay(2);
        }
        for (int i = 255; i >= 0; i--)
        {
            ledcWrite(1, i);
            delay(5);
        }
    }
    delay(2000);
    for (int i = 0; i <= 255; i++)
    {
        ledcWrite(1, i);
        delay(2);
    }
    delay(1000);

    // Touch
    gfx->XL_digitalWrite(XL95X5_TOUCH_RST, LOW);
    delay(200);
    gfx->XL_digitalWrite(XL95X5_TOUCH_RST, HIGH);
    delay(200);

    gfx->XL_digitalWrite(XL95X5_RS485_CON, HIGH);

    touch.init();

    // ESP32H2 Wifi Test

    gfx->XL_pinMode(XL95X5_ESP32H2_IO12, OUTPUT);
    gfx->XL_digitalWrite(XL95X5_ESP32H2_IO12, LOW);

    gfx->XL_pinMode(XL95X5_ESP32H2_IO4, INPUT);
    gfx->XL_pinMode(XL95X5_ESP32H2_IO5, INPUT);

    digitalWrite(ESP32H2_EN, HIGH); // ESP32H2系统重置
    delay(100);
    digitalWrite(ESP32H2_EN, LOW);
    delay(100);
    digitalWrite(ESP32H2_EN, HIGH);
    delay(1000);

    gfx->fillScreen(BLACK);
    gfx->setCursor(0, 0);

    BLE_Test();

    // ESP32S3 Wifi Test
    gfx->fillScreen(BLACK);
    gfx->setCursor(0, 0);
    wifi_test();

    if (!wifi_connection_failure_flag)
    {
        // Obtain and set the time from the network time server
        // After successful acquisition, the chip will use the RTC clock to update the holding time
        configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    }
    delay(5000);

    // SD Test
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS); // SPI boots
    gfx->fillScreen(BLACK);
    gfx->setCursor(0, 0);
    SD_Test();
    delay(5000);

    touch.read();
    TP_Point t = touch.getPoint(0);
    Last_X = t.x;
    Last_Y = t.y;

    gfx->fillScreen(WHITE);
    gfx->setCursor(100, 200);
    gfx->setTextSize(3);
    gfx->setTextColor(PURPLE);
    gfx->printf("Edge Detection");

    // gfx->drawRect(10, 10, 460, 460, RED);
    gfx->drawRect(0, 0, 480, 480, RED);

    gfx->setTextSize(2);
    gfx->setTextColor(NAVY);
}

void loop()
{
    if (Uart_Start == 1)
    {
#if defined T_Panel_V1_2_CAN
        // 总线状态信息
        twai_status_info_t twai_status_info;
        twai_get_status_info(&twai_status_info);
        // 通信报警检测
        uint32_t alerts_triggered;
        twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(1000));

        switch (alerts_triggered)
        {
        case TWAI_ALERT_ERR_PASS:
            Serial.println("\nAlert: TWAI controller has become error passive.");
            break;
        case TWAI_ALERT_BUS_ERROR:
            Serial.println("\nAlert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus.");
            Serial.printf("Bus error count: %d\n", twai_status_info.bus_error_count);
            break;
        case TWAI_ALERT_TX_FAILED:
            Serial.println("\nAlert: The Transmission failed.");
            Serial.printf("TX buffered: %d\t", twai_status_info.msgs_to_tx);
            Serial.printf("TX error: %d\t", twai_status_info.tx_error_counter);
            Serial.printf("TX failed: %d\n", twai_status_info.tx_failed_count);
            break;
        case TWAI_ALERT_TX_SUCCESS:
            Serial.println("\nAlert: The Transmission was successful.");
            Serial.printf("TX buffered: %d\t", twai_status_info.msgs_to_tx);
            break;
        case TWAI_ALERT_RX_QUEUE_FULL:
            Serial.println("\nAlert: The RX queue is full causing a received frame to be lost.");
            Serial.printf("RX buffered: %d\t", twai_status_info.msgs_to_rx);
            Serial.printf("RX missed: %d\t", twai_status_info.rx_missed_count);
            Serial.printf("RX overrun %d\n", twai_status_info.rx_overrun_count);
            break;

        default:
            break;
        }
#endif

        if (millis() > wifi_time_delay)
        {
            if (!wifi_connection_failure_flag)
            {
                printLocalTime();
            }
            else
            {
                gfx->fillRoundRect(10, 10, 200, 80, 15, WHITE);
                gfx->setCursor(20, 40);
                gfx->setTextSize(1);
                gfx->setTextColor(RED);
                gfx->print("Not connected to the network");
            }
            wifi_time_delay = millis() + 500;
        }

        if (millis() > cycleInterval)
        {
            Uart_Count++;
            gfx->setCursor(250, 360 + GFX_Print_Uart_Cursor);
            gfx->setTextSize(1);
            gfx->setTextColor(CYAN);

            gfx->setTextColor(YELLOW);
            gfx->printf("TX-> ");
            gfx->setTextColor(CYAN);
            gfx->printf("ESP32S3 to ESP32H2:%d", Uart_Count);

            GFX_Print_Uart_Cursor += 10;
            if (GFX_Print_Uart_Cursor > 130)
            {
                GFX_Print_Uart_Cursor = 0;
                gfx->fillRect(230, 330, 280, 150, BLACK);
                gfx->setCursor(265, 340);
                gfx->setTextSize(2);
                gfx->setTextColor(BLUE);
                gfx->printf("UART Information");
            }

#if defined T_Panel_V1_2_RS485
            gfx->setCursor(250, 360 + GFX_Print_Uart_Cursor);
            gfx->setTextSize(1);
            gfx->setTextColor(CYAN);

            gfx->setTextColor(YELLOW);
            gfx->printf("TX-> ");
            gfx->setTextColor(CYAN);
            gfx->printf("ESP32S3 to RS485: %d", Uart_Count);

            GFX_Print_Uart_Cursor += 10;
            if (GFX_Print_Uart_Cursor > 130)
            {
                GFX_Print_Uart_Cursor = 0;
                gfx->fillRect(230, 330, 280, 150, BLACK);
                gfx->setCursor(265, 340);
                gfx->setTextSize(2);
                gfx->setTextColor(BLUE);
                gfx->printf("UART Information");
            }
#endif

#if defined T_Panel_V1_2_CAN
            gfx->setCursor(250, 360 + GFX_Print_Uart_Cursor);
            gfx->setTextSize(1);
            gfx->setTextColor(CYAN);

            gfx->setTextColor(YELLOW);
            gfx->printf("TX-> ");
            gfx->setTextColor(CYAN);
            gfx->printf("ESP32S3 to CAN: %d", Uart_Count);

            GFX_Print_Uart_Cursor += 10;
            if (GFX_Print_Uart_Cursor > 130)
            {
                GFX_Print_Uart_Cursor = 0;
                gfx->fillRect(230, 330, 280, 150, BLACK);
                gfx->setCursor(265, 340);
                gfx->setTextSize(2);
                gfx->setTextColor(BLUE);
                gfx->printf("UART Information");
            }
#endif

            cycleInterval = millis() + 2000;

            while (SerialPort1.available() > 0) // 清除串口1缓存
            {
                SerialPort1.read();
            }

#if defined T_Panel_V1_2_RS485
            while (SerialPort2.available() > 0) // 清除串口2缓存
            {
                SerialPort2.read();
            }

            gfx->XL_digitalWrite(XL95X5_RS485_CON, HIGH);
            SerialPort2.printf("RS485: %d", Uart_Count);
            gfx->XL_digitalWrite(XL95X5_RS485_CON, LOW);
#endif

            SerialPort1.print(Uart_Count);

#if defined T_Panel_V1_2_CAN
            switch (twai_status_info.state)
            {
            case TWAI_STATE_RUNNING:
                Serial.println("\nTWAI_STATE_RUNNING");
                break;
            case TWAI_STATE_BUS_OFF:
                Serial.println("\nTWAI_STATE_BUS_OFF");
                twai_initiate_recovery();
                break;
            case TWAI_STATE_STOPPED:
                Serial.println("\nTWAI_STATE_STOPPED");
                twai_start();
                break;
            case TWAI_STATE_RECOVERING:
                Serial.println("\nTWAI_STATE_RECOVERING");
                break;

            default:
                break;
            }

            Twai_Send_Message();
#endif
        }

#if defined T_Panel_V1_2_CAN
        // 如果TWAI有信息接收到
        if (alerts_triggered & TWAI_ALERT_RX_DATA)
        {
            twai_message_t rx_buf;
            while (twai_receive(&rx_buf, 0) == ESP_OK)
            {
                Twai_Receive_Message(rx_buf);

                Serial.printf("%d", rx_buf.data[0]);

                gfx->setCursor(250, 360 + GFX_Print_Uart_Cursor);
                gfx->setTextSize(1);
                gfx->setTextColor(GREEN);

                gfx->setTextColor(YELLOW);
                gfx->printf("RX-> ");
                gfx->setTextColor(GREEN);
                gfx->printf("CAN to ESP32S3: %d", rx_buf.data[0]);

                GFX_Print_Uart_Cursor += 10;
                if (GFX_Print_Uart_Cursor > 130)
                {
                    GFX_Print_Uart_Cursor = 0;
                    gfx->fillRect(230, 330, 280, 150, BLACK);
                    gfx->setCursor(265, 340);
                    gfx->setTextSize(2);
                    gfx->setTextColor(BLUE);
                    gfx->printf("UART Information");
                }
            }
        }
#endif

        while (SerialPort1.available() > 0)
        {
            memset(DATA1, 0, sizeof(DATA1));

            SerialPort1.read(DATA1, sizeof(DATA1));
            Serial.printf("%s", DATA1);

            gfx->setCursor(250, 360 + GFX_Print_Uart_Cursor);
            gfx->setTextSize(1);
            gfx->setTextColor(GREEN);

            gfx->setTextColor(YELLOW);
            gfx->printf("RX-> ");
            gfx->setTextColor(GREEN);
            gfx->printf("ESP32H2 to ESP32S3: %s", DATA1);

            GFX_Print_Uart_Cursor += 10;
            if (GFX_Print_Uart_Cursor > 130)
            {
                GFX_Print_Uart_Cursor = 0;
                gfx->fillRect(230, 330, 280, 150, BLACK);
                gfx->setCursor(265, 340);
                gfx->setTextSize(2);
                gfx->setTextColor(BLUE);
                gfx->printf("UART Information");
            }
        }

#if defined T_Panel_V1_2_RS485
        while (SerialPort2.available() > 0)
        {
            memset(DATA1, 0, sizeof(DATA1));

            SerialPort2.read(DATA1, sizeof(DATA1));
            Serial.printf("%s", DATA1);

            gfx->setCursor(250, 360 + GFX_Print_Uart_Cursor);
            gfx->setTextSize(1);
            gfx->setTextColor(GREEN);

            gfx->setTextColor(YELLOW);
            gfx->printf("RX-> ");
            gfx->setTextColor(GREEN);
            gfx->printf("RS485 to ESP32S3: %s", DATA1);

            GFX_Print_Uart_Cursor += 10;
            if (GFX_Print_Uart_Cursor > 130)
            {
                GFX_Print_Uart_Cursor = 0;
                gfx->fillRect(230, 330, 280, 150, BLACK);
                gfx->setCursor(265, 340);
                gfx->setTextSize(2);
                gfx->setTextColor(BLUE);
                gfx->printf("UART Information");
            }
        }
#endif
    }

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
        gfx->setTextSize(2);
        gfx->setTextColor(NAVY);
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
            Image_Flag++;
            if (Image_Flag > 4)
            {
                Image_Flag = 0;
            }

            Last_X = t.x;
            Last_Y = t.y;

            Uart_Start = 1;
            GFX_Print_Uart_Cursor = 0;
            gfx->fillRect(230, 330, 280, 150, BLACK);
            gfx->setCursor(265, 340);
            gfx->setTextSize(2);
            gfx->setTextColor(BLUE);
            gfx->printf("UART Information");
        }
    }
}
