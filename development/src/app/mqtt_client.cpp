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
#include <PubSubClient.h>
#include "config.h"
#include "secrets.h"
#include "app/mqtt_client.h"
#include "app/wifi_manager.h"
#include "app/debug_logger.h"


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
 *              configures the broker settings.
 * Parameters:  client - Network client used for
 *                       secure MQTT communication.
 * Returns:     None
 * Notes:       Does not establish a connection.
 *              The connection is handled by the
 *              MQTT state machine.
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
    LOG_INFO("Connecting to HiveMQ");

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
 * Parameters:  None
 * Returns:     None    
 * Notes:       The next call to processMqttConnection
 *              will attempt to reconnect.
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
 * Parameters:  None
 * Returns:     true  - MQTT connected
 *              false - MQTT not connected yet or failed
 * Notes:       Starts or continues a non-blocking
 *              connection attempt.
 *************************************************/
bool mqttConnect(void)
{
    return processMqttConnection() == MqttConnectionState::CONNECTED;
}

/*************************************************
 * Function:    mqttLoop
 * Description: Processes MQTT client background
 *              tasks and maintains the connection.
 * Parameters:  None
 * Returns:     None    
 * Notes:       Should be called frequently in the main
 *              loop to ensure timely handling of
 *              MQTT messages.
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
 * Parameters:  None
 * Returns:     true  - MQTT connected 
 *              false - MQTT not connected
 * Notes:       None
 *************************************************/
bool getMqttConnectionStatus(void)
{
    return mqttClient.connected();
}

/*************************************************
 * Function:    mqttPublish
 * Description: Publishes telemetry data to the
 *              configured MQTT topic.
 * Parameters:  payload - JSON-formatted telemetry data
 * Returns:     true  - Message published successfully
 *              false - Message failed to publish
 * Notes:       None
 *************************************************/
bool mqttPublish(const char* payload)
{
    if (!mqttClient.connected())
    {
        LOG_WARN("Payload message failed: MQTT disconnected");
        return false;
    }

    String topic = String(DEVICE_ID) + "/telemetry";

    bool result = mqttClient.publish(topic.c_str(), payload);

    if (result)
    {
        LOG_INFO("Payload message sent");
        LOG_DEBUG("%s", payload);
    }
    else
    {
        LOG_WARN("Payload message failed");
    }

    return result;
}
