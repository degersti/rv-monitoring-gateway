#pragma once

#include <Arduino.h>
#include "measurement_record.h"

bool initBuffer(bool hasNewBootEpoch);
bool pushRecord(const MeasurementRecord& record);
bool readOldestRecord(MeasurementRecord& record);
bool removeOldestRecord();
bool isBufferEmpty();
uint16_t getRecordCount();
uint16_t getAbsoluteRecordCount();
uint16_t getRelativeRecordCount();
