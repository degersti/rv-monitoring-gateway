#pragma once

#include <Client.h>

enum class NetworkConnectionState
{
    IDLE,
    CONNECTING,
    CONNECTED,
    FAILED
};

enum class NetworkConnectionPhase
{
    PRIORITY,
    FALLBACK,
    FAILED
};

enum class NetworkType
{
    WIFI,
    CELLULAR
};

void initNetwork(NetworkType priorityNetwork, bool fallbackEnabled);
NetworkConnectionState processNetworkConnection(void);
void disconnectNetwork(void);
bool setActiveNetwork(NetworkType network);
bool getNetworkConnectionState(void);
Client& getNetworkClient(void);
NetworkType getActiveNetwork(void);
NetworkType getPriorityNetwork(void);
bool isFallbackEnabled(void);
bool isFallbackActive(void);