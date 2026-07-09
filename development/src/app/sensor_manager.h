#pragma once

#include "app/data_manager.h"

bool initSensorManager(void);
void readEnvironmentalValues(SensorData& data);
void readBatteryVoltages(SensorData& data);
void readAlarmPins(SensorData& data);

