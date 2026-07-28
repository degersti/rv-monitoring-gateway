#pragma once

#include "app/data_manager.h"

bool initSensorManager(void);
bool readEnvironmentalValues(MeasurementRecord& data);
void readBatteryVoltages(MeasurementRecord& data);
void readAlarmPins(MeasurementRecord& data);

