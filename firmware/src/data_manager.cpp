#include <Arduino.h>
#include "config.h"
#include "data_manager.h"

/*************************************************
 * Function:    getTelemetry
 * Description: Collects all sensor data (battery voltages, temperature, humidity, and alarm states), formats it into a JSON string, and returns a pointer to the formatted telemetry data.
 * Parameters:  Reference to a SensorData struct containing the sensor data
 * Returns:     Pointer to a character array containing the formatted telemetry data in JSON format
 * Notes:       - Calls the functions to read battery voltages, SHT31 sensor data, and alarm states to populate the SensorData struct
 *              - Uses snprintf to format the collected data into a JSON string stored in the payload array
 *              - The JSON string includes fields for houseBatteryVoltage, engineBatteryVoltage, temperature, humidity, waterAlarm, and smokeAlarm
 *              - The function returns a pointer to the payload array containing the formatted telemetry data for use in other parts of the program (e.g., for MQTT publishing)
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