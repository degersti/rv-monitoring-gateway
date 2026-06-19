#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

// HiveMQ Cloud
const char* mqtt_server = "2f48189618fb478e8cf80b9d4f4825d1.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;

// MQTT client
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
// Device ID for MQTT topic
const String deviceId = "rv-gateway";
// Payload buffer
char payload[256];
float house_voltage = 13.5;
float engine_voltage = 12.7;
float temperature = 22.4;
float humidity = 58.1;
bool water_alarm = false;
bool smoke_alarm = false;

// Function to connect to MQTT broker
void connectMQTT()
{
    mqttClient.setServer(mqtt_server, mqtt_port);

    Serial.print("Connecting to HiveMQ");

    while (!mqttClient.connected())
    {
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
        }
        else
        {
            Serial.print(".");
            Serial.print(" state=");
            Serial.println(mqttClient.state());
            delay(2000);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    WiFi.begin(wifi_ssid, wifi_password);

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Testweise ohne Zertifikatsprüfung.
    // Für Prototyp okay, später besser mit Root-CA.
    espClient.setInsecure();

    connectMQTT();
}

void loop()
{
    if (!mqttClient.connected())
    {
        connectMQTT();
    }

    mqttClient.loop();

    static unsigned long lastSend = 0;

    if (millis() - lastSend > 10000)
    {
        lastSend = millis();

        snprintf(
            payload,
            sizeof(payload),
            "{\"house_voltage\":%.1f,\"engine_voltage\":%.1f,\"temperature\":%.1f,\"humidity\":%.1f,\"water_alarm\":%s,\"smoke_alarm\":%s}",
            house_voltage,
            engine_voltage,
            temperature,
            humidity,
            water_alarm ? "true" : "false",
            smoke_alarm ? "true" : "false"
        );

        String topic = deviceId + "/telemetry";
        mqttClient.publish(topic.c_str(), payload);

        Serial.println("Payload message sent");
        Serial.println(payload);
    }
}