#pragma once

#include <Client.h>

enum class MqttConnectionState
{
    IDLE,
    CONNECTING,
    CONNECTED,
    FAILED
};

void initMqtt(Client& client);
MqttConnectionState processMqttConnection(const char* deviceId);
void disconnectMqtt(void);
bool mqttConnect(const char* deviceId);
bool getMqttConnectionStatus(void);
bool mqttPublish(const char* deviceId, const char* payload);
void mqttLoop();
