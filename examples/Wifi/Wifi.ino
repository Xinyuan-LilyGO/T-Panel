/*
 * @Description: Wifi test
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-08-18 15:38:10
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2023-08-18 15:47:26
 * @License: GPL 3.0
 */
#include "Arduino.h"
#include "WiFi.h"
#include <HTTPClient.h>

#define WIFI_SSID "LilyGo-AABB"
#define WIFI_PASSWORD "xinyuandianzi"

#define WIFI_CONNECT_WAIT_MAX (30 * 1000)

#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET_SEC 8 * 3600 // Time zone setting function, written as 8 * 3600 in East Eighth Zone (UTC/GMT+8:00)
#define DAY_LIGHT_OFFSET_SEC 0  // Fill in 3600 for daylight saving time, otherwise fill in 0

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println("Get time success");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S"); // Format Output
}

void wifi_test(void)
{
    String text;
    int wifi_num = 0;

    Serial.println("\nScanning wifi");
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

    text = "Connecting to ";
    Serial.print("Connecting to ");
    text += WIFI_SSID;
    text += "\n";

    Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t last_tick = millis();
    uint32_t i = 0;
    bool is_smartconfig_connect = false;

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        text += ".";
        delay(100);

        if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX)
        { /* Automatically start smartconfig when connection times out */
            text += "\nConnection timed out, start smartconfig";

            is_smartconfig_connect = true;
            WiFi.mode(WIFI_AP_STA);
            Serial.println("\r\n wait for smartconfig....");
            text += "\r\n wait for smartconfig....";
            text += "\nPlease use #ff0000 EspTouch# Apps to connect to the distribution network";
            WiFi.beginSmartConfig();

            while (1)
            {
                if (WiFi.smartConfigDone())
                {
                    Serial.println("\r\nSmartConfig Success\r\n");
                    Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
                    Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
                    text += "\nSmartConfig Success";
                    text += "\nSSID:";
                    text += WiFi.SSID().c_str();
                    text += "\nPSW:";
                    text += WiFi.psk().c_str();
                    last_tick = millis();
                    break;
                }
            }
        }
    }

    if (!is_smartconfig_connect)
    {
        text += "\nThe connection was successful ! \nTakes ";
        Serial.print("\nThe connection was successful ! \nTakes ");
        text += millis() - last_tick;
        Serial.print(millis() - last_tick);
        text += " ms\n";
        Serial.println(" ms\n");
    }
}

void setup()
{
    Serial.begin(115200);

    wifi_test();

    // Obtain and set the time from the network time server
    // After successful acquisition, the chip will use the RTC clock to update the holding time
    configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    printLocalTime();

    Serial.print("\n");
    Serial.println("Connecting https://www.baidu.com......");
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = "https://www.baidu.com"; // URL of Baidu homepage

        http.begin(url);                   // Initiate HTTP request
        int httpResponseCode = http.GET(); // Get response status code

        if (httpResponseCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTPS] GET... code: %d\n", httpResponseCode);

            // file found at server
            if (httpResponseCode == HTTP_CODE_OK || httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                String payload = http.getString();
                Serial.println(payload);
            }
        }
        else
        {
            Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
        }

        http.end(); 
    }
}

void loop()
{
    printLocalTime();
    delay(1000);
}
