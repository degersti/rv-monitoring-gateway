#pragma once

#include "app/data_manager.h"

bool initSensorManager(void);
void readSHT31(SensorData& data);
void readBatteryVoltages(SensorData& data);
void readAlarms(SensorData& data);

