#pragma once

#include <Arduino.h>
#include "app/data_manager.h"

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
bool isBufferEmpty();
bool isBufferFull();
uint16_t getRecordCount();
uint32_t getOverflowCount();
bool resetOverflowCount();

