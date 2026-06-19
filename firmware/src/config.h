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