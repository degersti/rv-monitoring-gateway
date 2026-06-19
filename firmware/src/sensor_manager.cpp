/*************************************************
 * File:        sensor_manager.cpp
 * Author:      Markus Gerstenberg
 * Date:        2026-06-18
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
#include "sensor_manager.h"
#include "Wire.h"
#include "config.h"
#include "Adafruit_SHT31.h"

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
bool initHardware(void)
{
    // Configure ADC resolution
    analogReadResolution(ADC_RESOLUTION);
    
    // Initialize I2C interface
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
    if (!sht31.begin(SHT31_ADDR)) { 
        Serial.println("Sensor nicht gefunden!");
        return false;                               
    }
    Serial.println("Sensor gefunden!");
    
    // Initialize alarm inputs
    pinMode(PIN_WATER_ALARM, INPUT_PULLUP);
    pinMode(PIN_SMOKE_ALARM, INPUT_PULLUP);
    Serial.println("Inputs initialisiert!");

    return true;
}
/*************************************************
 * Function:    readBatteryVoltages
 * Description: Reads and converts battery voltages
 *              from ADC inputs.
 * Parameters:  data - Sensor data structure
 * Returns:     None (void function)
 * Notes:       Applies ADC scaling and voltage
 *              divider compensation.
 *************************************************/
 
void readBatteryVoltages(SensorData& data)
{
    // Read ADC values for house and engine battery voltages
    int houseAdcValue = analogRead(PIN_HOUSE_ADC);  
    int engineAdcValue = analogRead(PIN_ENGINE_ADC);   

    // Convert ADC values to voltages (assuming a 3.3V reference and a voltage divider)
    data.houseBatteryVoltage = (houseAdcValue / (float)ADC_MAX_VALUE) * ADC_REFERENCE_VOLTAGE * VOLTAGE_DIVIDER_RATIO;
    data.engineBatteryVoltage = (engineAdcValue / (float)ADC_MAX_VALUE) * ADC_REFERENCE_VOLTAGE * VOLTAGE_DIVIDER_RATIO;
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
void readSHT31(SensorData& data)
{
    // Read temperature and humidity from SHT31 sensor
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();

    // Check if the readings are valid
     if (isnan(temperature) || isnan(humidity))
    {
        Serial.println("SHT31 read failed");
        data.temperature = -999.0f;
        data.humidity = -999.0f;
        return;
    }

    // Store the readings in the SensorData struct
    data.temperature = temperature;
    data.humidity = humidity;    
}
/*************************************************
 * Function:    readAlarms
 * Description: Reads water and smoke alarm inputs.
 * Parameters:  data - Sensor data structure
 * Returns:     None (void function)
 * Notes:       Alarm inputs are active LOW.
 *************************************************/
void readAlarms(SensorData& data)
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
    readSHT31(data);
    readAlarms(data);
};
