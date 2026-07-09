/*************************************************
 * File:        sensor_manager.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Provides access to all hardware sensors and
 * digital alarm inputs.
 *
 * Responsibilities:
 * - Battery voltage measurement
 * - Temperature measurement
 * - Humidity measurement
 * - Water alarm monitoring
 * - Smoke alarm monitoring
 *
 *************************************************/

#include <Arduino.h>
#include "Wire.h"
#include "config.h"
#include "app/debug_logger.h"
#include "Adafruit_SHT31.h"
#include "app/sensor_manager.h"

// Initialize the SHT31 sensor object
Adafruit_SHT31 sht31 = Adafruit_SHT31();

/*************************************************
 * Function:    initHardware
 * Description: Initializes all hardware peripherals
 *              and sensors.
 * Parameters:  None
 * Returns:     true  - Initialization successful
 *              false - Initialization failed
 * Notes:       Configures ADC, I2C, SHT31 and
 *              alarm inputs.
 *************************************************/
bool initSensorManager(void)
{
    // Configure ADC resolution
    analogReadResolution(ADC_RESOLUTION);

    analogSetPinAttenuation(PIN_HOUSE_ADC, ADC_11db);
    analogSetPinAttenuation(PIN_ENGINE_ADC, ADC_11db);
   
    // Initialize I2C interface
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
    if (!sht31.begin(SHT31_ADDR)) { 
        LOG_WARN("Sensor nicht gefunden!");
        return false;                               
    }
    LOG_INFO("Sensor gefunden!");    
    return true;
}
/*************************************************
 * Function:    readVoltageRaw
 * Description: Reads an ADC channel multiple times,
 *              calculates the average value and
 *              converts it into the corresponding
 *              input voltage before calibration.
 * Parameters:  pin - ADC input pin
 * Returns:     Raw input voltage in volts
 * Notes:       Applies ADC reference scaling and
 *              voltage divider compensation.
 *              No calibration is applied.
 *************************************************/
float readVoltageRaw(int pin)
{
    uint32_t adcSum = 0;

    for (uint8_t i = 0; i < SAMPLE_COUNT; i++)
    {
        adcSum += analogRead(pin);
        delay(SAMPLE_DELAY_MS);
    }

    float adcRawAverage = adcSum / (float)SAMPLE_COUNT;

    float adcVoltage =
        (adcRawAverage / (float)ADC_MAX_VALUE) * ADC_REFERENCE_VOLTAGE;

    float inputVoltage =
        adcVoltage * VOLTAGE_DIVIDER_RATIO;

    return inputVoltage;
}
/*************************************************
 * Function:    applyCalibration
 * Description: Applies a linear calibration to a
 *              measured voltage value.
 * Parameters:  voltage - Raw voltage value
 *              gain    - Calibration gain factor
 *              offset  - Calibration offset
 * Returns:     Calibrated voltage in volts
 * Notes:       Uses the formula:
 *              calibrated = voltage * gain + offset
 *************************************************/
float applyCalibration(float voltage, float gain, float offset)
{
    return voltage * gain + offset;
}
/*************************************************
 * Function:    readBatteryVoltages
 * Description: Reads the raw battery voltages from
 *              the ADC input channels and applies
 *              channel-specific calibration.
 * Parameters:  data - Sensor data structure to be
 *              updated with calibrated voltages.
 * Returns:     None
 * Notes:       readVoltageRaw() performs ADC averaging,
 *              ADC scaling and voltage divider
 *              compensation.
 *              applyCalibration() applies gain and
 *              offset correction.
 *************************************************/
 void readBatteryVoltages(SensorData& data)
{
    // Read raw, divider-compensated battery voltages
    float rawHouseVoltage = readVoltageRaw(PIN_HOUSE_ADC);
    float rawEngineVoltage = readVoltageRaw(PIN_ENGINE_ADC);
     
    // Apply channel-specific calibration and store results
    data.houseBatteryVoltage = applyCalibration(rawHouseVoltage, CAL_GAIN_HOUSE_VOLTAGE, CAL_OFFSET_HOUSE_VOLTAGE);
    data.engineBatteryVoltage = applyCalibration(rawEngineVoltage, CAL_GAIN_ENGINE_VOLTAGE, CAL_OFFSET_ENGINE_VOLTAGE);
} 
/*************************************************
 * Function:    readSHT31
 * Description: Reads temperature and humidity
 *              from the SHT31 sensor.
 * Parameters:  data - Sensor data structure
 * Returns:     None (void function)
 * Notes:       Invalid readings are reported as
 *              error values.
 *************************************************/
bool readEnvironmentalValues(SensorData& data)
{
    // Read temperature and humidity from SHT31 sensor
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();

    // Check if the readings are valid
    if (isnan(temperature) || isnan(humidity))
    {
        LOG_ERROR("SHT31 read failed");
        data.temperature = -999.0f;
        data.humidity = -999.0f;
        return false;
    }

    // Store the readings in the SensorData struct
    data.temperature = temperature;
    data.humidity = humidity;    
    return true;
}
/*************************************************
 * Function:    readAlarms
 * Description: Reads water and smoke alarm inputs.
 * Parameters:  data - Sensor data structure
 * Returns:     None (void function)
 * Notes:       Alarm inputs are active LOW.
 *************************************************/
void readAlarmPins(SensorData& data)
{
    data.waterAlarm = digitalRead(PIN_WATER_ALARM) == LOW;
    data.smokeAlarm = digitalRead(PIN_SMOKE_ALARM) == LOW;
}
/*************************************************
 * Function:    readAllSensorData
 * Description: Reads all sensor and alarm data.
 * Parameters:  data - Sensor data structure
 * Returns:     None (void function)
 * Notes:       Convenience wrapper for all sensor
 *              acquisition functions.
 *************************************************/
void readAllSensorData(SensorData& data)
{
    // Read all sensor data
    readBatteryVoltages(data);
    readEnvironmentalValues(data);
    readAlarmPins(data);
};
