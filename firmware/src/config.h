#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_sleep.h"

// ==================================================
// Hardware configuration
// ==================================================

// Debug / installation mode
constexpr uint8_t PIN_SERIAL_DEBUG_ENABLE = 1;

// Battery voltage measurement ADC pins
constexpr uint8_t PIN_HOUSE_ADC  = 4;
constexpr uint8_t PIN_ENGINE_ADC = 5;

// Alarm input pins
constexpr uint8_t PIN_WATER_ALARM = 12;
constexpr uint8_t PIN_SMOKE_ALARM = 13;

constexpr uint64_t WAKEUP_PIN_MASK =
    (1ULL << static_cast<gpio_num_t>(PIN_WATER_ALARM)) |
    (1ULL << static_cast<gpio_num_t>(PIN_SMOKE_ALARM));

// I2C pins for SHT31
constexpr uint8_t PIN_I2C_SDA = 40;
constexpr uint8_t PIN_I2C_SCL = 41;

// ESP32-S3 DevKitC onboard RGB LED
constexpr uint8_t PIN_RGB_LED = 48;

// SHT31 temperature / humidity sensor configuration
constexpr uint8_t SHT31_ADDR = 0x44;

// ==================================================
// Runtime / cycle configuration
// ==================================================

constexpr uint32_t SERIAL_MONITOR_WAIT_SEC  = 10;
constexpr uint32_t CYCLE_INTERVAL_MIN       = 5;
constexpr uint32_t WATCHDOG_TIMEOUT_SEC     = 30;

constexpr uint64_t SEC_TO_MS = 1000ULL;
constexpr uint64_t MIN_TO_MS = 60000ULL;
constexpr uint64_t MIN_TO_US = 60000000ULL;

// ==================================================
// WiFi configuration
// ==================================================

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS        = 10000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS         = 5000;
constexpr uint32_t WIFI_STATUS_PRINT_INTERVAL_MS  = 500;


// ==================================================
// MQTT configuration
// ==================================================
constexpr uint16_t MQTT_BUFFER_SIZE = 512;

constexpr const char* MQTT_SERVER =
    "2f48189618fb478e8cf80b9d4f4825d1.s1.eu.hivemq.cloud";

constexpr uint16_t MQTT_PORT = 8883;

constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 5000;

constexpr size_t TELEMETRY_PAYLOAD_SIZE = 256;


// ==================================================
// NTP / time synchronization configuration
// ==================================================

constexpr char NTP_SERVER[] = "pool.ntp.org";

constexpr uint32_t NTP_SYNC_INTERVAL_SECONDS = 86400; // 24 h
constexpr uint32_t NTP_SYNC_TIMEOUT_MS       = 10000; // 10 s
// Minimum plausible Unix timestamp.
// Values below this are treated as relative timestamps.
constexpr uint32_t VALID_TIMESTAMP_VALUE     = 1767222000; //01.01.2026 00:00 Uhr


// ==================================================
// ADC configuration
// ==================================================

constexpr uint8_t  ADC_RESOLUTION        = 12;
constexpr uint16_t ADC_MAX_VALUE         = (1 << ADC_RESOLUTION) - 1;
constexpr float    ADC_REFERENCE_VOLTAGE = 3.3f;


// ==================================================
// Battery voltage measurement configuration
// ==================================================

// Voltage divider:
// R1 = upper resistor
// R2 = lower resistor
constexpr float R1 = 100000.0f;
constexpr float R2 = 18000.0f;

constexpr float VOLTAGE_DIVIDER_RATIO = (R1 + R2) / R2;

// Calibration for house battery ADC input
// Optimized for approx. 9–15 V on GPIO4
constexpr float CAL_GAIN_HOUSE_VOLTAGE   = 1.022f;
constexpr float CAL_OFFSET_HOUSE_VOLTAGE = 0.439f;

// Calibration for engine battery ADC input
// Optimized for approx. 9–15 V on GPIO5
constexpr float CAL_GAIN_ENGINE_VOLTAGE   = 1.025f;
constexpr float CAL_OFFSET_ENGINE_VOLTAGE = 0.428f;

// ADC sampling
constexpr uint8_t  SAMPLE_COUNT    = 32;
constexpr uint16_t SAMPLE_DELAY_MS = 2;

// ==================================================
// Measurement buffer configuration
// ==================================================

constexpr uint16_t MEASUREMENT_BUFFER_SIZE = 256;