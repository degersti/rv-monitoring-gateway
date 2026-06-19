#pragma once

#include <Client.h>

void initMqtt(Client& client);
bool mqttConnect(void);
bool getMqttConnectionStatus(void);
bool mqttPublish(const char* payload);
void mqttLoop();
