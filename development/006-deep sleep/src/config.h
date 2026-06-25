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
// Runtime variables
// --------------------------------------------------


