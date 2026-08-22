#pragma once

#include <Arduino.h>
#include <Client.h>
#include "app/network_manager.h"


void initCellular(void);
NetworkConnectionState processCellularConnection(void);
void disconnectCellular(void);
bool isCellularConnected(void);
Client& getCellularClient(void);