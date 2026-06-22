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
#include "data_manager.h"

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
char* getTelemetry(char* payload, SensorData& data)
{
    // Format the data into a JSON string
    snprintf(
            payload,
            TELEMETRY_PAYLOAD_SIZE,
            "{\"houseBatteryVoltage\":%.1f,\"engineBatteryVoltage\":%.1f,\"temperature\":%.1f,\"humidity\":%.1f,\"waterAlarm\":%s,\"smokeAlarm\":%s}",
            data.houseBatteryVoltage,
            data.engineBatteryVoltage,
            data.temperature,
            data.humidity,
            data.waterAlarm ? "true" : "false",
            data.smokeAlarm ? "true" : "false"
        );
    return payload;
}