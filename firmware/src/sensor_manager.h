#pragma once

#include "data_manager.h"

bool initHardware(void);
void readSHT31(SensorData& data);
void readBatteryVoltages(SensorData& data);
void readAlarms(SensorData& data);

