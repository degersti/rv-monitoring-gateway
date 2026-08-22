#pragma once

#include <Arduino.h>
#include <Client.h>

enum class NetworkConnectionState
{
    IDLE,
    CONNECTING,
    CONNECTED,
    FAILED
};

enum class NetworkType
{
    WIFI,
    CELLULAR
};

void initNetwork(void);
NetworkConnectionState processNetworkConnection(void);
void disconnectNetwork(void);
bool setActiveNetwork(NetworkType network);
bool getNetworkConnectionState(void);
Client& getNetworkClient(void);
NetworkType getActiveNetwork(void);