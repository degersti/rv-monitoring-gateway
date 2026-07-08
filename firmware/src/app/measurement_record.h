#pragma once

#include "stdint.h"

/*************************************************
 * Struct:      MeasurementRecord
 * Description: Persistent measurement data model.
 * Notes:       Used by data_manager, measurement_
 *              buffer and mqtt_manager.
 *************************************************/
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