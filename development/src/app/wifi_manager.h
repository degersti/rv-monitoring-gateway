#pragma once

#include <Client.h>
#include "app/network_manager.h"

void initWifi(void);
NetworkConnectionState processWifiConnection(void);
void disconnectWifi(void);
bool getWiFiConnectionState(void);
Client& getWifiClient(void);
bool connectWifi(void);
