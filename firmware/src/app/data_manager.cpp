/*************************************************
 * File:        data_manager.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Generates telemetry payload data.
 *
 * Responsibilities:
 * - Sensor data collection
 * - Telemetry formatting
 * - JSON payload generation
 * - Data preparation for MQTT
 *
 *************************************************/

#include <Arduino.h>
#include "config.h"
#include "app/debug_logger.h"
#include "app/data_manager.h"
#include "app/runtime_manager.h"
#include "app/time_manager.h"
#include "app/sensor_manager.h"


// Temporary JSON payload buffer
static char payload[TELEMETRY_PAYLOAD_SIZE];

// Current sensor and alarm values
static MeasurementRecord data = {
    .bootEpochId = 0,
    .timestamp = 0,
    .houseBatteryVoltage = -999.0f,
    .engineBatteryVoltage = -999.0f,
    .temperature = -999.0f,
    .humidity = -999.0f,
    .waterAlarm = false,
    .smokeAlarm = false
};
/*************************************************
 * Function:    getTelemetry
 * Description: Collects all sensor values and
 *              formats them into a JSON payload.
 * Parameters:  data - Reference to a SensorData
 *                     structure
 * Returns:     Pointer to the JSON payload string
 * Notes:       Reads battery voltages,
 *              temperature, humidity and alarm
 *              states, then converts the data
 *              into a telemetry payload suitable
 *              for MQTT transmission.
 *************************************************/
char* getTelemetry(void)
{
    // Format the data into a JSON string
    snprintf(
            payload,
            TELEMETRY_PAYLOAD_SIZE,
            "{\"bootEpochId\":%lu,\"timestamp\":%lu,\"houseBatteryVoltage\":%.1f,\"engineBatteryVoltage\":%.1f,\"temperature\":%.1f,\"humidity\":%.1f,\"waterAlarm\":%s,\"smokeAlarm\":%s}",
            (unsigned long)data.bootEpochId,
            (unsigned long)data.timestamp,
            data.houseBatteryVoltage,
            data.engineBatteryVoltage,
            data.temperature,
            data.humidity,
            data.waterAlarm ? "true" : "false",
            data.smokeAlarm ? "true" : "false"
        );
    return payload;
}
/*************************************************
 * Function:    hasValidTimestamp
 * Description: Checks whether the current
 *              timestamp is already a valid
 *              Unix timestamp.
 * Parameters:  None
 * Returns:     True if the timestamp is valid,
 *              otherwise false.
 * Notes:       Relative timestamps are identified
 *              by values below
 *              VALID_TIMESTAMP_VALUE.
 *************************************************/
static bool hasValidTimestamp(void)
{
    return data.timestamp >= VALID_TIMESTAMP_VALUE;
}
/*************************************************
 * Function:    checkValidity
 * Description: Validates the current measurement
 *              record and updates a relative
 *              timestamp when possible.
 * Parameters:  None
 * Returns:     RecordValidity indicating whether
 *              the record is valid, should be
 *              retained for later processing or
 *              discarded.
 * Notes:       Relative timestamps are converted
 *              to absolute Unix timestamps when
 *              NTP time is available and the
 *              record belongs to the current
 *              boot epoch.
 *************************************************/
RecordValidity checkValidity(void)
{
    const uint32_t currentBootEpochId = getBootEpochId();

    LOG_DEBUG(
        "Checking record validity: bootEpochId=%lu, currentBootEpochId=%lu, timestamp=%lu",
        data.bootEpochId,
        currentBootEpochId,
        data.timestamp
    );

    if (hasValidTimestamp())
    {
        LOG_INFO("Record status: VALID");
        return RecordValidity::VALID;
    }

    if (data.bootEpochId != currentBootEpochId)
    {
        LOG_INFO("Record status: DISCARD [invalid bootEpochId]");

        return RecordValidity::DISCARD;
    }

    if (!isTimeAvailable())
    {
        LOG_INFO("Record status: KEEP [time not available]");

        return RecordValidity::KEEP;
    }

    const uint32_t relativeTimestamp = data.timestamp;
    data.timestamp = reconstructTimestamp(relativeTimestamp);

    LOG_DEBUG(
        "Timestamp reconstructed: relative=%lu, absolute=%lu",
        relativeTimestamp,
        data.timestamp
    );

    if (hasValidTimestamp())
    {
        LOG_INFO("Record status: VALID");
        return RecordValidity::VALID;
    }

    LOG_INFO("Record status: DISCARD [timestamp reconstruction failed]");

    return RecordValidity::DISCARD;
}
/*************************************************
 * Function:    updateSensorData
 * Description: Collects the latest sensor and
 *              alarm values from all hardware
 *              modules.
 * Parameters:  None
 * Returns:     None
 * Notes:       Updates the internal SensorData
 *              structure with the latest battery
 *              voltages, environmental sensor
 *              readings and alarm states.
 *************************************************/
bool updateData(void)
{
    bool success = true;

    data.bootEpochId = getBootEpochId();
    data.timestamp = getCurrentTimestamp();

    readBatteryVoltages(data);
    success &= readEnvironmentalValues(data);
    readAlarmPins(data);

    LOG_INFO("Record status: UPDATED");

    LOG_DEBUG(
        "Record details: "
        "bootEpochId=%lu, "
        "timestamp=%lu (%s), "
        "houseBattery=%.2f V, "
        "engineBattery=%.2f V, "
        "temperature=%.1f °C, "
        "humidity=%.1f %%, "
        "waterAlarm=%s, "
        "smokeAlarm=%s",
        (unsigned long)data.bootEpochId,
        (unsigned long)data.timestamp,
        hasValidTimestamp() ? "absolute" : "relative",
        data.houseBatteryVoltage,
        data.engineBatteryVoltage,
        data.temperature,
        data.humidity,
        data.waterAlarm ? "YES" : "NO",
        data.smokeAlarm ? "YES" : "NO"
    );

    return success;
}
/*************************************************
 * Function:    getCurrentData
 * Description: Returns a reference to the current
 *              sensor and alarm data structure.
 * Parameters:  None
 * Returns:     Reference to the current SensorData
 *              structure
 * Notes:       Provides access to the latest
 *              collected sensor and alarm values.
 *************************************************/
MeasurementRecord& getCurrentData()
{
    return data;
}