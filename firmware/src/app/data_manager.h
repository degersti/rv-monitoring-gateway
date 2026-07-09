#pragma once

struct SensorData
{
    uint32_t bootEpochId;
    uint64_t timestamp;

    float houseBatteryVoltage;
    float engineBatteryVoltage;

    float temperature;
    float humidity;

    bool waterAlarm;
    bool smokeAlarm;
};

char* getTelemetry(void);
bool  updateData(void);