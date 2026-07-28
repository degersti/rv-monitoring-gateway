#pragma once

struct MeasurementRecord
{
    uint32_t bootEpochId;
    uint32_t timestamp;

    float houseBatteryVoltage;
    float engineBatteryVoltage;

    float temperature;
    float humidity;

    bool waterAlarm;
    bool smokeAlarm;
};
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