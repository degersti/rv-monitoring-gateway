/*************************************************
 * File:        mqtt_client.cpp
 * Author:      Markus Gerstenberg
 * Date:        2026-06-18
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
#include "config.h"
#include "secrets.h"
#include <PubSubClient.h>

// mqtt client instance
static PubSubClient mqttClient;
// mqtt client credentials 
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const String deviceId = DEVICE_ID;

/*************************************************
 * Function:    initMqtt
 * Description: Initializes the MQTT client and 
 *              configures the broker connection.
 * Parameters:  client - Network client used for 
 *              MQTT communication
 * Returns:     None (void function)
 * Notes:       Must be called before mqttConnect()
 *************************************************/
void initMqtt(Client& client)
{
    mqttClient.setClient(client);
    mqttClient.setServer(mqtt_server, mqtt_port);
}
/*************************************************
 * Function:    mqttConnect
 * Description: Establishes a connection to the 
 *              MQTT broker if not already connected.
 * Parameters:  none
 * Returns:     true  - Connected successfully
 *              false - Connection failed
 * Notes:       Publishes an online status message 
 *              after a successful connection.
 *************************************************/
bool mqttConnect(void)
{
    if (mqttClient.connected())
    {
        return true;
    }

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
        return true;
    }

    Serial.print(" failed, state=");
    Serial.println(mqttClient.state());

    return false;
}
/*************************************************
 * Function:    mqttLoop
 * Description: Processes MQTT client background 
 *              tasks and maintains the connection.
 * Parameters:  none
 * Returns:     none
 * Notes:       Should be called regularly from 
 *              the main loop.
 *************************************************/
void mqttLoop(void)
{
    mqttClient.loop();
}
/*************************************************
 * Function:    getMqttConnectionStatus
 * Description: Returns the current MQTT 
 *              connection state.
 * Parameters:  None
 * Returns:     true  - MQTT connected 
 *              false - MQTT disconnected
 * Notes:       Wrapper around PubSubClient.
 *************************************************/
bool getMqttConnectionStatus(void)
{
    return mqttClient.connected();
}
/*************************************************
 * Function:    mqttPublish
 * Description: Publishes telemetry data to the 
 *              configured MQTT topic.
 * Parameters:  payload - JSON payload string to 
 *              publish
 * Returns:     true  - Publish successful
 *              false - Publish failed
 * Notes:       Data is published to: 
 *              <DEVICE_ID>/telemetry
 *************************************************/
bool mqttPublish(const char* payload)
{
    String topic = String(DEVICE_ID) + "/telemetry";

    bool result = mqttClient.publish(topic.c_str(), payload);

    if (result)
    {
        Serial.println("Payload message sent");
        Serial.println(payload);
    }
    else
    {
        Serial.println("Payload message failed");
    }

    return result;
}
