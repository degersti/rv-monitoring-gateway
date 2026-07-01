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
MqttConnectionState processMqttConnection(void);
void resetMqttConnection(void);
bool mqttConnect(void);
bool getMqttConnectionStatus(void);
bool mqttPublish(const char* payload);
void mqttLoop();
