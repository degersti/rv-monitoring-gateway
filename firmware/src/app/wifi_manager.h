#pragma once

#include <Client.h>
#include "app/network_manager.h"

void initWifi(void);
NetworkConnectionState processWifiConnection(void);
void disconnectWifi(void);
bool isWifiConnected(void);
Client& getWifiClient(void);
int getWifiRssi(void);
bool connectWifi(void);
