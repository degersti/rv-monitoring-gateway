/*************************************************
 * File:        DEV-004-wifi-and-mqtt.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Integration test for the network_manager and
 * mqtt_client modules using WiFi connectivity.
 *
 * Test objective:
 * Verifies that the network_manager can establish
 * a network connection and provide a usable client
 * to the MQTT layer.
 *
 * Tested functionality:
 * 1. Runtime manager initialization
 * 2. Network manager initialization
 * 3. WiFi connection through network_manager
 * 4. Network client provisioning through
 *    getNetworkClient()
 * 5. MQTT client initialization
 * 6. MQTT broker connection
 * 7. MQTT keep-alive processing
 * 8. Periodic telemetry publishing
 *
 * Expected result:
 * - processNetworkConnection() eventually returns
 *   NetworkConnectionState::CONNECTED
 * - processMqttConnection() eventually returns
 *   MqttConnectionState::CONNECTED
 * - MQTT keep-alive remains operational
 * - Telemetry is published every 10 seconds
 *
 * Notes:
 * - WiFi is accessed only through the public
 *   network_manager API.
 * - The wifi_manager is not accessed directly.
 * - This test verifies the integration between
 *   network connectivity and MQTT communication.
 * - Cellular connectivity is not tested here.
 *************************************************/

#include <Arduino.h>

#include "app/network_manager.h"
#include "app/mqtt_client.h"
#include "app/data_manager.h"
#include "app/runtime_manager.h"


static constexpr uint32_t PUBLISH_INTERVAL_MS = 10000;

static uint32_t lastPublishTime = 0;
static uint32_t publishCounter = 0;


/*************************************************
 * Function:    setupDevWifiAndMqtt
 * Description: Initializes all modules required
 *              for the WiFi and MQTT integration
 *              test.
 * Parameters:  None
 * Returns:     None
 * Notes:       Network and MQTT connections are
 *              established asynchronously in the
 *              loop function.
 *************************************************/
void setupDevWifiAndMqtt()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("--------------------------------");
    Serial.println("DEV-004 WiFi and MQTT Test");
    Serial.println("--------------------------------");

    // -------------------------------------------------
    // Initialize runtime services
    // -------------------------------------------------

    initRuntimeManager();

    // -------------------------------------------------
    // Initialize network manager
    // -------------------------------------------------

    initNetwork();

    // -------------------------------------------------
    // Initialize MQTT client
    //
    // The network client is provided by the
    // network_manager. The MQTT layer therefore does
    // not directly depend on the WiFi implementation.
    // -------------------------------------------------

    initMqtt(getNetworkClient());

    Serial.println("WiFi and MQTT initialized");
}


/*************************************************
 * Function:    loopDevWifiAndMqtt
 * Description: Processes network and MQTT
 *              connection handling and publishes
 *              test telemetry periodically.
 * Parameters:  None
 * Returns:     None
 * Notes:       Must be called repeatedly from the
 *              main application loop.
 *************************************************/
void loopDevWifiAndMqtt()
{
    // -------------------------------------------------
    // Test 1: Establish network connection
    // -------------------------------------------------

    NetworkConnectionState networkState =
        processNetworkConnection();

    if (networkState !=
        NetworkConnectionState::CONNECTED)
    {
        return;
    }

    // -------------------------------------------------
    // Test 2: Establish MQTT connection
    // -------------------------------------------------

    MqttConnectionState mqttState =
        processMqttConnection(getDeviceId());

    if (mqttState !=
        MqttConnectionState::CONNECTED)
    {
        return;
    }

    // -------------------------------------------------
    // Test 3: Process MQTT keep-alive and incoming data
    // -------------------------------------------------

    mqttLoop();

    // -------------------------------------------------
    // Test 4: Publish telemetry periodically
    // -------------------------------------------------

    const uint32_t now = millis();

    if (now - lastPublishTime <
        PUBLISH_INTERVAL_MS)
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