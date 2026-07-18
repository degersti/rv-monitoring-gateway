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
#include "app/debug_strings.h"

// WiFi network credentials
const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;

// Secure TCP/TLS client used by MQTT
static WiFiClientSecure espClient;

static WiFiConnectionState wifiState = WiFiConnectionState::IDLE;
static uint32_t connectStartTime = 0;
static uint32_t lastRetryTime = 0;
static uint32_t wifiConnectionAttempt = 0;

static void startWifiConnection(void)
{
    wifiConnectionAttempt++;
    if (wifiConnectionAttempt == 1)
    {
        LOG_INFO(
            "WiFi status: CONNECTING [SSID=%s, timeout=%.1f s]",
            wifi_ssid,
            WIFI_CONNECT_TIMEOUT_MS / 1000.0f);
    }
    else
    {
        LOG_INFO(
            "WiFi status: CONNECTING [attempt=%lu]",
            static_cast<unsigned long>(wifiConnectionAttempt));
    }



    WiFi.disconnect(true);
    delay(10);
    WiFi.begin(wifi_ssid, wifi_password);

    connectStartTime = millis();
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
    connectStartTime = 0;
    lastRetryTime = 0;
    wifiConnectionAttempt = 0;
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
            const uint32_t elapsedTime = millis() - connectStartTime;

            LOG_INFO(
                "WiFi status: CONNECTING [SSID=%s, attempt=%lu, elapsed=%.1f s]",
                wifi_ssid,
                static_cast<unsigned long>(wifiConnectionAttempt),
                elapsedTime / 1000.0f);

            LOG_DEBUG(
                "WiFi network details: IP=%s, RSSI=%d dBm",
                WiFi.localIP().toString().c_str(),
                WiFi.RSSI());
        }

        wifiState = WiFiConnectionState::CONNECTED;
        return wifiState;
    }

    if (wifiState == WiFiConnectionState::CONNECTED)
    {
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
        if (now - connectStartTime >= WIFI_CONNECT_TIMEOUT_MS)
        {
            const uint32_t elapsedTime = now - connectStartTime;
            const wl_status_t status = WiFi.status();

            LOG_DEBUG(
                "WiFi connection failed: reason=%s attempt=%lu, elapsed=%.1f s",
                getWifiStatusName(status),
                static_cast<unsigned long>(wifiConnectionAttempt),
                elapsedTime / 1000.0f);

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
    wifiConnectionAttempt = 0;

    LOG_DEBUG("WiFi network details: status=%s",
        getWifiStatusName(WiFi.status()));
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
