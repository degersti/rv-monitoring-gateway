/*************************************************
 * File:        mqtt_client.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Handles MQTT broker communication including
 * connection management and telemetry publishing.
 *
 * Responsibilities:
 * - MQTT client initialization
 * - Broker connection handling
 * - Connection monitoring
 * - Telemetry publishing
 *
 *************************************************/

#include <Arduino.h>
#include "mqtt_client.h"
#include "status_indicator.h"
#include "wifi_manager.h"
#include "config.h"
#include "secrets.h"
#include <PubSubClient.h>

// MQTT client instance
static PubSubClient mqttClient;

// MQTT client credentials
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const String deviceId = DEVICE_ID;

static MqttConnectionState mqttState = MqttConnectionState::IDLE;
static uint32_t lastReconnectAttempt = 0;
static bool mqttInitialized = false;

/*************************************************
 * Function:    initMqtt
 * Description: Initializes the MQTT client and
 *              configures the broker connection.
 *************************************************/
void initMqtt(Client& client)
{
    mqttClient.setClient(client);
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttState = MqttConnectionState::IDLE;
    mqttInitialized = true;
}

/*************************************************
 * Function:    tryMqttConnect
 * Description: Attempts to establish a connection
 *              to the MQTT broker.
 * Parameters:  None
 * Returns:     true  - Connection successful
 *              false - Connection failed
 * Notes:       Generates a unique client ID,
 *              publishes an online status
 *              message after successful
 *              connection and updates the
 *              internal MQTT connection state.
 *************************************************/
static bool tryMqttConnect(void)
{
    Serial.print("Connecting to HiveMQ");

    String clientId = deviceId + "-" + String(random(0xffff), HEX);

    if (mqttClient.connect(
            clientId.c_str(),
            mqtt_user,
            mqtt_password))
    {
        Serial.println();
        Serial.println("HiveMQ connected");

        String topic = deviceId + "/status";
        mqttClient.publish(topic.c_str(), "{\"status\":\"online\"}");

        Serial.println("Status message sent");
        mqttState = MqttConnectionState::CONNECTED;
        return true;
    }

    Serial.print(" failed, state=");
    Serial.println(mqttClient.state());

    mqttState = MqttConnectionState::FAILED;
    lastReconnectAttempt = millis();
    return false;
}

/*************************************************
 * Function:    processMqttConnection
 * Description: Handles MQTT connection attempts
 *              with retry timing.
 * Parameters:  None
 * Returns:     Current MQTT connection state
 * Notes:       PubSubClient connect itself is still
 *              synchronous, but this function avoids
 *              aggressive reconnect loops.
 *************************************************/
MqttConnectionState processMqttConnection(void)
{
    const uint32_t now = millis();

    if (!mqttInitialized)
    {
        initMqtt(getWifiClient());
    }

    if (!getWiFiConnectionStatus())
    {
        if (mqttClient.connected())
        {
            mqttClient.disconnect();
        }

        mqttState = MqttConnectionState::IDLE;
        return mqttState;
    }

    if (mqttClient.connected())
    {
        mqttClient.loop();
        mqttState = MqttConnectionState::CONNECTED;
        return mqttState;
    }

    if (mqttState == MqttConnectionState::FAILED &&
        now - lastReconnectAttempt < MQTT_RETRY_INTERVAL_MS)
    {
        return mqttState;
    }

    mqttState = MqttConnectionState::CONNECTING;
    tryMqttConnect();

    return mqttState;
}

/*************************************************
 * Function:    resetMqttConnection
 * Description: Disconnects MQTT and resets the
 *              reconnect state.
 *************************************************/
void resetMqttConnection(void)
{
    if (mqttClient.connected())
    {
        mqttClient.disconnect();
    }

    mqttState = MqttConnectionState::IDLE;
    lastReconnectAttempt = 0;
}

/*************************************************
 * Function:    mqttConnect
 * Description: Compatibility wrapper for older code.
 *************************************************/
bool mqttConnect(void)
{
    return processMqttConnection() == MqttConnectionState::CONNECTED;
}

/*************************************************
 * Function:    mqttLoop
 * Description: Processes MQTT client background
 *              tasks and maintains the connection.
 *************************************************/
void mqttLoop(void)
{
    if (mqttClient.connected())
    {
        mqttClient.loop();
    }
}

/*************************************************
 * Function:    getMqttConnectionStatus
 * Description: Returns the current MQTT connection
 *              state.
 *************************************************/
bool getMqttConnectionStatus(void)
{
    return mqttClient.connected();
}

/*************************************************
 * Function:    mqttPublish
 * Description: Publishes telemetry data to the
 *              configured MQTT topic.
 *************************************************/
bool mqttPublish(const char* payload)
{
    if (!mqttClient.connected())
    {
        Serial.println("Payload message failed: MQTT disconnected");
        return false;
    }

    String topic = String(DEVICE_ID) + "/telemetry";

    bool result = mqttClient.publish(topic.c_str(), payload);

    if (result)
    {
        triggerTelemetryFlash();
        Serial.println("Payload message sent");
        Serial.println(payload);
    }
    else
    {
        Serial.println("Payload message failed");
    }

    return result;
}
