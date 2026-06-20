#include <Arduino.h>

// ADC Configuration
const uint8_t ADC_RESOLUTION = 12;
const uint16_t ADC_MAX_VALUE = (1 << ADC_RESOLUTION) - 1;
const float ADC_REFERENCE_VOLTAGE = 3.3f;

// Voltage divider
const float R1 = 100000.0f;
const float R2 = 18000.0f;
const float VOLTAGE_DIVIDER_RATIO = (R1 + R2) / R2;

// Calibration
// Pin 4, optimized for apprx. 9–15 V
const float CAL_GAIN_PIN1 = 1.022f;
const float CAL_OFFSET_PIN1 = 0.439f;

// Pin 5, optimized for apprx. 9–15 V
const float CAL_GAIN_PIN2 = 1.025f;
const float CAL_OFFSET_PIN2 = 0.428f;

// Pins
const int ADC_PIN1 = 4;
const int ADC_PIN2 = 5;

// Sampling
const uint8_t SAMPLE_COUNT = 32;
const uint16_t SAMPLE_DELAY_MS = 2;

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

float applyCalibration(float voltage, float gain, float offset)
{
    return voltage * gain + offset;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("RV Monitoring Gateway");
    Serial.println("001 - Voltage Monitoring Calibrated ADC Test");

    analogReadResolution(ADC_RESOLUTION);

    analogSetPinAttenuation(ADC_PIN1, ADC_11db);
    analogSetPinAttenuation(ADC_PIN2, ADC_11db);

    pinMode(ADC_PIN1, INPUT);
    pinMode(ADC_PIN2, INPUT);
}

void loop()
{
    float rawVoltage1 = readVoltageRaw(ADC_PIN1);
    float rawVoltage2 = readVoltageRaw(ADC_PIN2);

    float calibratedVoltage1 =
        applyCalibration(rawVoltage1, CAL_GAIN_PIN1, CAL_OFFSET_PIN1);

    float calibratedVoltage2 =
        applyCalibration(rawVoltage2, CAL_GAIN_PIN2, CAL_OFFSET_PIN2);

    Serial.print("Voltage on Pin ");
    Serial.print(ADC_PIN1);
    Serial.print(" raw: ");
    Serial.print(rawVoltage1, 2);
    Serial.print(" V | calibrated: ");
    Serial.print(calibratedVoltage1, 2);
    Serial.println(" V");

    Serial.print("Voltage on Pin ");
    Serial.print(ADC_PIN2);
    Serial.print(" raw: ");
    Serial.print(rawVoltage2, 2);
    Serial.print(" V | calibrated: ");
    Serial.print(calibratedVoltage2, 2);
    Serial.println(" V");

    Serial.println();

    delay(5000);
}