#pragma once

#include <Client.h>

enum class WiFiConnectionState
{
    IDLE,
    CONNECTING,
    CONNECTED,
    FAILED
};

void initWifi(void);
WiFiConnectionState processWifiConnection(void);
void disconnectWifi(void);
bool getWiFiConnectionStatus(void);
Client& getWifiClient(void);

// Compatibility wrapper. Prefer processWifiConnection() in the state machine.
bool connectWifi(void);
