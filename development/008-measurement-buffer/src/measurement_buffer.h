#pragma once

#include <Arduino.h>
#include "measurement_record.h"

// Buffer metadata
struct MeasurementBufferMetadata
{
    uint32_t magic;
    uint16_t writeIndex;
    uint16_t readIndex;
    uint16_t recordCount;
    uint32_t overflowCounter;
};
// Initialization
bool initBuffer();

// Write operations
bool pushRecord(const MeasurementRecord& record);

// Read operations
bool readOldestRecord(MeasurementRecord& record);
bool removeOldestRecord();

// Status
bool isEmpty();
bool isFull();
uint16_t getRecordCount();
uint32_t getOverflowCount();
bool resetOverflowCount();

