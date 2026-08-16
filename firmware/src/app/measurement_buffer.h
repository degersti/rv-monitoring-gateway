#pragma once

#include <Arduino.h>
#include "measurement_record.h"

// Initializes the persistent single-file SD buffer.
bool initBuffer(bool hasNewBootEpoch);

// Appends one measurement record to the buffer.
bool pushRecord(const MeasurementRecord& record);

// Reads the oldest absolute record without removing it.
bool readOldestRecord(MeasurementRecord& record);

// Removes the oldest absolute record after transmission.
bool removeOldestRecord();

// Returns true when no active records are buffered.
bool isBufferEmpty();

// Returns active record counts from persistent metadata.
uint16_t getRecordCount();
uint16_t getAbsoluteRecordCount();
uint16_t getRelativeRecordCount();
