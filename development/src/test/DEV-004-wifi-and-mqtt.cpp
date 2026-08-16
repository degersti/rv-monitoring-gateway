#include <Arduino.h>

#include "app/network_manager.h"
#include "app/mqtt_client.h"
#include "app/data_manager.h"
#include "app/runtime_manager.h"

static uint32_t lastPublishTime = 0;
static uint32_t publishCounter = 0;

static const uint32_t PUBLISH_INTERVAL_MS = 10000;

void setupDevWifiAndMqtt()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("DEV-0054 WiFi and MQTT Test");
    Serial.println("--------------------------------");

    initRuntimeManager();
    initNetwork();
    initMqtt(getNetworkClient());

    Serial.println("WiFi and MQTT initialized");
}

void loopDevWifiAndMqtt()
{
    NetworkConnectionState NetworkState = processNetworkConnection();

    if (NetworkState != NetworkConnectionState::CONNECTED)
    {
        return;
    }

    MqttConnectionState mqttState = processMqttConnection(getDeviceId());

    if (mqttState != MqttConnectionState::CONNECTED)
    {
        return;
    }

    mqttLoop();

    uint32_t now = millis();

    if (now - lastPublishTime < PUBLISH_INTERVAL_MS)
    {
        return;
    }

    lastPublishTime = now;
    publishCounter++;

    Serial.println();
    Serial.println("Publishing test telemetry:");
    Serial.println(getTelemetry());

    mqttPublish(getDeviceId(), getTelemetry());
}