/*************************************************
 * File:        DEV-004-wifi-and-mqtt.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Integration test for WiFi network connectivity
 * and MQTT communication through network_manager.
 *
 * Test objective:
 * Verifies the complete WiFi and MQTT connection
 * path through the network_manager.
 *
 *
 * TEST 1 - Normal connection
 * --------------------------
 * Preconditions:
 * - Valid WiFi credentials
 * - WiFi access point available
 * - MQTT broker reachable
 *
 * Expected result:
 * - Network state changes:
 *     IDLE -> CONNECTING -> CONNECTED
 * - MQTT state changes:
 *     IDLE -> CONNECTING -> CONNECTED
 * - MQTT connection remains active
 * - Telemetry is published every 10 seconds
 *
 *
 * TEST 2 - Network connection failure
 * -----------------------------------
 * Preconditions:
 * - WiFi access point unavailable or invalid
 *   WiFi credentials configured
 *
 * Expected result:
 * - Network state changes:
 *     IDLE -> CONNECTING -> FAILED
 * - MQTT connection is not established
 * - Network connection cycle remains FAILED
 *
 *
 * TEST 3 - Connection loss
 * ------------------------
 * Preconditions:
 * - Start test with working WiFi connection
 * - Wait until Network and MQTT are CONNECTED
 * - Disable the WiFi access point afterwards
 *
 * Expected result:
 * - WiFi connection loss is detected
 * - WiFi reconnect is attempted
 * - Network state changes from CONNECTED back
 *   into the connection process
 * - If reconnect succeeds:
 *     Network returns to CONNECTED
 *     MQTT reconnects if required
 * - If reconnect fails:
 *     Network eventually reaches FAILED
 *
 *
 * Notes:
 * - WiFi is configured as priority network.
 * - Cellular fallback is intentionally disabled.
 * - processNetworkConnection() must be called
 *   continuously, even while connected, so that
 *   connection loss can be detected.
 * - A FAILED network state ends the current
 *   connection cycle.
 * - A completely new network connection cycle
 *   must be started externally using initNetwork().
 *************************************************/

#include <Arduino.h>

#include "app/network_manager.h"
#include "app/mqtt_client.h"
#include "app/data_manager.h"
#include "app/runtime_manager.h"
#include "app/debug_logger.h"


static constexpr uint32_t PUBLISH_INTERVAL_MS = 10000;

static uint32_t lastPublishTime = 0;
static uint32_t publishCounter = 0;

static NetworkConnectionState previousNetworkState =
    NetworkConnectionState::IDLE;

static MqttConnectionState previousMqttState =
    MqttConnectionState::IDLE;


/*************************************************
 * Function:    getNetworkStateName
 * Description: Converts a network connection
 *              state into a readable string.
 * Parameters:  state - Network connection state
 * Returns:     State name as string
 * Notes:       Used for development test output.
 *************************************************/
static const char* getNetworkStateName(
    NetworkConnectionState state)
{
    switch (state)
    {
        case NetworkConnectionState::IDLE:
            return "IDLE";

        case NetworkConnectionState::CONNECTING:
            return "CONNECTING";

        case NetworkConnectionState::CONNECTED:
            return "CONNECTED";

        case NetworkConnectionState::FAILED:
            return "FAILED";
    }

    return "UNKNOWN";
}


/*************************************************
 * Function:    getMqttStateName
 * Description: Converts an MQTT connection state
 *              into a readable string.
 * Parameters:  state - MQTT connection state
 * Returns:     State name as string
 * Notes:       Used for development test output.
 *************************************************/
static const char* getMqttStateName(
    MqttConnectionState state)
{
    switch (state)
    {
        case MqttConnectionState::IDLE:
            return "IDLE";

        case MqttConnectionState::CONNECTING:
            return "CONNECTING";

        case MqttConnectionState::CONNECTED:
            return "CONNECTED";

        case MqttConnectionState::FAILED:
            return "FAILED";
    }

    return "UNKNOWN";
}


/*************************************************
 * Function:    setupDevWifiAndMqtt
 * Description: Initializes all modules required
 *              for the WiFi and MQTT integration
 *              test.
 * Parameters:  None
 * Returns:     None
 * Notes:       WiFi is used as priority network.
 *              Cellular fallback is disabled so
 *              WiFi failure can be tested directly.
 *************************************************/
void setupDevWifiAndMqtt()
{
    initSerialDebugNow();
    delay(100);

    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("DEV-004 WiFi and MQTT Test");
    Serial.println("--------------------------------");

    // -------------------------------------------------
    // Initialize runtime services
    // -------------------------------------------------

    initRuntimeManager();

    // -------------------------------------------------
    // Initialize network manager
    //
    // WiFi is the priority network.
    // Fallback is disabled for this test.
    // -------------------------------------------------

    initNetwork(NetworkType::CELLULAR, false);

    // -------------------------------------------------
    // Initialize MQTT client
    //
    // The client is provided by network_manager.
    // The MQTT layer therefore remains independent
    // of the underlying network interface.
    // -------------------------------------------------

    initMqtt(getNetworkClient());

    Serial.println();
    Serial.println("Test configuration:");
    Serial.println("Priority network : WIFI");
    Serial.println("Fallback         : DISABLED");
    Serial.println();
}


/*************************************************
 * Function:    loopDevWifiAndMqtt
 * Description: Processes network and MQTT
 *              connectivity and publishes test
 *              telemetry periodically.
 * Parameters:  None
 * Returns:     None
 * Notes:       Must be called continuously from
 *              the application loop.
 *************************************************/
void loopDevWifiAndMqtt()
{
    // -------------------------------------------------
    // Test 1: Process network connection
    //
    // This function must continue to be called while
    // CONNECTED so that connection loss can also be
    // detected.
    // -------------------------------------------------

    NetworkConnectionState networkState =
        processNetworkConnection();


    // -------------------------------------------------
    // Print network state changes
    // -------------------------------------------------

    if (networkState != previousNetworkState)
    {
        Serial.printf(
            "Network state changed: %s -> %s\n",
            getNetworkStateName(previousNetworkState),
            getNetworkStateName(networkState));

        previousNetworkState = networkState;
    }


    // -------------------------------------------------
    // Test 2: Complete network connection cycle failed
    //
    // The network_manager remains in FAILED until a
    // new connection cycle is started externally.
    // -------------------------------------------------

    if (networkState == NetworkConnectionState::FAILED)
    {
        return;
    }


    // -------------------------------------------------
    // Network is still connecting or reconnecting
    //
    // MQTT processing is only started once a usable
    // network connection is available.
    // -------------------------------------------------

    if (networkState != NetworkConnectionState::CONNECTED)
    {
        return;
    }


    // -------------------------------------------------
    // Test 3: Process MQTT connection
    // -------------------------------------------------

    MqttConnectionState mqttState =
        processMqttConnection(getDeviceId());


    // -------------------------------------------------
    // Print MQTT state changes
    // -------------------------------------------------

    if (mqttState != previousMqttState)
    {
        Serial.printf(
            "MQTT state changed: %s -> %s\n",
            getMqttStateName(previousMqttState),
            getMqttStateName(mqttState));

        previousMqttState = mqttState;
    }


    // -------------------------------------------------
    // MQTT is not connected yet
    // -------------------------------------------------

    if (mqttState != MqttConnectionState::CONNECTED)
    {
        return;
    }


    // -------------------------------------------------
    // Test 4: Maintain MQTT connection
    //
    // Processes MQTT keep-alive and incoming data.
    // -------------------------------------------------

    mqttLoop();


    // -------------------------------------------------
    // Test 5: Publish telemetry periodically
    // -------------------------------------------------

    const uint32_t now = millis();

    if (now - lastPublishTime < PUBLISH_INTERVAL_MS)
    {
        return;
    }

    lastPublishTime = now;
    publishCounter++;

    Serial.println();

    Serial.printf(
        "Publishing test telemetry [%lu]:\n",
        static_cast<unsigned long>(
            publishCounter));

    Serial.println(getTelemetry());

    mqttPublish(
        getDeviceId(),
        getTelemetry());
}