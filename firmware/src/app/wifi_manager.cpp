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
 * - Non-blocking WiFi connection management
 * - Connection status monitoring
 * - Secure client provisioning
 *
 *************************************************/

#include <Arduino.h>
#include "config.h"
#include "secrets.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "app/debug_logger.h"

// WiFi network credentials
const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;

// Secure TCP/TLS client used by MQTT
static WiFiClientSecure espClient;

static WiFiConnectionState wifiState = WiFiConnectionState::IDLE;
static uint32_t connectStartTime = 0;
static uint32_t lastRetryTime = 0;
static uint32_t lastStatusPrintTime = 0;

static void startWifiConnection(void)
{
    LOG_INFO("Connecting to WiFi");

    WiFi.disconnect(true);
    delay(10);
    WiFi.begin(wifi_ssid, wifi_password);

    connectStartTime = millis();
    lastStatusPrintTime = connectStartTime;
    wifiState = WiFiConnectionState::CONNECTING;
}

/*************************************************
 * Function:    initWifi
 * Description: Initializes WiFi handling and the
 *              secure client used by MQTT.
 * Parameters:  None
 * Returns:     None
 * Notes:       Does not block and does not start a
 *              connection attempt by itself.
 *************************************************/
void initWifi(void)
{
    LOG_INFO("Initializing WiFi");
    WiFi.mode(WIFI_STA);
    espClient.setInsecure();
    wifiState = WiFiConnectionState::IDLE;
}

/*************************************************
 * Function:    processWifiConnection
 * Description: Handles WiFi connection progress
 *              without blocking the main loop.
 * Parameters:  None
 * Returns:     Current WiFi connection state
 * Notes:       Must be called repeatedly while the
 *              application is trying to connect.
 *************************************************/
WiFiConnectionState processWifiConnection(void)
{
    const uint32_t now = millis();

    if (WiFi.status() == WL_CONNECTED)
    {
        if (wifiState != WiFiConnectionState::CONNECTED)
        {
            LOG_INFO("WiFi connected");
            LOG_DEBUG("IP Address: %s", WiFi.localIP().toString().c_str());
        }

        wifiState = WiFiConnectionState::CONNECTED;
        return wifiState;
    }

    if (wifiState == WiFiConnectionState::CONNECTED)
    {
        LOG_INFO("WiFi disconnected");
        wifiState = WiFiConnectionState::IDLE;
    }

    if (wifiState == WiFiConnectionState::IDLE)
    {
        startWifiConnection();
        return wifiState;
    }

    if (wifiState == WiFiConnectionState::FAILED)
    {
        if (now - lastRetryTime >= WIFI_RETRY_INTERVAL_MS)
        {
            startWifiConnection();
        }

        return wifiState;
    }

    if (wifiState == WiFiConnectionState::CONNECTING)
    {
        if (now - lastStatusPrintTime >= WIFI_STATUS_PRINT_INTERVAL_MS)
        {
            LOG_PROGRESS(".");
            lastStatusPrintTime = now;
        }

        if (now - connectStartTime >= WIFI_CONNECT_TIMEOUT_MS)
        {
            LOG_PROGRESS("\n");
            LOG_WARN("WiFi connection timeout");

            WiFi.disconnect(true);
            lastRetryTime = now;
            wifiState = WiFiConnectionState::FAILED;
        }
    }

    return wifiState;
}

/*************************************************
 * Function:    resetWifiConnection
 * Description: Resets WiFi connection handling so
 *              the next process call starts again.
 * Parameters:  None
 * Returns:     None
 * Notes:       Disconnects WiFi and clears the
 *              connection state.
 *************************************************/
void disconnectWifi(void)
{
    WiFi.disconnect(true);
    wifiState = WiFiConnectionState::IDLE;
    connectStartTime = 0;
    lastRetryTime = 0;
    lastStatusPrintTime = 0;
}

/*************************************************
 * Function:    connectWifi
 * Description: Compatibility wrapper for older 
 *              code. Starts or continues a 
 *              non-blocking connection attempt.
 * Returns:     true  - WiFi connected
 *              false - WiFi not connected yet 
 *              or failed
 * Notes:       Should be called repeatedly while 
 *              the application is trying to connect.
 *************************************************/
bool connectWifi(void)
{
    return processWifiConnection() == WiFiConnectionState::CONNECTED;
}

/*************************************************
 * Function:    getWiFiConnectionStatus
 * Description: Returns the current WiFi connection
 *              state.
 * Parameters:  None
 * Returns:     true  - WiFi connected
 *              false - WiFi not connected yet 
 *              or failed
 * Notes:       None
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
 * Returns:     Reference to the secure WiFi client
 * Notes:       The client is configured to accept
 *              any server certificate without 
 *              validation.
 *************************************************/
Client& getWifiClient(void)
{
    return espClient;
}
