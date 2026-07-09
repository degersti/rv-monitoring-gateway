#pragma once

struct SensorData
{
    float houseBatteryVoltage;
    float engineBatteryVoltage;

    float temperature;
    float humidity;

    bool waterAlarm;
    bool smokeAlarm;
};

char* getTelemetry(void);
void  updateData(void);