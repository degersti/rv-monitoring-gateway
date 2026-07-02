#include <Arduino.h>

#include "app/wifi_manager.h"
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
    initWifi();
    initMqtt(getWifiClient());

    Serial.println("WiFi and MQTT initialized");
}

void loopDevWifiAndMqtt()
{
    WiFiConnectionState wifiState = processWifiConnection();

    if (wifiState != WiFiConnectionState::CONNECTED)
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

    SensorData testData;

    testData.houseBatteryVoltage = 12.4 + (publishCounter % 5) * 0.1;
    testData.engineBatteryVoltage = 12.7 + (publishCounter % 3) * 0.1;
    testData.temperature = 21.5 + (publishCounter % 10) * 0.2;
    testData.humidity = 55.0 + (publishCounter % 8);
    testData.waterAlarm = false;
    testData.smokeAlarm = false;

    char payload[256];

    getTelemetry(payload, testData);

    Serial.println();
    Serial.println("Publishing test telemetry:");
    Serial.println(payload);

    mqttPublish(getDeviceId(), payload);
}