#pragma once

#include "app/measurement_record.h"

enum class RecordValidity
{
    DISCARD,         // Record cannot be updated and should be discarded
    KEEP,            // Record may be updated later when time is available
    VALID            // Timestamp is already valid or was updated successfully
};

char* getTelemetry(void);
RecordValidity checkValidity(void);
bool  updateData(void);
MeasurementRecord& getCurrentData(void);