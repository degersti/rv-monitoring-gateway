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

char* getTelemetry(void);
bool  updateData(void);
MeasurementRecord& getCurrentData(void);