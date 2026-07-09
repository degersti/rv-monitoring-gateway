#pragma once

#include "app/data_manager.h"

bool initSensorManager(void);
bool readEnvironmentalValues(SensorData& data);
bool readBatteryVoltages(SensorData& data);
bool readAlarmPins(SensorData& data);

