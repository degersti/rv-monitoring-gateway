/*************************************************
 * File:        wifi_manager.cpp
 * Author:      Markus Gerstenberg
 * Date:        2026-06-19
 *
 * Description:
 * Handles WiFi network connectivity and provides
 * access to the secure network client.
 *
 * Responsibilities:
 * - WiFi connection management
 * - Connection status monitoring
 * - Secure client provisioning
 *
 *************************************************/

#include <Arduino.h>
#include "config.h"
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

// WiFi network credentials
const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;

// Secure TCP/TLS client used by MQTT
static WiFiClientSecure espClient;

/*************************************************
 * Function:    connectWifi
 * Description: Connects the ESP32 to the configured 
 *              WiFi network.
 * Parameters:  None
 * Returns:     true  - WiFi connected successfully
 *              false - Connection timeout or failed
 * Notes:       Uses a fixed connection timeout and 
 *              configures the TLS client to accept 
 *              insecure certificates.
 *************************************************/
bool connectWifi(void)
{
    WiFi.begin(wifi_ssid, wifi_password);

    Serial.print("Connecting to WiFi");

    const uint32_t timeoutMs = 10000;
    uint32_t startTime = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - startTime > timeoutMs)
        {
            Serial.println();
            Serial.println("WiFi connection timeout");

            WiFi.disconnect();
            return false;
        }

        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    espClient.setInsecure();

    return true;
}
/*************************************************
 * Function:    getWiFiConnectionStatus
 * Description: Returns the current WiFi connection 
 *              state.
 * Parameters:  None
 * Returns:     true  - WiFi connected
 *               false - WiFi disconnected
 * Notes:       Wrapper around WiFi.status().
 *************************************************/
bool getWiFiConnectionStatus(void)
{
    return (WiFi.status() == WL_CONNECTED);
}
/*************************************************
 * Function:    getWifiClient
 * Description: Provides access to the secure WiFi 
 *              client used for MQTT communication.
 * Parameters:  None
 * Returns:     Reference to the WiFiClientSecure 
 *              instance
 * Notes:       The client is configured in 
 *              connectWifi().
 *************************************************/
Client& getWifiClient(void)
{
    return espClient;
}