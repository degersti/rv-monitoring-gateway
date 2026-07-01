#pragma once

#include "esp_sleep.h"
// --------------------------------------------------
// Pin configuration
// --------------------------------------------------

#define SERIAL_DEBUG_PIN GPIO_NUM_1

#define WATER_ALARM_PIN GPIO_NUM_12
#define SMOKE_ALARM_PIN GPIO_NUM_13

constexpr uint64_t WAKEUP_PIN_MASK =
    (1ULL << WATER_ALARM_PIN) |
    (1ULL << SMOKE_ALARM_PIN);

// ESP32-S3 DevKitC onboard RGB LED
constexpr uint8_t PIN_RGB_LED = 48;
// --------------------------------------------------
// Timing configuration
// --------------------------------------------------

constexpr uint32_t SERIAL_MONITOR_WAIT_MS = 10000;
constexpr uint32_t CYCLE_INTERVAL_SEC = 15;

// --------------------------------------------------
// NTP configuration
// --------------------------------------------------

constexpr char NTP_SERVER[] = "pool.ntp.org";
constexpr uint32_t NTP_SYNC_INTERVAL_SECONDS = 86400; //24h
constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 10000; //10sec

// --------------------------------------------------
// Measurement buffer configuration
// --------------------------------------------------
constexpr uint16_t MEASUREMENT_BUFFER_SIZE = 5;