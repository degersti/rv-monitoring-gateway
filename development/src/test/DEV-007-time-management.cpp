#include <Arduino.h>
#include <WiFi.h>
#include "app/time_manager.h"
#include "secrets.h"

uint32_t savedTimestamps[] ={0,20,40};

void connectWifi()
{
    Serial.print("Connecting to WiFi");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void setupDevTimeManagement()
{
    Serial.begin(115200);
    // Wait for user to connect to terminal after wakeup
    delay(10000);
    // initialize time Manager
    initTimeManager();
}

void loopDevTimeManagement()
{
    if(getCurrentTimestamp() >0)
    {
        connectWifi();
        if(isTimeSyncRequired())
        {     
           if(forceTimeSync())
           {
                 Serial.println("System time update: DONE");
                 for(int i=0; i < sizeof(savedTimestamps) / sizeof(savedTimestamps[0]); i++)
                 {
                    Serial.print(savedTimestamps[i]);
                    Serial.print(" updated to: ");
                    Serial.println(reconstructTimestamp(savedTimestamps[i]));
                 }
           }else{
                Serial.println("System time update: FAILED");
           }
        }else{
            Serial.println("System time update: NOT REQUIRED");
        }
    }
    // Print timestamp and timeAvailable
    Serial.print("Timestamp: ");
    Serial.print(getCurrentTimestamp());
    Serial.print(" | Time valid: ");
    Serial.println(isTimeValid() ? "YES" : "NO");
    Serial.flush();
    // Wait for serial data to be transmitted
    delay(5000);
    // Enter deep sleep
    esp_sleep_enable_timer_wakeup((uint64_t)5UL * 1000000ULL); 
    esp_deep_sleep_start();

}