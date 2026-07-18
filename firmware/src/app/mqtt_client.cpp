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
    LOG_INFO("Initializing MQTT client: host=%s, port=%d, buffer=%u",
         mqtt_server,
         mqtt_port,
         MQTT_BUFFER_SIZE);

    mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
    mqttClient.setClient(client);
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttState = MqttConnectionState::IDLE;
    mqttInitialized = true;
}

/*************************************************
 * Function:    tryMqttConnect
 * Description: Attempts to establish a connection
 *              to the MQTT broker.
 * Parameters:  deviceId - Unique MQTT client ID
 *                         used during connection.
 * Returns:     true  - Connection successful
 *              false - Connection failed
 * Notes:       Generates a unique client ID,
 *              publishes an online status
 *              message after successful
 *              connection and updates the
 *              internal MQTT connection state.
 *************************************************/
static bool tryMqttConnect(const char* deviceId)
{
    LOG_INFO("MQTT status: CONNECTING_TO_BROKER [host=%s, port=%d]",
         mqtt_server,
         mqtt_port);

    char clientId[32];

    snprintf(clientId,
             sizeof(clientId),
             "%s-%04X",
             deviceId,
             random(0x10000));

    if (mqttClient.connect(clientId, mqtt_user, mqtt_password))
    {
        LOG_INFO("MQTT status: CONNECTED [clientId=%s]", clientId);

        char topic[64];

        snprintf(topic,
                 sizeof(topic),
                 "gateway/%s/status",
                 deviceId);

        mqttClient.publish(topic, "{\"status\":\"online\"}");

        LOG_DEBUG("MQTT publishing details: topic=%s, qos=0, retained=false", topic);

        mqttState = MqttConnectionState::CONNECTED;
        return true;
    }

    LOG_DEBUG("MQTT publishing details: state=%d", mqttClient.state());

    mqttState = MqttConnectionState::FAILED;
    lastReconnectAttempt = millis();

    return false;
}
/*************************************************
 * Function:    processMqttConnection
 * Description: Handles MQTT connection attempts
 *              with retry timing.
 * Parameters:  deviceId - Unique MQTT client ID
 *                         used during connection.
 * Returns:     Current MQTT connection state
 * Notes:       PubSubClient connect itself is still
 *              synchronous, but this function avoids
 *              aggressive reconnect loops.
 *************************************************/
MqttConnectionState processMqttConnection(const char* deviceId)
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
    tryMqttConnect(deviceId);

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
void disconnectMqtt(void)
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
 * Parameters:  deviceId - Unique MQTT client ID
 *                         used during connection.
 * Returns:     true  - MQTT connected
 *              false - MQTT not connected yet or failed
 * Notes:       Starts or continues a non-blocking
 *              connection attempt.
 *************************************************/
bool mqttConnect(const char* deviceId)
{
    return processMqttConnection(deviceId) == MqttConnectionState::CONNECTED;
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
 * Parameters:  deviceId - Unique device identifier
 *              payload - JSON-formatted telemetry data
 * Returns:     true  - Message published successfully
 *              false - Message failed to publish
 * Notes:       None
 *************************************************/
bool mqttPublish(const char* deviceId, const char* payload)
{
    if (!mqttClient.connected())
    {
        return false;
    }

    String topic = String("gateway/") + deviceId + "/measurement";

    LOG_DEBUG(
        "MQTT publishing details: topic=%s, qos=0, retained=false",
        topic.c_str());

    LOG_DEBUG("MQTT payload: %s", payload);

    return mqttClient.publish(topic.c_str(), payload);
}
