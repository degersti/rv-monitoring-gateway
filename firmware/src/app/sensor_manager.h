#pragma once

#include "app/data_manager.h"

bool initSensorManager(void);
bool readEnvironmentalValues(SensorData& data);
void readBatteryVoltages(SensorData& data);
void readAlarmPins(SensorData& data);

