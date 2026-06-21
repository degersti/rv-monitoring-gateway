#pragma once

#include <stdint.h>
#include <stddef.h>

/*******************************************************
 * MQTT CONFIGURATION
 ******************************************************/
// Device and telemetry configuration
 constexpr size_t TELEMETRY_PAYLOAD_SIZE = 256;
 constexpr const char* DEVICE_ID = "rv-gateway";

// MQTT Broker configuration
constexpr const char* MQTT_SERVER =
    "2f48189618fb478e8cf80b9d4f4825d1.s1.eu.hivemq.cloud"; 
constexpr uint16_t MQTT_PORT = 8883;

/*******************************************************
 * HARDWARE CONFIGURATION
 ******************************************************/    
// ADC Configuration
constexpr uint8_t ADC_RESOLUTION = 12;
constexpr uint16_t ADC_MAX_VALUE = (1 << ADC_RESOLUTION) - 1;
constexpr float ADC_REFERENCE_VOLTAGE = 3.3f;

// Voltage divider configuration for battery voltage measurement
constexpr float R1 = 100000.0f;
constexpr float R2 = 18000.0f;
constexpr float VOLTAGE_DIVIDER_RATIO =
    (R1 + R2) / R2;

// ADC Pins
constexpr uint8_t PIN_HOUSE_ADC = 4;
constexpr uint8_t PIN_ENGINE_ADC = 5;

// Calibration
// Pin 4, optimized for apprx. 9–15 V
const float CAL_GAIN_HOUSE_VOLTAGE = 1.022f;
const float CAL_OFFSET_HOUSE_VOLTAGE = 0.439f;

// Pin 5, optimized for apprx. 9–15 V
const float CAL_GAIN_ENGINE_VOLTAGE = 1.025f;
const float CAL_OFFSET_ENGINE_VOLTAGE = 0.428f;

// Sampling
const uint8_t SAMPLE_COUNT = 32;
const uint16_t SAMPLE_DELAY_MS = 2;

// Alarm Inputs
constexpr uint8_t PIN_WATER_ALARM = 12;
constexpr uint8_t PIN_SMOKE_ALARM = 13;

// I2C Configuration for SHT31 sensor
constexpr uint8_t SHT31_ADDR = 0x44;
// I2C Pins
constexpr uint8_t PIN_I2C_SDA = 40;
constexpr uint8_t PIN_I2C_SCL = 41;

/*******************************************************
 * OTHER CONFIGURATION
 ******************************************************/
constexpr uint32_t TELEMETRY_INTERVAL_MS = 10000;

// WiFi connection handling
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 10000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 5000;
constexpr uint32_t WIFI_STATUS_PRINT_INTERVAL_MS = 500;

// MQTT connection handling
constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 5000;

// ESP32-S3 DevKitC onboard RGB LED
constexpr uint8_t PIN_RGB_LED = 48;